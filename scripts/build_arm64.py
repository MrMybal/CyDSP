"""Cross-compile ARM64 static libraries and optional compile-only checks; no target execution."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cmake', type=Path, default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'))
p.add_argument('--llvm', type=Path, default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin'))
p.add_argument('--ninja', type=Path, default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'))
p.add_argument('--msvc', type=Path, default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807'))
p.add_argument('--sdk', type=Path, default=Path(r'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0'))
p.add_argument('--build-dir', type=Path)
p.add_argument('--compile-checks', action='store_true')
a = p.parse_args()
include = [a.msvc / 'include', *(a.sdk / name for name in ('ucrt', 'shared', 'um', 'winrt'))]
compiler = a.llvm / 'clang-cl.exe'
archiver = a.llvm / 'llvm-lib.exe'
linker = a.llvm / 'lld-link.exe'
if not all(path.is_file() for path in (a.cmake, a.ninja, compiler, archiver, linker)) or not all(path.is_dir() for path in include):
    p.error('Installed LLVM/CMake/Ninja and MSVC/SDK headers required; pass paths explicitly')
environment = os.environ.copy()
environment['INCLUDE'] = ';'.join(map(str, include))
folder = a.build_dir.resolve() if a.build_dir else ROOT / 'build/windows-arm64-clang'
configure = [str(a.cmake), '-S', str(ROOT), '-B', str(folder), '-G', 'Ninja',
             '-DCMAKE_CXX_COMPILER=' + str(compiler), '-DCMAKE_AR=' + str(archiver),
             '-DCMAKE_LINKER=' + str(linker), '-DCMAKE_MAKE_PROGRAM=' + str(a.ninja),
             '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL',
             '-DCYDSP_BUILD_TESTS=OFF', '-DCYDSP_BUILD_MEASUREMENTS=OFF', '-DCYDSP_SANITIZE_ADDRESS=OFF', '-DCYDSP_ENABLE_SIMD=ON',
             '-DCYDSP_COMPILE_CHECKS=' + ('ON' if a.compile_checks else 'OFF')]
toolchain = ROOT / 'cmake/toolchains/windows-arm64-clang.cmake'
cache = folder / 'CMakeCache.txt'
if cache.is_file():
    entries = [line.split('=', 1)[1] for line in cache.read_text(encoding='utf-8').splitlines()
               if line.startswith(('CMAKE_TOOLCHAIN_FILE:FILEPATH=', 'CMAKE_TOOLCHAIN_FILE:UNINITIALIZED='))]
    if len(entries) != 1 or Path(entries[0]).resolve() != toolchain.resolve():
        p.error('Existing ARM64 build uses another toolchain; choose a fresh build directory')
else:
    # CMake consumes this option on the first configuration, not incremental reruns.
    configure.append('-DCMAKE_TOOLCHAIN_FILE:FILEPATH=' + str(toolchain))
subprocess.run(configure, cwd=ROOT, env=environment, check=True)
subprocess.run([str(a.cmake), '--build', str(folder), '--parallel'], cwd=ROOT, env=environment, check=True)
print('PASS: ARM64 clang-cl/NEON static compilation; no application executable linking or execution')
