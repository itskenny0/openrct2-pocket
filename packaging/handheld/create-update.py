#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Create a merge-only Brick update from an audited ARM build."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
for name in ('binary', 'graphics', 'audit', 'preview', 'strip', 'output'):
    parser.add_argument('--' + name, type=Path, required=True)
parser.add_argument('--github-commit', required=True)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
def git(ref):
    return subprocess.check_output(['git', 'rev-parse', ref], cwd=repo, text=True).strip()

with tempfile.TemporaryDirectory(prefix='pocket-update-') as directory:
    stage = Path(directory)
    game = stage / '.ports/openrct2-handheld'
    data = game / 'engine/share/openrct2'
    data.mkdir(parents=True)
    for target in (stage / 'OpenRCT2-Handheld.sh', stage / '.ports/OpenRCT2-Handheld.sh'):
        shutil.copy2(repo / 'packaging/handheld/OpenRCT2-Handheld.sh', target)
        target.chmod(0o755)
    shutil.copy2(args.binary, game / 'openrct2')
    subprocess.run([str(args.strip), '--strip-unneeded', str(game / 'openrct2')], check=True)
    (game / 'openrct2').chmod(0o755)
    for name in ('g2', 'fonts', 'palettes', 'tracks'):
        shutil.copy2(args.graphics / (name + '.dat'), data / (name + '.dat'))
    # This is the launcher template, never the user's save-handheld/config.ini.
    shutil.copy2(repo / 'packaging/handheld/config.ini', game / 'config.ini')
    shutil.copy2(repo / 'docs/handheld/MODERN-PATHS.md', stage / 'INSTALL-AND-TEST.md')
    shutil.copy2(repo / 'src/openrct2-ui/handheld/PocketSans.LICENSE', game / 'PocketSans.LICENSE')
    shutil.copy2(repo / 'licence.txt', game / 'licence.txt')
    shutil.copy2(args.audit, game / 'ABI-AUDIT.txt')
    shutil.copy2(args.preview, stage / 'NATIVE-UI-PREVIEW.png')
    shutil.copy2(Path(__file__), game / 'create-update.py')
    (game / 'BUILD-INFO.txt').write_text(
        'OpenRCT2 Pocket Brick 0.7 — modern UI and paths\n'
        f'Local source commit: {git("HEAD")}\nSource tree: {git("HEAD^{tree}")}\n'
        f'GitHub equivalent commit: {args.github_commit}\nBranch: handheld/modern-ui-paths\n'
        'AArch64 Linux / Cortex-A53; GCC 16.2.0 / glibc 2.27 sysroot\n'
        'MinSizeRel; software renderer; scripting enabled; TTF/HTTP/network disabled\n'
        'Existing proof runtime libraries retained. See ABI-AUDIT.txt.\n'
        'Physical Brick validation pending. Preview uses sample data / backdrop composite.\n')
    files = sorted(p for p in stage.rglob('*') if p.is_file())
    (stage / 'SHA256SUMS.txt').write_text(''.join(
        hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.relative_to(stage).as_posix() + '\n'
        for p in files))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(p for p in stage.rglob('*') if p.is_file()):
            archive.write(path, path.relative_to(stage).as_posix())
with zipfile.ZipFile(args.output) as archive:
    assert archive.testzip() is None
    assert not any('/save-handheld/' in p or '/RCT1/' in p or '/RCT2/' in p or '/libs/' in p
                   for p in archive.namelist())
print(f'{args.output}: {args.output.stat().st_size} bytes')
print('SHA256: ' + hashlib.sha256(args.output.read_bytes()).hexdigest())
