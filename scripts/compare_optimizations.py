"""Explicit before/after executable comparison, alternating trial order; no build or downloads."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
from reporting import source_digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--before', type=Path, required=True)
p.add_argument('--after', type=Path, required=True)
p.add_argument('--baseline-report', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--trials', type=int, default=6)
a = p.parse_args()
if a.trials < 3:
    p.error('at least three trials are required')
root = Path(__file__).resolve().parents[1]
baseline = json.loads(a.baseline_report.read_text(encoding='utf-8'))
if baseline.get('status') != 'passed':
    p.error('baseline must be a passed native report')
report = {
    'timeUtc': datetime.now(timezone.utc).isoformat(), 'status': 'running',
    'cpu': baseline.get('cpu'), 'system': baseline.get('system'),
    'baselineSourceSha256': baseline['sourceSha256'], 'sourceSha256': source_digest(),
    'harnessSha256': hashlib.sha256((root/'tests/FocusedMeasurements.cpp').read_bytes()).hexdigest(),
    'executableSha256': {name: hashlib.sha256(path.read_bytes()).hexdigest()
                        for name, path in [('before', a.before), ('after', a.after)]},
    'method': 'Alternating before/after order; identical shuffled case seed per pair. '
              '51 batch-average samples/case; median of trial medians. No affinity/frequency/OS changes.',
    'speedupDefinition': 'beforeMedianNs / afterMedianNs; greater than 1 is faster',
    'runs': [], 'comparisons': []}
try:
    for trial in range(a.trials):
        order = [('before', a.before), ('after', a.after)]
        if trial % 2:
            order.reverse()
        for name, executable in order:
            command = [str(executable.resolve()), str(trial + 1)]
            result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=180)
            if result.returncode:
                raise RuntimeError(f'{name} trial {trial}: {result.stderr[-3000:]}')
            rows = json.loads(result.stdout)
            if not rows:
                raise RuntimeError('empty measurement output')
            report['runs'].append({'version': name, 'trial': trial, 'records': rows})
            print(f'PASS {name} trial {trial+1}: {len(rows)} cases', flush=True)
    keys = ('operation', 'sampleType', 'backend', 'frames', 'channels', 'offset')
    grouped = {}
    expected = None
    for run in report['runs']:
        seen = set()
        for row in run['records']:
            key = tuple(row[k] for k in keys)
            if key in seen:
                raise RuntimeError('duplicate measurement key')
            seen.add(key)
            values = grouped.setdefault(key, {'before': [], 'after': []})
            values[run['version']].append(row['medianNs'])
        if expected is None:
            expected = seen
        elif seen != expected:
            raise RuntimeError('before/after case sets differ')
    for key, values in sorted(grouped.items()):
        before, after = (statistics.median(values[name]) for name in ('before', 'after'))
        if min(before, after) <= 0:
            raise RuntimeError('zero-duration batched measurement')
        report['comparisons'].append({**dict(zip(keys, key)), 'beforeMedianNs': before,
            'afterMedianNs': after, 'speedup': before/after,
            'beforeTrialMediansNs': values['before'], 'afterTrialMediansNs': values['after']})
    report['status'] = 'passed'
except Exception as error:
    report['status'] = 'failed'
    report['error'] = str(error)
finally:
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
if report['status'] != 'passed':
    raise SystemExit(report['error'])
print('Report: '+str(a.output))
