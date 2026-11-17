"""Check the optional measured HRTF files, notices and strict decimal size limit."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def verify(directory):
    directory = Path(directory).resolve()
    manifest = json.loads((directory / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schemaVersion') != 1 or manifest.get('license') != 'Apache-2.0':
        raise ValueError('Unexpected measured-data manifest')
    expected = {'D1': 8802, 'D2': 8802, 'H10': 2114}
    if len(manifest['files']) != 3 or {item['subject']: item['points'] for item in manifest['files']} != expected:
        raise ValueError('Unexpected profiles or point counts')
    signal_bytes = 0
    for item in manifest['files']:
        name = item['file']
        if Path(name).name != name or not name.endswith('.sofa'):
            raise ValueError('Invalid SOFA path')
        path = directory / name
        data = path.read_bytes()
        if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('Changed original SOFA: ' + name)
        if data[:8] != b'\x89HDF\r\n\x1a\n' or item['sampleRate'] != 48000 or item['taps'] != 256:
            raise ValueError('Unexpected format: ' + name)
        if item['precisionBits'] != 64:
            raise ValueError('The original float64 precision must be retained')
        signal_bytes += len(data)
    for field in ('license', 'notice'):
        name = manifest[field + 'File']
        if Path(name).name != name:
            raise ValueError('Invalid notice path')
        if hashlib.sha256((directory / name).read_bytes()).hexdigest() != manifest[field + 'Sha256']:
            raise ValueError('Changed attribution/legal text: ' + name)
    legal = (directory / manifest['licenseFile']).read_text(encoding='utf-8')
    if not all(text in legal for text in ('Apache License', 'Version 2.0', 'Redistribution.', 'END OF TERMS AND CONDITIONS')):
        raise ValueError('Incomplete Apache license')
    if signal_bytes != manifest['signalBytes']:
        raise ValueError('Inconsistent signal size')
    total = 0
    for path in directory.rglob('*'):
        if path.is_symlink():
            raise ValueError('Unexpected symlink in data distribution')
        if path.is_file():
            total += path.stat().st_size
    limit = manifest['maximumDistributionBytes']
    if limit != 100000000 or total >= limit:
        raise ValueError('Measured-data selection must stay below 100,000,000 bytes')
    return {'profiles': len(manifest['files']), 'signalBytes': signal_bytes, 'folderBytes': total,
            'maximumBytes': limit}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=ROOT / 'data/hrtf/sadie-ii-v2-2')
    args = parser.parse_args()
    print('PASS HRTF data: ' + json.dumps(verify(args.directory)))

if __name__ == '__main__':
    main()
