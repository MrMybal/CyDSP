"""Production-only local checks; no project test executable is launched."""
import argparse,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--preset',default='windows-x64');p.add_argument('--cmake');a=p.parse_args()
subprocess.run([sys.executable,'-B',str(ROOT/'scripts/verify_sources.py')],check=True,cwd=ROOT)
command=[sys.executable,'-B',str(ROOT/'scripts/build.py'),'--preset',a.preset,'--compile-checks']
if a.cmake:command+=['--cmake',a.cmake]
subprocess.run(command,check=True,cwd=ROOT)
print('PASS source/provenance and production static/compile checks; no native runtime')
