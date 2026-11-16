"""Opt-in production install and independent native consumer validation."""
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
CMAKE=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
folder=ROOT/'build/advanced-package'
production=folder/'production';prefix=folder/'install';consumer=folder/'consumer'
folder.mkdir(parents=True,exist_ok=True);consumer.mkdir(exist_ok=True)
report={'sourceSha256':source_digest(),'timeUtc':datetime.now(timezone.utc).isoformat(),'status':'running','productionTestsEnabled':False}
def run(name,command):
    result=subprocess.run(list(map(str,command)),cwd=ROOT,capture_output=True,text=True,timeout=240)
    (folder/(name+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
    if result.returncode:raise RuntimeError(name+' failed: '+(result.stdout+result.stderr)[-4000:])
    return result.stdout
try:
    run('configure',[CMAKE,'-S',ROOT,'-B',production,'-G','Visual Studio 17 2022','-A','x64','-DCYDSP_BUILD_TESTS=OFF','-DCYDSP_BUILD_MEASUREMENTS=OFF','-DCYDSP_COMPILE_CHECKS=OFF','-DCMAKE_INSTALL_PREFIX='+str(prefix)])
    run('build',[CMAKE,'--build',production,'--config','Release','--parallel','6'])
    cache=(production/'CMakeCache.txt').read_text(encoding='utf-8')
    assert 'CYDSP_BUILD_TESTS:BOOL=OFF' in cache and 'CYDSP_BUILD_MEASUREMENTS:BOOL=OFF' in cache
    assert not list(production.rglob('cydsp_tests.exe')) and not list(production.rglob('cydsp_measurements.exe'))
    run('install',[CMAKE,'--install',production,'--config','Release'])
    assert (prefix/'share/CyDSP/LICENSE').is_file()
    assert {p.name for p in (prefix/'share/CyDSP/licenses').glob('*.txt')} == {p.name for p in (ROOT/'third_party/licenses').glob('*.txt')}
    (consumer/'CMakeLists.txt').write_text("""cmake_minimum_required(VERSION 3.24)
project(AdvancedCyDSPConsumer LANGUAGES CXX)
find_package(CyDSP 0.4 CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE CyDSP::CyDSP)
""",encoding='utf-8')
    (consumer/'main.cpp').write_text(r"""#include <cydsp/CyDSP.h>
#include <array>
#include <iostream>
int main(){using namespace cydsp;QualitySettings q;if(qualitySettings(QualityProfile::realtimeHigh,8000,q)!=Status::ok)return 1;
AudioBuffer<float> input,out,h,p;for(auto* b:{&input,&out,&h,&p})if(b->resize(1,64)!=Status::ok)return 2;
SourceBatch<float> batch;if(batch.prepare({8000,64,1},16)!=Status::ok||batch.start(0,{})!=Status::ok||batch.process(input.block())!=Status::ok)return 3;
SpectralPitchShifter<float> pitch;if(pitch.prepare({8000,64,1},{128,32,WindowType::hann})!=Status::ok||pitch.process(input.block(),out.block())!=Status::ok)return 4;
HarmonicPercussive<float> split;if(split.prepare({8000,64,1},{{128,32,WindowType::hann},7,7})!=Status::ok||split.process(input.block(),h.block(),p.block())!=Status::ok)return 5;
ContactExciter<float> contact;if(contact.prepare({8000,64,1})!=Status::ok||contact.strike(.001)!=Status::ok||contact.render(out.block())!=Status::ok)return 6;
EchoCanceller<float> echo;if(echo.prepare({8000,64,1},{16,0,10})!=Status::ok||echo.process(input.block(),input.block(),out.block())!=Status::ok)return 7;
std::array<float,8> ir{1,0,0,0,0,0,0,0};std::array<HrirPoint<float>,1> points{{{{1,0,0},ir,ir}}};HrtfSet<float> set;if(set.prepare(points,q.hrtf)!=Status::ok)return 8;
HoaBinaural<float> hoa;AudioBuffer<float> stereo;if(stereo.resize(2,64)!=Status::ok||hoa.prepare({8000,64,1},0,set)!=Status::ok||hoa.process(input.block(),stereo.block())!=Status::ok)return 9;
std::array<double,64> coefficients{};if(ambisonicCoefficients(7,{1,0,0},coefficients)!=Status::ok)return 10;
DiffractionFilter<float> diffraction;if(diffraction.prepare({8000,64,1})!=Status::ok||diffraction.setPath({})!=Status::ok||diffraction.process(input.block(),out.block())!=Status::ok)return 11;
ImpulseReport report;if(analyzeImpulse(ir,8000,report)!=Status::ok)return 12;
std::cout<<"PASS installed advanced CyDSP consumer\n";}
""",encoding='utf-8')
    run('consumer-configure',[CMAKE,'-S',consumer,'-B',consumer/'build','-G','Visual Studio 17 2022','-A','x64','-DCMAKE_PREFIX_PATH='+str(prefix)])
    run('consumer-build',[CMAKE,'--build',consumer/'build','--config','Release','--parallel','6'])
    report['consumerOutput']=run('consumer-run',[consumer/'build/Release/consumer.exe'])
    report['status']='passed'
except Exception as error:report.update(status='failed',error=str(error))
(ROOT/'build/advanced-install-check.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed':raise SystemExit(report['error'])
print('PASS: production static install + advanced independent consumer')
