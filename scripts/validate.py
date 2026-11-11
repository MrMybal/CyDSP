"""Run local Windows source/static/compile-only validation; never run CyDSP native code."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
from reporting import source_digest
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cmake', default=r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
p.add_argument('--skip-arm64', action='store_true', help='Skip and record ARM64 static compilation')
a = p.parse_args()
logs = ROOT / 'build/validation-logs'
logs.mkdir(parents=True, exist_ok=True)
report = {'sourceSha256':source_digest(),'timeUtc': datetime.now(timezone.utc).isoformat(), 'status': 'running',
          'nativeRuntimeExecuted': False, 'checks': [], 'unverified': [
              'native regressions, allocation tracing, SIMD/hardware dispatch execution',
              'audio quality and throughput/latency benchmarks',
              'ARM64 application executable link/runtime, ARM32, Linux, macOS and GCC',
              'Unreal integration and final application executable linking']}


def save():
    (ROOT / 'build/validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


def run(name, script, *arguments):
    result = subprocess.run([sys.executable, '-B', str(ROOT / 'scripts' / script), *arguments],
                            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (logs / (name + '.log')).write_text(result.stdout, encoding='utf-8')
    warning = bool(re.search(r'(?:warning|avertissement)\s*(?:[A-Z]+\d+|:)', result.stdout, re.I))
    entry = {'name': name, 'passed': result.returncode == 0 and not warning,
             'log': 'build/validation-logs/' + name + '.log'}
    report['checks'].append(entry)
    save()
    print(('PASS: ' if entry['passed'] else 'FAIL: ') + name, flush=True)
    if not entry['passed']:
        print(result.stdout[-10000:])
        raise RuntimeError(name)


def archive_machines(path):
    data = path.read_bytes()
    if data[:8] != b'!<arch>\n':
        raise ValueError('Not a COFF archive: ' + str(path))
    offset = 8
    machines = set()
    members = 0
    while offset < len(data):
        header = data[offset:offset + 60]
        if len(header) != 60 or header[58:] != b'`\n':
            raise ValueError('Malformed archive member')
        size = int(header[48:58])
        name = header[:16].strip()
        payload = data[offset + 60:offset + 60 + size]
        if len(payload) != size:
            raise ValueError('Truncated archive member')
        if name not in (b'/', b'//', b'/SYM64/'):
            if len(payload) < 8:
                raise ValueError('Truncated object')
            machine = int.from_bytes(payload[:2], 'little')
            if payload[:4] == b'\x00\x00\xff\xff':
                machine = int.from_bytes(payload[6:8], 'little')
            machines.add(machine)
            members += 1
        offset += 60 + size + (size % 2)
    return machines, members


save()
try:
    run('sources', 'verify_sources.py')
    for preset in ('windows-x64', 'windows-x86', 'windows-x64-scalar'):
        run(preset, 'build.py', '--preset', preset, '--compile-checks', '--cmake', a.cmake)
    if a.skip_arm64:
        report['checks'].append({'name': 'windows-arm64-clang', 'skipped': True})
        report['unverified'].append('ARM64 current sources not compiled in this run')
    else:
        run('windows-arm64-clang', 'build_arm64.py', '--compile-checks', '--cmake', a.cmake)
    run('installed-package', 'verify_package.py', '--cmake', a.cmake)
    for preset, expected in (('windows-x64', 0x8664), ('windows-x86', 0x14c),
                             ('windows-x64-scalar', 0x8664)):
        folder = ROOT / 'build' / preset
        machines, members = archive_machines(folder / 'Release/CyDSP.lib')
        if machines != {expected} or not members:
            raise ValueError('Wrong COFF architecture: ' + preset)
        cache = (folder / 'CMakeCache.txt').read_text(encoding='utf-8')
        if 'CYDSP_BUILD_TESTS:BOOL=OFF' not in cache or 'CYDSP_COMPILE_CHECKS:BOOL=ON' not in cache:
            raise ValueError('Unexpected native test policy: ' + preset)
        tree = ET.parse(folder / 'CyDSP.vcxproj')
        ns = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
        for group in tree.findall('.//m:ItemDefinitionGroup', ns):
            for setting in group.findall('.//m:EnableEnhancedInstructionSet', ns):
                if 'AdvancedVector' in (setting.text or ''):
                    raise ValueError('Global AVX option leaked into baseline: ' + preset)
        for item in tree.findall('.//m:ClCompile', ns):
            for setting in item.findall('m:EnableEnhancedInstructionSet', ns):
                if 'AdvancedVector' in (setting.text or '') and not item.attrib.get('Include', '').endswith(('Avx2.cpp', 'Avx512.cpp')):
                    raise ValueError('AVX option outside private implementation: ' + preset)
        report['checks'].append({'name': preset + '-coff-policy', 'passed': True,
                                 'machine': hex(expected), 'objectMembers': members})
    if not a.skip_arm64:
        folder = ROOT / 'build/windows-arm64-clang'
        machines, members = archive_machines(folder / 'CyDSP.lib')
        if machines != {0xaa64} or not members:
            raise ValueError('Wrong ARM64 COFF architecture')
        cache = (folder / 'CMakeCache.txt').read_text(encoding='utf-8')
        if 'CYDSP_BUILD_TESTS:BOOL=OFF' not in cache or 'CYDSP_COMPILE_CHECKS:BOOL=ON' not in cache:
            raise ValueError('Unexpected ARM64 native test policy')
        ninja = (folder / 'build.ninja').read_text(encoding='utf-8')
        if any(name in ninja for name in ('Avx2.cpp.obj', 'Sse2.cpp.obj')) or 'Neon.cpp.obj' not in ninja:
            raise ValueError('Wrong ARM64 implementation sources')
        for definition in ('CYDSP_HAS_NEON=1', 'CYDSP_HAS_AVX2=0', 'CYDSP_HAS_SSE2=0'):
            if definition not in ninja:
                raise ValueError('Wrong ARM64 backend definition')
        report['checks'].append({'name': 'windows-arm64-clang-coff-policy', 'passed': True,
                                 'machine': hex(0xaa64), 'objectMembers': members})
    report['publicHeaders'] = len(list((ROOT / 'include/cydsp').rglob('*.h')))
    report['staticAssertSites'] = sum(path.read_text(encoding='utf-8').count('static_assert(')
                                    for path in (ROOT / 'tests').glob('*.cpp'))
    report['status'] = 'passed'
except Exception as error:
    report['status'] = 'failed'
    report['failure'] = str(error)
    save()
    raise
save()
print('PASS: static archives, compile contracts, package and architecture policy; runtime remains unexecuted')
print('Report: ' + str(ROOT / 'build/validation.json'))
