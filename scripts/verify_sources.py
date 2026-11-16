"""Read-only source/provenance checks; no native code is executed or compiled."""
import ast
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
errors = []


def require(condition, message):
    if not condition:
        errors.append(message)


def project_file(relative):
    path = (ROOT / relative).resolve()
    if not path.is_relative_to(ROOT) or not path.is_file():
        raise ValueError('Missing/escaping manifest path: ' + relative)
    return path


license_text = project_file('LICENSE').read_text(encoding='utf-8')
require('MIT License' in license_text and 'Copyright (c) 2026 MrMybal CyberAlien' in license_text,
        'Missing CyDSP MIT license or ownership')
require('Permission is hereby granted' in license_text and 'THE SOFTWARE IS PROVIDED' in license_text,
        'Incomplete CyDSP MIT license')


manifest = json.loads((ROOT / 'third_party/manifest.json').read_text(encoding='utf-8'))
require(manifest['schemaVersion'] == 1, 'Unknown manifest schema')
for component in manifest['components']:
    name = component['name']
    require(component['license'] in ('MIT', 'MIT-0', 'ISC', 'BSD-3-Clause AND MIT', 'Apache-2.0 AND Zlib AND MIT AND ISC AND (GPL-2.0-or-later WITH Bison-exception-2.2)', 'ISC AND BSD-3-Clause AND LicenseRef-minimp3-Public-Domain'), 'Unexpected third-party license: ' + name)
    require(bool(re.fullmatch(r'[0-9a-f]{40}', component['revision'])), 'Unpinned revision: ' + name)
    require(bool(component['scope']), 'Missing adaptation scope: ' + name)
    notice = project_file(component['noticePath']).read_text(encoding='utf-8')
    require(('Apache License' in notice or 'Permission is hereby granted' in notice or 'Permission to use, copy, modify' in notice or 'Redistribution and use in source and binary forms' in notice) and ('THE SOFTWARE IS PROVIDED' in notice or 'THIS SOFTWARE IS PROVIDED' in notice),
            'Incomplete permissive notice: ' + name)
    if name == 'choc':
        require(all(x in notice for x in ('Josh Coalson', 'Xiph.org Foundation', 'Jutta Degener', 'public domain worldwide')), 'Missing embedded codec notice')
    for record in component['files']:
        path = project_file(record['archivePath'])
        require(hashlib.sha256(path.read_bytes()).hexdigest() == record['sha256'],
                'Changed upstream archive: ' + str(path))
        require(component['revision'] in record['url'], 'Unpinned upstream URL: ' + name)
    for relative in component['adaptedFiles']:
        source = project_file(relative).read_text(encoding='utf-8')
        for copyright_line in component['copyrights']:
            require(copyright_line in source, 'Missing original attribution: ' + relative)
        require(component['noticePath'] in source, 'Missing notice reference: ' + relative)

# Original SOFA fixtures are source assets, with no imported measurement rights.
fixtures=json.loads(project_file('tests/data/sofa/manifest.json').read_text(encoding='utf-8'))
require(fixtures['license']=='MIT', 'Unexpected fixture license')
for name,digest in fixtures['files'].items():
    file=project_file('tests/data/sofa/'+name)
    require(hashlib.sha256(file.read_bytes()).hexdigest()==digest, 'Changed synthetic SOFA fixture: '+name)

count = 0
sources = []
for folder in ('include', 'src', 'tests', 'examples'):
    for path in (ROOT / folder).rglob('*'):
        if path.suffix not in ('.h', '.cpp'):
            continue
        sources.append(path)
        count += 1
        require(b'\r' not in path.read_bytes(), 'Non-LF source: ' + str(path))
        content = path.read_text(encoding='utf-8')
        for target in re.findall(r'^#include ["<](cydsp/[^">]+|detail/[^">]+)[">]', content, re.M):
            require(any((base / target).is_file() for base in (ROOT / 'include', ROOT / 'src', path.parent)),
                    'Missing include ' + target + ' in ' + str(path))
        if path.name not in ('Avx2.cpp', 'Avx512.cpp', 'Sve.cpp', 'Sse2.cpp', 'Neon.cpp') and path not in (ROOT / 'src/detail/AdaaAvx.h', ROOT / 'src/detail/FirAvx.h', ROOT / 'src/detail/FftAvx.h', ROOT / 'src/detail/SimdX86.h', ROOT / 'src/detail/SimdNeon.h', ROOT / 'src/detail/SimdSve.h'):
            require(not any(header in content for header in ('immintrin.h', 'arm_neon.h', 'arm64_neon.h', 'arm_sve.h')),
                    'ISA intrinsics outside private kernels: ' + str(path))
        for private in ('AdaaAvx', 'FirAvx', 'FftAvx', 'SaturationSimd', 'CurveSimd', 'TileKernelBindings', 'ConvolutionSimd'):
            if '#include "detail/' + private + '.h"' in content:
                allowed = ('Avx2', 'Avx512', 'Neon', 'Sve') if private in ('CurveSimd', 'TileKernelBindings', 'ConvolutionSimd') else ('Avx2', 'Avx512')
                require(path in tuple(ROOT / ('src/math/' + name + '.cpp') for name in allowed),
                        'Private SIMD include outside ISA translation units: ' + str(path))
        if '#include "SaturationSimd.h"' in content:
            require(path == ROOT / 'src/detail/CurveSimd.h', 'Shared SIMD implementation included outside ISA kernel chain: ' + str(path))
        if any('#include "' + name + '.h"' in content for name in ('SimdX86', 'SimdNeon', 'SimdSve')):
            require(path == ROOT / 'src/detail/SaturationSimd.h', 'Private SIMD adapter included outside common kernel: ' + str(path))
        if folder == 'include':
            require('#include "detail/' not in content, 'Private include in public API: ' + str(path))

