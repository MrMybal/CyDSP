"""Inspect compiled object code and per-file flags; never execute a CyDSP program."""
from datetime import datetime,timezone
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--arm',action='store_true');p.add_argument('--ninja',action='store_true');a=p.parse_args()
OBJDUMP=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\llvm-objdump.exe')
report={'sourceSha256':source_digest(),'timeUtc':datetime.now(timezone.utc).isoformat(),'status':'running','executesProjectCode':False,'checks':[]}
def require(value,message):
    if not value:raise RuntimeError(message)
def inspect(label,folder,arm=False,ninja=False):
    if ninja:
        text=(folder/'build.ninja').read_text(encoding='utf-8')
        objects=[];current='';flags={}
        for line in text.splitlines():
            if line.startswith('build '):current=line.replace('\\','/')
            elif line.startswith('  FLAGS =') and any('CMakeFiles/'+target+'.dir/' in current for target in ('CyDSP','CyDSPStretch','CyAudioCoreEngine','CyAudioCoreFiles','CyAudioCoreDevices','CyAudioCoreSources','CyAudioCorePlugins','CyAudioCorePluginRuntime')):
                obj=current[6:].split(': ',1)[0];flags[obj]=line
                if '-mavx512' in line:require('/Avx512.cpp.' in obj,'AVX-512 flags leaked: '+obj)
                if '+sve' in line or '-msve-vector-bits' in line:require('/Sve.cpp.' in obj,'SVE flags leaked: '+obj)
                objects.append(folder/obj)
        require(objects,'Missing object rules: '+label)
    else:objects=[p for base in (folder,folder/'cydsp') for target in ('CyDSP','CyDSPStretch','CyAudioCoreEngine','CyAudioCoreFiles','CyAudioCoreDevices','CyAudioCoreSources','CyAudioCorePlugins','CyAudioCorePluginRuntime') for p in (base/(target+'.dir/Release')).rglob('*.obj')]
    require(len(objects)>=26,'Incomplete archive objects: '+label)
    kernels=[]
    for path in objects:
        stem=path.name.split('.')[0]
        result=subprocess.run([str(OBJDUMP),'-d','--no-show-raw-insn',*(['--mattr=+sve'] if arm and 'arm64' in label else []),str(path)],cwd=ROOT,capture_output=True,text=True,check=True)
        code=result.stdout
        instructions=re.findall(r'^\s*[0-9a-f]+:\s+([a-z][a-z0-9.]*)',code,re.M)
        if not instructions:
            sections=subprocess.run([str(OBJDUMP),'-h',str(path)],cwd=ROOT,capture_output=True,text=True,check=True).stdout
            text_sections=[int(fields[2],16) for line in sections.splitlines()
                           if len(fields:=line.split())>=5 and fields[0].isdigit() and 'TEXT' in fields[4:]]
            require('Sections:' in sections and all(size==0 for size in text_sections),'Nonempty undecoded code section: '+str(path))
            continue  # A table-only or platform-disabled translation unit has no code.

        fused=[op for op in instructions if re.match(r'^(?:vfmadd|vfmsub|vfnmadd|vfnmsub|fmadd|fmsub|fnmadd|fnmsub|fmla|fmls)',op)]
        require(not fused,'Unexpected fused FP instruction: '+str(path)+' '+str(fused[:4]))
        if arm:
            scalable=bool(re.search(r'\bz\d+\.[bhsd]\b',code))
            if stem=='Sve':require(scalable,'SVE object contains no scalable-vector instructions')
            else:require(not scalable,'SVE instructions leaked: '+str(path))
        else:
            wide=bool(re.search(r'%zmm\d+\b|\bzmm\d+\b',code))
            if stem=='Avx512':require(wide,'AVX-512 object contains no ZMM instructions')
            else:require(not wide,'ZMM instructions leaked: '+str(path))
            if stem not in ['Avx512','Avx2']:require(not re.search(r'%ymm\d+\b|\bymm\d+\b',code),'YMM instructions leaked: '+str(path))
        if stem in ['Sve','Neon','Avx512','Avx2','Sse2']:
            kernels.append({'implementation':stem,'objectSha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    report['checks'].append({'target':label,'objectsInspected':len(objects),'fusedFpInstructions':0,'kernels':kernels,'isaIsolation':True})
try:
    inspect('arm64' if a.arm else 'x64',a.build_dir.resolve(),a.arm,a.ninja)
    report['status']='passed'
except Exception as error:report.update(status='failed',error=str(error))
(ROOT/'build/simd-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed':raise SystemExit(report['error'])
print('PASS: '+str(sum(c['objectsInspected'] for c in report['checks']))+' objects disassembled; wide ISA isolated; no fused FP instructions')
