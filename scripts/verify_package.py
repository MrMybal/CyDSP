"""Install and compile a static CMake consumer; no project executable is run."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cmake', default=r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
a = p.parse_args()
stage = ROOT / 'build/stage'
consumer = ROOT / 'build/package-consumer'
consumer.mkdir(parents=True, exist_ok=True)
subprocess.run([a.cmake, '--install', str(ROOT / 'build/windows-x64'), '--config', 'Release',
                '--prefix', str(stage)], cwd=ROOT, check=True)
(consumer / 'CMakeLists.txt').write_text(
    'cmake_minimum_required(VERSION 3.24)\nproject(CyDSPConsumer LANGUAGES CXX)\n'
    'find_package(CyDSP 0.1 CONFIG REQUIRED)\nadd_library(Consumer STATIC Consumer.cpp)\n'
    'target_link_libraries(Consumer PRIVATE CyDSP::CyDSP)\n', encoding='utf-8')
(consumer / 'Consumer.cpp').write_text(
    '#include <cydsp/CyDSP.h>\n#include <array>\n'
    'cydsp::Status prepareConsumer() {\n'
    '  const cydsp::ProcessSpec spec{48000,512,2};\n'
    '  const std::array<float,3> impulse{1,0.5f,0.25f};\n'
    '  cydsp::PartitionedConvolver<float> convolution;\n'
    '  auto status=convolution.prepare(spec,impulse,128);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::HybridConvolver<float> hybrid;status=hybrid.prepare(spec,impulse,2);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::Fir<float> fir;status=fir.prepare(spec,impulse);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::DelayLine<float> delay;status=delay.prepare(spec,48000);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::RealFft<double> fft;status=fft.prepare(256);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::Resampler<float> converter;status=converter.prepare({48000,44100,2,512,512});\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::BiquadCoefficients eq;status=cydsp::designPeakingBiquad(48000,1000,0.707,6,eq);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::BiquadBank<float> voices;status=voices.prepare(spec);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=voices.setCoefficients(0,eq);if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::SmoothedBiquad<float> smooth;status=smooth.prepare(spec);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=smooth.setCoefficients(eq,128);if(status!=cydsp::Status::ok)return status;\n'
    '  std::array<float,4> left{},right{};std::array<float,8> flat{};\n'
    '  float* channels[]{left.data(),right.data()};\n'
    '  status=cydsp::interleave({channels,2,4},flat);if(status!=cydsp::Status::ok)return status;\n'
    '  status=cydsp::deinterleave(flat,{channels,2,4});if(status!=cydsp::Status::ok)return status;\n'
    '  const std::array<double,4> mix{0.5,0.5,0.5,0.5};\n'
    '  cydsp::ChannelMatrix<float> matrix;status=matrix.prepare(spec,2,mix);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=matrix.setMatrix(mix,64);if(status!=cydsp::Status::ok)return status;\n'
    '  status=matrix.process({channels,2,4},{channels,2,4});\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::SmoothedConvolver<float> ir;status=ir.prepare(spec,impulse,2);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=ir.stageImpulse(impulse);if(status!=cydsp::Status::ok)return status;\n'
    '  status=ir.startTransition(8);if(status!=cydsp::Status::ok)return status;\n'
    '  status=ir.process({channels,2,4},{channels,2,4});\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::SmoothedBiquadCascade<float> fullEq;status=fullEq.prepare(spec,4);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=fullEq.setCoefficients(std::span{&eq,1},128);if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::NonuniformConvolver<float> nonuniform;status=nonuniform.prepare(spec,impulse,2,8);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  const cydsp::ConvolutionConfig config{cydsp::ConvolutionMode::scheduled,2,8};\n'
    '  cydsp::Convolver<float> selected;status=selected.prepare(spec,impulse,config);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=selected.process({channels,2,4},{channels,2,4});if(status!=cydsp::Status::ok)return status;\n'
    '  status=ir.prepare(spec,impulse,config);if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::ScheduledConvolver<float> scheduled;status=scheduled.prepare(spec,impulse,2,8);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=scheduled.process({channels,2,4},{channels,2,4});if(status!=cydsp::Status::ok)return status;\n'
    '  const std::array<cydsp::ConvolutionRoute<float>,1> routes{{{0,1,impulse}}};\n'
    '  cydsp::MatrixConvolver<float> mimo;status=mimo.prepare(spec,2,routes,config);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=mimo.process({channels,2,4},{channels,2,4});if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::AsyncConvolver<float> async;status=async.prepare(spec,impulse,config);\n'
    '  if(status!=cydsp::Status::ok)return status;\n'
    '  status=async.stageImpulse(impulse,8);if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::AudioGraph<float> graph;const std::array<cydsp::GraphInput,1> pass{{{cydsp::graphExternalInput,1}}};\n'
    '  status=graph.prepare(spec,{},pass);if(status!=cydsp::Status::ok)return status;\n'
    '  status=converter.setRates(48000,32000);if(status!=cydsp::Status::ok)return status;\n'
    '  status=converter.finish({channels,2,4}).status;if(status!=cydsp::Status::ok)return status;\n'
    '  cydsp::BiquadCascade<float> cascade;return cascade.prepare(spec,std::span{&eq,1});\n'
    '}\n', encoding='utf-8')
subprocess.run([a.cmake, '-S', str(consumer), '-B', str(consumer / 'build'), '-G',
                'Visual Studio 17 2022', '-A', 'x64', '-DCMAKE_PREFIX_PATH=' + str(stage)],
               cwd=ROOT, check=True)
subprocess.run([a.cmake, '--build', str(consumer / 'build'), '--config', 'Release'], cwd=ROOT, check=True)
for relative in ('lib/CyDSP.lib', 'lib/cmake/CyDSP/CyDSPConfig.cmake', 'include/cydsp/CyDSP.h',
                 'share/CyDSP/manifest.json', 'share/CyDSP/Licensing.md'):
    if not (stage / relative).is_file():
        raise SystemExit('Missing installed artifact: ' + relative)
manifest = json.loads((ROOT / 'third_party/manifest.json').read_text(encoding='utf-8'))
for component in manifest['components']:
    notice = ROOT / component['noticePath']
    if notice.read_bytes() != (stage / 'share/CyDSP/licenses' / notice.name).read_bytes():
        raise SystemExit('Changed installed notice: ' + notice.name)
for source in (ROOT / 'include').rglob('*.h'):
    installed = stage / 'include' / source.relative_to(ROOT / 'include')
    if source.read_bytes() != installed.read_bytes():
        raise SystemExit('Changed installed header: ' + str(installed))
if (ROOT / 'third_party/manifest.json').read_bytes() != (stage / 'share/CyDSP/manifest.json').read_bytes():
    raise SystemExit('Changed installed manifest')
if (ROOT/'docs/Licensing.md').read_bytes() != (stage/'share/CyDSP/Licensing.md').read_bytes():
    raise SystemExit('Changed installed licensing/provenance document')
targets = (stage / 'lib/cmake/CyDSP/CyDSPTargets.cmake').read_text(encoding='utf-8')
if ROOT.as_posix().lower() in targets.replace('\\', '/').lower():
    raise SystemExit('Source checkout path leaked into installed target')
print('PASS: staged CMake package, all MIT notices and static consumer compilation; no executable link/runtime check')