for path in (ROOT / 'scripts').glob('*.py'):
    ast.parse(path.read_text(encoding='utf-8'), filename=str(path))
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
require('add_library(CyDSP STATIC' in cmake, 'Production target must be static')
project_version = re.search(r'project\(CyDSP VERSION (\d+)\.(\d+)\.(\d+) ', cmake)
version_header = (ROOT / 'include/cydsp/core/Version.h').read_text(encoding='utf-8')
require(bool(project_version), 'Missing CMake project version')
version = project_version.groups() if project_version else ('', '', '')
for name, value in zip(('MAJOR', 'MINOR', 'PATCH'), version):
    require(('#define CYDSP_VERSION_' + name + ' ' + value + '\n') in version_header,
            'cydsp/core/Version.h does not match the CMake project version')
for name in ('VERSION.md', 'VERSION.fr.md'):
    require((ROOT / name).read_text(encoding='utf-8').startswith('# CyDSP ' + '.'.join(version) + '\n'),
            name + ' title does not match the CMake project version')
for option in ('CYDSP_BUILD_TESTS', 'CYDSP_COMPILE_CHECKS'):
    require(bool(re.search(r'option\(' + option + r' [^\n]+ OFF\)', cmake)), option + ' must be opt-in')
require('add_custom_target' not in cmake and 'POST_BUILD' not in cmake,
        'Unexpected automatic native process hook')
require('-march=native' not in cmake and '-ffast-math' not in cmake,
        'Unexpected global CPU/FP assumptions')
require('/fp:strict' in cmake and '-ffp-contract=off' in cmake, 'Missing baseline FP policy')
for line in cmake.splitlines():
    if '/arch:AVX512' in line or '-mavx512' in line:
        require('set_source_files_properties(src/math/Avx512.cpp' in line,
                'AVX-512 option must be local to its implementation')
    if '-march=armv8-a+sve' in line or '-msve-vector-bits=' in line:
        require('set_source_files_properties(src/math/Sve.cpp' in line,
                'SVE options must be local to its implementation')
    if '/arch:AVX2' in line or '-mavx2' in line:
        require('set_source_files_properties(src/math/Avx2.cpp' in line,
                'AVX2 option must be local to the AVX2 source')
for path in (ROOT / 'src').rglob('*.cpp'):
    require(path.relative_to(ROOT).as_posix() in cmake + ''.join(p.read_text() for p in (ROOT/'cmake').glob('*.cmake')), 'Implementation absent from CMake: ' + str(path))
presets = json.loads((ROOT / 'CMakePresets.json').read_text(encoding='utf-8'))
for preset in presets['configurePresets']:
    require(preset.get('cacheVariables', {}).get('CYDSP_BUILD_TESTS', 'OFF') == 'OFF',
            'Native tests enabled by preset: ' + preset['name'])
documentation = [ROOT / 'README.md', ROOT / 'README.fr.md', ROOT / 'VERSION.md', ROOT / 'VERSION.fr.md', *(ROOT / 'docs').glob('*.md')]
# Ignored local notes are not published documentation. Tracked files are never
# skipped by check-ignore; exported source packages work without Git as before.
ignored_documentation = set()
if (ROOT / '.git').exists() and shutil.which('git'):
    result = subprocess.run(['git', '--no-optional-locks', '-C', str(ROOT), '-c',
                             'safe.directory=' + ROOT.as_posix(), 'check-ignore', '-z', '--stdin'],
                            input='\0'.join(p.relative_to(ROOT).as_posix() for p in documentation) + '\0',
                            text=True, capture_output=True)
    require(result.returncode in (0, 1), 'Unable to inspect local documentation exclusions')
    ignored_documentation = set(result.stdout.split('\0'))
for path in documentation:
    if path.relative_to(ROOT).as_posix() in ignored_documentation:
        continue
    for target in re.findall(r'\[[^\]]+\]\(([^)]+)\)', path.read_text(encoding='utf-8')):
        if '://' in target or target.startswith('#'):
            continue
        require((path.parent / target.split('#')[0]).is_file(),
                'Missing documentation link: ' + target + ' in ' + str(path))
toolchain = (ROOT / 'cmake/toolchains/windows-arm64-clang.cmake').read_text(encoding='utf-8')
require('set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)' in toolchain,
        'ARM cross-build probes must use static libraries')
require(not (ROOT / '.github/workflows').exists(), 'No GitHub Actions authorized')
if errors:
    raise SystemExit('\n'.join(errors))
print(f'PASS: {count} C++ sources/headers; {len(manifest["components"])} pinned commercially compatible components; includes, Python, JSON and build policy')
