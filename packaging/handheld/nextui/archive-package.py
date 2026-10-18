#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Archive exactly the verified package manifest, excluding staging/transfer debris."""
import argparse
import hashlib
import json
import stat
import zipfile
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--stage', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
prefix = Path('Tools/tg5040/OpenRCT2 Pocket.pak')
pak = a.stage / prefix
manifest_bytes = (pak / 'manifest.json').read_bytes()
manifest = json.loads(manifest_bytes)
with zipfile.ZipFile(a.output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for name, expected in sorted(manifest['files'].items()):
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Unsafe manifest path: ' + name)
        f = pak / relative
        content = f.read_bytes()
        if hashlib.sha256(content).hexdigest() != expected:
            raise ValueError('Staged file changed: ' + name)
        info = zipfile.ZipInfo(str(prefix / relative))
        info.create_system = 3
        info.external_attr = (stat.S_IFREG | (0o755 if f.stat().st_mode & 0o111 else 0o644)) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(info, content)
    archive.writestr(str(prefix / 'manifest.json'), manifest_bytes)
print(a.output)
