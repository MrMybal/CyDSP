"""Cross-build Android archives, public headers and native test ELF files; no device execution."""
import argparse
from android_paths import android_ndk
import json
from pathlib import Path
from reporting import source_digest
import subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--measure',action='store_true')
p.add_argument('--abi',choices=['armeabi-v7a','arm64-v8a','x86','x86_64'],default='arm64-v8a')
p.add_argument('--ndk',type=Path,default=android_ndk())
p.add_argument('--cmake',default=r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
p.add_argument('--ninja',default=r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
a=p.parse_args();folder=ROOT/'build'/('android-'+a.abi);folder.mkdir(parents=True,exist_ok=True)
report={'sourceSha256':source_digest(),'abi':a.abi,'ndk':str(a.ndk),'status':'running','targetRuntimeExecuted':False}
commands=[('configure',[a.cmake,'-S',str(ROOT),'-B',str(folder),'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+a.ninja,
    '-DCMAKE_TOOLCHAIN_FILE='+str(a.ndk/'build/cmake/android.toolchain.cmake'),'-DANDROID_ABI='+a.abi,'-DANDROID_PLATFORM=android-24',
    '-DANDROID_STL=c++_static','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_TRY_COMPILE_TARGET_TYPE=EXECUTABLE','-UCMAKE_HAVE_*PTHREAD*',
    '-DCYDSP_BUILD_TESTS=ON','-DCYDSP_COMPILE_CHECKS=ON','-DCYDSP_ENABLE_SIMD=ON','-DCYDSP_BUILD_MEASUREMENTS='+('ON' if a.measure else 'OFF')]),
    ('build',[a.cmake,'--build',str(folder),'--parallel','6'])]
try:
    for name,command in commands:
        result=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=600)
        (folder/(name+'.log')).write_text(result.stdout,encoding='utf-8')
        if result.returncode:raise RuntimeError(name+': '+result.stdout[-7000:])
    data=(folder/'cydsp_tests').read_bytes()
    if data[:4]!=b'\x7fELF':raise RuntimeError('Missing target ELF executable')
    machine=int.from_bytes(data[18:20],'little')
    if machine!={'armeabi-v7a':40,'arm64-v8a':183,'x86':3,'x86_64':62}[a.abi]:raise RuntimeError('Wrong ELF architecture')
    report.update(status='passed',elfMachine=machine,linkedTests=True)
except Exception as error:report.update(status='failed',error=str(error))
(folder/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed':raise SystemExit(report['error'])
print('PASS: Android '+a.abi+' library, public headers and regression executable link; no target runtime')
