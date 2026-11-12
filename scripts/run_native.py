"""Explicit opt-in native regression/measurement runs; production presets stay unchanged."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
from reporting import source_digest
import platform
import subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler',choices=['msvc','clang'],default='msvc')
p.add_argument('--arch',choices=['x64','x86','scalar'],default='x64')
p.add_argument('--build-dir',type=Path)
p.add_argument('--measure',action='store_true')
p.add_argument('--sanitize',action='store_true')
p.add_argument('--cmake',default=r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
a=p.parse_args()
tag=a.arch+('-clang' if a.compiler=='clang' else '')+('-asan' if a.sanitize else '')
folder=ROOT/'build'/('native-'+tag+('-ninja' if a.compiler=='clang' else ''))
if a.build_dir: folder=a.build_dir.resolve()
logs=ROOT/'build/native-logs';logs.mkdir(parents=True,exist_ok=True)
report={'sourceSha256':source_digest(),'compilerToolset':a.compiler,'timeUtc':datetime.now(timezone.utc).isoformat(),'architecture':a.arch,'addressSanitizer':a.sanitize,'system':platform.platform(),'status':'running'}
try:
    import winreg
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:
        report['cpu']=winreg.QueryValueEx(key,'ProcessorNameString')[0].strip()
except (ImportError,OSError): report['cpu']=platform.processor()
environment=os.environ.copy()
if a.sanitize:
    runtime=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/bin/Hostx64')/('x86' if a.arch=='x86' else 'x64')
    environment['PATH']=str(runtime)+os.pathsep+environment.get('PATH','')

def run(name,command,timeout=240):
    result=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=timeout,env=environment)
    (logs/(tag+'-'+name+'.log')).write_text(result.stdout,encoding='utf-8')
    if result.returncode:
        raise RuntimeError(name+' exit '+str(result.returncode)+' ('+hex(result.returncode & 0xffffffff)+'): '+result.stdout[-6000:])
    return result.stdout
try:
    options=['-DCYDSP_ENABLE_SIMD='+('OFF' if a.arch=='scalar' else 'ON'),'-DCYDSP_BUILD_TESTS=ON','-DCYDSP_COMPILE_CHECKS=ON',
        '-DCYDSP_BUILD_MEASUREMENTS='+('ON' if a.measure else 'OFF'),'-DCYDSP_SANITIZE_ADDRESS='+('ON' if a.sanitize else 'OFF')]
    if a.compiler=='clang':
        vs=Path('C:/Program Files/Microsoft Visual Studio/2022/Community')
        llvm=vs/'VC/Tools/Llvm/x64/bin';msvc=vs/'VC/Tools/MSVC/14.40.33807'
        sdk=Path('C:/Program Files (x86)/Windows Kits/10');version='10.0.26100.0'
        target='x86' if a.arch=='x86' else 'x64'
        environment['INCLUDE']=';'.join(str(p) for p in [msvc/'include',*[sdk/'Include'/version/name for name in ['ucrt','shared','um','winrt']]])
        environment['LIB']=';'.join(str(p) for p in [msvc/'lib'/target,sdk/'Lib'/version/'ucrt'/target,sdk/'Lib'/version/'um'/target])
        environment['PATH']=';'.join(str(p) for p in [llvm,msvc/'bin/Hostx64'/target,sdk/'bin'/version/'x64'])+';'+environment.get('PATH','')
        ninja=vs/'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
        command=[a.cmake,'-S',str(ROOT),'-B',str(folder),'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+str(ninja),
            '-DCMAKE_C_COMPILER='+str(llvm/'clang-cl.exe'),'-DCMAKE_C_COMPILER_TARGET='+('i686-pc-windows-msvc' if a.arch=='x86' else 'x86_64-pc-windows-msvc'),'-DCMAKE_CXX_COMPILER='+str(llvm/'clang-cl.exe'),'-DCMAKE_LINKER='+str(llvm/'lld-link.exe'),
            '-DCMAKE_AR='+str(llvm/'llvm-lib.exe'),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL',
            '-DCMAKE_CXX_COMPILER_TARGET='+('i686' if target=='x86' else 'x86_64')+'-pc-windows-msvc']+options
        executable=folder
    else:
        command=[a.cmake,'-S',str(ROOT),'-B',str(folder),'-G','Visual Studio 17 2022','-A','Win32' if a.arch=='x86' else 'x64']+options
        executable=folder/'Release'
    run('configure',command)
    run('build',[a.cmake,'--build',str(folder),'--config','Release','--parallel','6'])
    report['regressions']=run('regressions',[str(executable/'cydsp_tests.exe')])
    if a.measure:
        report['measurements']=json.loads(run('measurements',[str(executable/'cydsp_measurements.exe')],600))
    report['status']='passed'
except Exception as error:
    report['status']='failed';report['error']=str(error)
finally:
    (ROOT/'build'/('native-'+tag+'.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed': raise SystemExit(report['error'])
print('PASS: native '+a.arch+' regressions'+(' and measurements' if a.measure else ''))
print('Report: '+str(ROOT/'build'/('native-'+tag+'.json')))
