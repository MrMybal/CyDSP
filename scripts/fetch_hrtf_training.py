"""Explicitly fetch pinned SADIE II HRIR data into the ignored training workspace.

Uses Python standard library only. Does not train or download a neural model.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'data/hrtf/training-sources.json'
ORIGIN = 'https://zenodo.org/records/12092466/files/'

def checked_hash(path, checksum):
    algorithm, expected = checksum.split(':')
    if algorithm not in ('md5', 'sha256'):
        raise ValueError('Unsupported checksum algorithm')
    digest = hashlib.new(algorithm)
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest() == expected

def fetch(subject, rate, destination):
    name = subject['subject']
    url = subject['url']
    if not url.startswith(ORIGIN) or url != ORIGIN + name + '_HRIR_SOFA.zip?download=1':
        raise ValueError('Unpinned archive origin')
    archive = destination / 'archives' / (name + '_HRIR_SOFA.zip')
    archive.parent.mkdir(parents=True, exist_ok=True)
    expected_bytes = subject['archiveBytes']
    checksum = 'sha256:' + subject['archiveSha256']
    if not archive.is_file() or archive.stat().st_size != expected_bytes or not checked_hash(archive, checksum):
        # Reuse an independently verified download made while preparing the bundled selection.
        seed = ROOT / 'build/hrtf-downloads/sadie-ii-v2-2' / archive.name
        temporary = archive.with_name(archive.name + '.part-' + uuid.uuid4().hex)
        try:
            if seed.is_file() and seed.stat().st_size == expected_bytes and checked_hash(seed, checksum):
                shutil.copyfile(seed, temporary)
            else:
                request = urllib.request.Request(url, headers={'User-Agent': 'CyDSP-HRTF-training/0.4.1'})
                received = 0
                with urllib.request.urlopen(request, timeout=60) as response, temporary.open('wb') as out:
                    while block := response.read(1024 * 1024):
                        received += len(block)
                        if received > expected_bytes:
                            raise ValueError('Archive exceeded pinned size')
                        out.write(block)
            if temporary.stat().st_size != expected_bytes or not checked_hash(temporary, checksum) or not checked_hash(temporary, subject['checksum']):
                raise ValueError('Archive size/checksum mismatch: ' + name)
            temporary.replace(archive)
        finally:
            temporary.unlink(missing_ok=True)
    member_name = subject['members'][str(rate)]
    output = destination / str(rate) / Path(member_name).name
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + '.part-' + uuid.uuid4().hex)
    try:
        with zipfile.ZipFile(archive) as source:
            member = source.getinfo(member_name)
            if member.file_size <= 0 or member.file_size > 200000000 or member.is_dir():
                raise ValueError('Invalid SOFA member size')
            with source.open(member) as stream, temporary.open('wb') as out:
                shutil.copyfileobj(stream, out, length=1024 * 1024)
            if temporary.stat().st_size != member.file_size:
                raise ValueError('Incomplete SOFA member')
        with temporary.open('rb') as stream:
            if stream.read(8) != b'\x89HDF\r\n\x1a\n':
                raise ValueError('SOFA member is not HDF5')
        digest = hashlib.sha256()
        with temporary.open('rb') as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(block)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    record = {'subject': name, 'sampleRate': rate, 'file': output.relative_to(destination).as_posix(),
              'bytes': output.stat().st_size, 'sha256': digest.hexdigest(),
              'source': url, 'archiveChecksum': subject['checksum'], 'archiveSha256': subject['archiveSha256'],
              'suggestedSubjectSplit': subject['suggestedSubjectSplit']}
    print(json.dumps({'subject': name, 'bytes': record['bytes'], 'sampleRate': rate}), flush=True)
    return record

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rate', type=int, choices=(44100, 48000, 96000), default=96000)
    parser.add_argument('--subjects', nargs='+', help='Subset of D1 D2 H3 ... H20; default: all twenty')
    parser.add_argument('--list', action='store_true', help='List selected sources without downloading')
    args = parser.parse_args()
    source = json.loads(SOURCE.read_text(encoding='utf-8'))
    subjects = source['subjects']
    if args.subjects:
        names = set(args.subjects)
        if not names.issubset({s['subject'] for s in subjects}):
            parser.error('Unknown subject ID')
        subjects = [s for s in subjects if s['subject'] in names]
    if args.list:
        print(json.dumps({'sampleRate': args.rate, 'archivesBytes': sum(s['archiveBytes'] for s in subjects),
                          'subjects': [s['subject'] for s in subjects]}, indent=2))
        return
    destination = (ROOT / 'build/hrtf-training/sadie-ii-v2-2').resolve()
    if not destination.is_relative_to((ROOT / 'build').resolve()):
        raise ValueError('Training workspace escaped build directory')
    destination.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=3) as pool:
        records = list(pool.map(lambda s: fetch(s, args.rate, destination), subjects))
    shutil.copyfile(ROOT / 'data/hrtf/sadie-ii-v2-2/LICENSE.txt', destination / 'LICENSE.txt')
    attribution = (ROOT / 'data/hrtf/sadie-ii-v2-2/NOTICE.txt').read_text(encoding='utf-8').split('CyDSP selection:')[0]
    (destination / 'NOTICE.txt').write_text(
        attribution + '\nCyDSP local training corpus: original HRIR SOFA archive members.\n'
        'See each index-*.json for the selected subjects, sample rate, file hashes and provenance.\n'
        'The SOFA members remain unmodified. Fetching alone does not train a model.\n',
        encoding='utf-8', newline='\n')
    index = {'schemaVersion': 1, 'dataset': source['dataset'], 'version': source['version'],
             'license': source['license'], 'sampleRate': args.rate, 'files': records,
             'signalBytes': sum(s['bytes'] for s in records),
             'splitNote': 'Suggested human split: H3-H16 train, H17-H18 validation, H19-H20 test; D1/D2 reference. Keep every rate, direction and augmented copy of a subject in its assigned partition. This command does not train a model.'}
    suffix = '-' + '-'.join(s['subject'] for s in subjects) if args.subjects else ''
    path = destination / ('index-' + str(args.rate) + suffix + '.json')
    path.write_text(json.dumps(index, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'profiles': len(records), 'signalBytes': index['signalBytes'],
                      'index': path.relative_to(ROOT).as_posix()}), flush=True)

if __name__ == '__main__':
    main()
