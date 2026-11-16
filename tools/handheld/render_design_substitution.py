#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate native explicit design choices and capture the production 512x384 workflow."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--assets', type=Path, required=True, help='Private native RCT2 Data/ObjData installation')
parser.add_argument('--output', type=Path, required=True, help='PNG output only; generated parks/designs stay private')
parser.add_argument('--ninja', default='ninja')
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
build = args.build.resolve()
subprocess.run([args.ninja, '-C', str(build), 'openrct2'], check=True)
commands = subprocess.check_output([args.ninja, '-C', str(build), '-t', 'commands', 'openrct2'], text=True).splitlines()
compile_tokens = shlex.split(next(c for c in commands if '/handheld/Explorer.cpp' in c and ' -c ' in c))
flags = []
for i, value in enumerate(compile_tokens):
    if value.startswith(('-I', '-D')):
        flags.append(value)
    elif value == '-isystem':
        flags.extend((value, compile_tokens[i + 1]))
link = shlex.split(commands[-1])
start = next(i for i, value in enumerate(link) if value.endswith('c++'))
end = link.index('&&', start) if '&&' in link[start:] else len(link)
link = [v for v in link[start:end] if not v.endswith('/Ui.cpp.o') and not v.startswith('-Wl,--dependency-file')]
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='pocket-design-substitution-') as directory:
    private = Path(directory)
    i = link.index('-o')
    link[i + 1] = str(private / 'capture')
    link.insert(i, str(repo / 'test/handheld/DesignSubstitutionTests.cpp'))
    link[1:1] = ['-std=c++20', *flags, '-I' + str(repo / 'src'), '-I' + str(repo / 'src/thirdparty')]
    subprocess.run(link, cwd=build, check=True)
    (private / 'capture').chmod(0o755)
    subprocess.run([str(private / 'capture'), str(args.data.resolve()), str(private / 'profile'), str(args.assets.resolve())],
                   check=True, env={**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy',
                                    'OPENRCT2_HANDHELD': '1', 'POCKET_DESIGN_SUBSTITUTION_CAPTURES': str(args.output.resolve())})
