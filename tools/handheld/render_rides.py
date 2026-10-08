#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Capture production Rides Dashboard workflow over a real loaded RCT scenario."""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--data', type=Path, required=True, help='OpenRCT2 engine data')
parser.add_argument('--assets', type=Path, required=True, help='Private RCT installation containing Data and ObjData')
parser.add_argument('--scenario', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--ninja', default='ninja')
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
build = args.build.resolve()
subprocess.run([args.ninja, '-C', str(build), 'openrct2'], check=True)
commands = subprocess.check_output([args.ninja, '-C', str(build), '-t', 'commands', 'openrct2'], text=True).splitlines()
compile_tokens = shlex.split(next(line for line in commands if '/handheld/Explorer.cpp' in line and ' -c ' in line))
definitions = [v for v in compile_tokens if v.startswith('-D')]
includes = []
for index, value in enumerate(compile_tokens):
    if value.startswith('-I'): includes.append(value)
    elif value == '-isystem': includes.extend((value, compile_tokens[index + 1]))
link = shlex.split(commands[-1])
start = next(i for i, value in enumerate(link) if value.endswith('c++'))
end = link.index('&&', start) if '&&' in link[start:] else len(link)
link = [v for v in link[start:end] if not v.endswith('/Ui.cpp.o')]
with tempfile.TemporaryDirectory(prefix='pocket-rides-capture-') as directory:
    root = Path(directory)
    index = link.index('-o')
    link[index + 1] = str(root / 'capture')
    link.insert(index, str(repo / 'test/handheld/RidesCapture.cpp'))
    link[1:1] = ['-std=c++20', *definitions, *includes]
    subprocess.run(link, cwd=build, check=True)
    subprocess.run([str(root / 'capture'), str(args.data.resolve()), str(args.assets.resolve()),
                    str(args.scenario.resolve()), str(args.output.resolve()), str(root / 'profile')], check=True)
