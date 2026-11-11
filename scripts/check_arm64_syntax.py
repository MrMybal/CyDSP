"""Opt-in ARM64 compile-only check with an installed Windows Clang/MSVC SDK.

No ARM binary is linked or executed; this is not ARM runtime validation.
"""
import argparse
import os
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--clang',type=Path,default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-cl.exe'))
p.add_argument('--msvc',type=Path,default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807'))
p.add_argument('--sdk',type=Path,default=Path(r'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0'))
a=p.parse_args()
include=[a.msvc/'include',*(a.sdk/name for name in ('ucrt','shared','um','winrt'))]
if not a.clang.is_file() or not all(path.is_dir() for path in include):
    p.error('Installed Clang, MSVC headers and SDK required; pass their paths explicitly')
environment=os.environ.copy()
environment['INCLUDE']=';'.join(map(str,include))
sources=[path for path in (ROOT/'src').rglob('*.cpp') if path.stem not in ('Avx2','Sse2')]
sources += list((ROOT/'tests').glob('*.cpp')) + list((ROOT/'examples').glob('*.cpp'))
for source in sources:
    args=[str(a.clang),'--target=aarch64-pc-windows-msvc','/Zs','/std:c++20','/EHsc','/W4','/fp:strict',
        '/I'+str(ROOT/'include'),'/I'+str(ROOT/'src'),'/DCYDSP_HAS_SSE2=0','/DCYDSP_HAS_AVX2=0','/DCYDSP_HAS_NEON=1',str(source)]
    result=subprocess.run(args,cwd=ROOT,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    if result.returncode or 'warning:' in result.stdout:
        print(result.stdout)
        raise SystemExit(result.returncode or 1)
print(f'PASS: ARM64 Clang syntax/constexpr checks for {len(sources)} sources; no linking or execution')
