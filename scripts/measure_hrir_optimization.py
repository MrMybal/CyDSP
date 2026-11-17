"""Explicit CPU-only HRIR benchmark; no training dependencies or GPU access."""
import argparse, datetime, hashlib, json, platform, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus',type=Path,required=True,help='corpus.bin produced by validate_hrtf_rendering.py at 48 kHz')
    parser.add_argument('--library',type=Path,default=ROOT/'build/windows-x64/Release/CyDSP.lib')
    parser.add_argument('--cmake',type=Path,default=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'))
    parser.add_argument('--output',type=Path)
    parser.add_argument('--neighbours',type=int,default=0)
    parser.add_argument('--fractional-taps',type=int,default=0)
    args=parser.parse_args()
    corpus=args.corpus.resolve();library=args.library.resolve()
    if not corpus.is_file() or not library.is_file(): parser.error('Corpus and library must exist')
    out=(args.output or ROOT/'build/hrir-optimization'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')).resolve()
    out.mkdir(parents=True,exist_ok=False)
    source=ROOT/'scripts/native/HrirOptimization.cpp'
    (out/'CMakeLists.txt').write_text(f'cmake_minimum_required(VERSION 3.24)\nproject(HrirOptimization LANGUAGES CXX)\nadd_executable(hrir-optimization "{source.as_posix()}")\ntarget_compile_features(hrir-optimization PRIVATE cxx_std_20)\ntarget_include_directories(hrir-optimization PRIVATE "{(ROOT/"include").as_posix()}")\ntarget_link_libraries(hrir-optimization PRIVATE "{library.as_posix()}")\nif(MSVC)\n target_compile_options(hrir-optimization PRIVATE /W4 /fp:precise)\nendif()\n')
    subprocess.run([str(args.cmake),'-S',str(out),'-B',str(out/'cmake-build'),'-A','x64'],check=True)
    subprocess.run([str(args.cmake),'--build',str(out/'cmake-build'),'--config','Release'],check=True)
    result=subprocess.run([str(out/'cmake-build/Release/hrir-optimization.exe'),str(corpus),str(args.neighbours),str(args.fractional_taps)],check=True,capture_output=True,text=True)
    report=json.loads(result.stdout)
    report.update(cpu=platform.processor(),system=platform.platform(),sourceSha256=sha(source),librarySha256=sha(library),corpusSha256=sha(corpus))
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/'source.cpp').write_bytes(source.read_bytes())
    (out/'driver.py').write_bytes(Path(__file__).read_bytes())
    print(json.dumps(report,indent=2)); print(out)
if __name__=='__main__': main()
