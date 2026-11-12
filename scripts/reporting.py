"""Stable digest of compiler inputs for validation report freshness."""
import hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def source_digest():
    digest=hashlib.sha256()
    files=[ROOT/'CMakeLists.txt', ROOT/'third_party/manifest.json']
    files.extend(p for p in (ROOT/'third_party').rglob('*') if p.suffix in ('.h', '.cpp', '.c'))
    for directory in ('include','src','tests','examples','cmake'):
        files.extend(p for p in (ROOT/directory).rglob('*') if p.suffix in ('.h','.cpp','.cmake','.in','.def'))
    for path in sorted(files):
        digest.update(path.relative_to(ROOT).as_posix().encode()+b'\0'+path.read_bytes()+b'\0')
    return digest.hexdigest()
