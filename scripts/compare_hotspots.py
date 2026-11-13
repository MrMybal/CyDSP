"""Explicit alternating comparison of two opt-in hotspot measurement executables."""
import argparse
import array
import ctypes
import hashlib
import json
import math
from pathlib import Path
import statistics
import subprocess
from reporting import source_digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--before', type=Path, required=True)
p.add_argument('--after', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--trials', type=int, default=3)
p.add_argument('--logical-cpu', type=int)
p.add_argument('--before-render', type=Path)
p.add_argument('--after-render', type=Path)
a = p.parse_args()
if a.trials < 3:
    p.error('at least three process pairs are required')
runs = {'before': [], 'after': []}
for number in range(a.trials):
    for label in (('before', 'after') if number % 2 == 0 else ('after', 'before')):
        executable = getattr(a, label).resolve()
        process = subprocess.Popen([str(executable)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if a.logical_cpu is not None:
            if not hasattr(ctypes, 'windll'):
                process.terminate()
                p.error('per-process affinity is supported here only on Windows')
            if not ctypes.windll.kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(process._handle)), ctypes.c_size_t(1 << a.logical_cpu)):
                process.terminate()
                raise OSError('unable to set measurement process affinity')
        out, err = process.communicate(timeout=180)
        if process.returncode:
            raise RuntimeError(label + ': ' + err)
        entries = json.loads(out)
        assert len({x['case'] for x in entries}) == len(entries)
        assert all(math.isfinite(x['medianBatchMeanUs']) and x['medianBatchMeanUs'] > 0 for x in entries)
        runs[label].append({x['case']: x for x in entries})
assert all(x.keys() == runs['before'][0].keys() for group in runs.values() for x in group)
rows = []
for name in runs['before'][0]:
    values = {label: [x[name]['medianBatchMeanUs'] for x in runs[label]] for label in runs}
    before, after = statistics.median(values['before']), statistics.median(values['after'])
    rows.append({'case': name, 'beforeUs': before, 'afterUs': after,
                 'speedupBeforeOverAfter': before / after,
                 'costChangePercent': (after / before - 1) * 100, 'processMeansUs': values})
report = {'status': 'passed', 'sourceSha256': source_digest(),
          'baselineProductionCommit': 'fda6bd30c5b622795cc08e8bd2750188a6c5a0c0',
          'method': 'Alternating independent process pairs; median of process medians of nine 128-call batch means, 256-frame blocks at 48 kHz. No global CPU/FP state changes.',
          'logicalCpu': a.logical_cpu, 'processPairs': a.trials,
          'executablesSha256': {label: hashlib.sha256(getattr(a, label).read_bytes()).hexdigest() for label in runs}, 'cases': rows}
if bool(a.before_render) != bool(a.after_render):
    p.error('supply both render files')
if a.before_render:
    arrays = []
    for path in (a.before_render, a.after_render):
        data = array.array('d'); data.frombytes(path.read_bytes()); arrays.append(data)
    assert len(arrays[0]) == len(arrays[1]) and all(math.isfinite(x) for data in arrays for x in data)
    report['renderComparison'] = {'samples': len(arrays[0]),
        'maxAbsoluteError': max(abs(x-y) for x, y in zip(*arrays)),
        'sha256': [hashlib.sha256(path.read_bytes()).hexdigest() for path in (a.before_render, a.after_render)]}
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('PASS: ' + str(len(rows)) + ' component cases, ' + str(a.trials) + ' alternating process pairs')
