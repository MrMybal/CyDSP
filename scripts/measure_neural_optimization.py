# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Explicit CPU-only neural rendering benchmark; no training or GPU dependency."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[1]
CMAKE = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--library', type=Path, default=ROOT / 'build/windows-x64/Release/CyDSP.lib')
    parser.add_argument('--baseline-library', type=Path)
    parser.add_argument('--baseline-include', type=Path)
    parser.add_argument('--cmake', type=Path, default=CMAKE)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if bool(args.baseline_library) != bool(args.baseline_include):
        parser.error('Provide both frozen baseline library and include directory')
    model, library = args.model.resolve(), args.library.resolve()
    if not model.is_file() or not library.is_file():
        parser.error('Model and library must exist')
    out = (args.output or ROOT / 'build/neural-optimization' /
           (datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8])).resolve()
    if not out.is_relative_to((ROOT / 'build').resolve()):
        parser.error('Keep benchmark output under the ignored build directory')
    out.mkdir(parents=True, exist_ok=False)
    source = ROOT / 'scripts/native/NeuralOptimization.cpp'
    target_paths = {'after': (ROOT / 'include', library)}
    if args.baseline_library:
        target_paths['before'] = (args.baseline_include.resolve(), args.baseline_library.resolve())
    cmake = ['cmake_minimum_required(VERSION 3.24)', 'project(NeuralOptimization LANGUAGES CXX)']
    for name, (includes, archive) in target_paths.items():
        if not includes.is_dir() or not archive.is_file():
            parser.error('Frozen baseline files must exist')
        cmake += [f'add_executable({name} "{source.as_posix()}")',
                  f'target_compile_features({name} PRIVATE cxx_std_20)',
                  f'target_include_directories({name} PRIVATE "{includes.as_posix()}")',
                  f'target_link_libraries({name} PRIVATE "{archive.as_posix()}")',
                  'if(MSVC)', f' target_compile_options({name} PRIVATE /W4 /fp:strict)', 'endif()']
        if name == 'after':
            cmake.append('target_compile_definitions(after PRIVATE CYDSP_NEURAL_BATCH=1)')
    (out / 'CMakeLists.txt').write_text('\n'.join(cmake) + '\n', encoding='utf-8', newline='\n')
    subprocess.run([str(args.cmake), '-S', str(out), '-B', str(out / 'cmake-build'), '-A', 'x64'], check=True)
    subprocess.run([str(args.cmake), '--build', str(out / 'cmake-build'), '--config', 'Release'], check=True)
    reports = {}
    for name in ('before', 'after'):
        if name not in target_paths:
            continue
        exe = out / f'cmake-build/Release/{name}.exe'
        result = subprocess.run([str(exe), str(model)], check=True, capture_output=True, text=True)
        report = json.loads(result.stdout)
        rows = report['measurements']
        if len(rows) != (18 if name == 'after' else 3) or report['sampleRate'] != 48000:
            raise ValueError('Unexpected benchmark output')
        for row in rows:
            for key in ('staticUs', 'movingPreparedUs', 'movingTotalUs'):
                if not 0 < row[key] < 1e9:
                    raise ValueError('Invalid timing')
        report.update(executableSha256=sha(exe), librarySha256=sha(target_paths[name][1]))
        reports[name] = report
        (out / (name + '.json')).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    metadata = {'modelSha256': sha(model), 'sourceSha256': sha(source), 'system': platform.platform(),
                'cpu': platform.processor(), 'singleThread': True, 'gpu': False,
                'statistic': 'Median of 9 series means, 96 calls each after 24 warm-up blocks; not block p95/max.',
                'motion': '62.5 Hz direction updates, 256-frame fades, 128-frame audio at 48 kHz.',
                'columns': {'staticUs': 'Audio rendering and stereo mix per block.',
                            'movingPreparedUs': 'Rendering/mix/fades and installation/replay of prepared designs per block.',
                            'movingTotalUs': 'Also includes model prediction and design; total CPU even if those run on a worker.'},
                'history': '0 = legacy cold transition; 512 = default replay; 2048 = longer replay. Static transfer unchanged; transition behavior and costs differ.',
                'scope': 'Same existing model weights; no training. Finite native outputs checked. No device deadline or subjective quality guarantee.'}
    (out / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
    (out / 'benchmark-source.cpp').write_bytes(source.read_bytes())
    (out / 'driver.py').write_bytes(Path(__file__).read_bytes())
    lines = ['# Native neural rendering optimization', '',
             'Microseconds of CPU time per 128-frame block at 48 kHz; smaller is faster.',
             'Moving sources: 62.5 Hz updates and 256-frame fades. Total includes prediction/design,',
             'even when a host moves that work to its worker. Prepared excludes prediction/design.', '',
             '| Version | Voices | Batch | Replay frames | Static us | Moving prepared us | Moving total us |',
             '| --- | ---: | --- | ---: | ---: | ---: | ---: |']
    for name, report in reports.items():
        for row in report['measurements']:
            lines.append(f"| {name} | {row['voices']} | {row['batch']} | {row['historyFrames']} | {row['staticUs']:.3f} | {row['movingPreparedUs']:.3f} | {row['movingTotalUs']:.3f} |")
    lines += ['', 'No training or changed model weights. Replaying finite input history improves',
              'transition initialization but does not reconstruct an infinite IIR past.',
              'The zero-replay rows preserve the legacy cold-transition policy; the default',
              '512-frame and optional 2048-frame rows include extra work for different transitions.',
              'Series means are not per-block worst cases. Hardware, hashes and timing semantics',
              'are recorded in manifest.json. No library can promise superiority for every workload.']
    (out / 'Report.md').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'directory': str(out), 'reports': reports}, indent=2))


if __name__ == '__main__':
    main()
