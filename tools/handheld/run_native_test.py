#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Link a handheld integration test against a completed native GUI build."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build',type=Path,required=True)
parser.add_argument('--data',type=Path,required=True)
parser.add_argument('--ninja',default='ninja')
parser.add_argument('--assets',type=Path,help='Optional local RCT installation; enables real UI lifecycle testing with SDL dummy drivers')
args=parser.parse_args()
repo=Path(__file__).resolve().parents[2]
build=args.build.resolve()
subprocess.run([args.ninja,'-C',str(build),'openrct2'],check=True)
commands=subprocess.check_output([args.ninja,'-C',str(build),'-t','commands','openrct2'],text=True).splitlines()
command=commands[-1]
compile_command=next(line for line in commands if '/handheld/Explorer.cpp' in line and ' -c ' in line)
compile_tokens=shlex.split(compile_command)
definitions=[value for value in compile_tokens if value.startswith('-D')]
includes=[]
for index,value in enumerate(compile_tokens):
    if value.startswith('-I'): includes.append(value)
    elif value=='-isystem': includes.extend((value,compile_tokens[index+1]))
link=shlex.split(command)
start=next(i for i,v in enumerate(link) if v.endswith('c++'))
end=link.index('&&',start) if '&&' in link[start:] else len(link)
link=[v for v in link[start:end] if not v.endswith('/Ui.cpp.o')]
with tempfile.TemporaryDirectory(prefix='pocket-native-test-') as directory:
    root=Path(directory)
    i=link.index('-o'); link[i+1]=str(root/'test')
    link.insert(i,str(repo/'test/handheld/PathActionsTests.cpp'))
    link[1:1]=['-std=c++20',*definitions,*includes,f'-I{repo / "src"}',f'-I{repo / "src/thirdparty"}']
    subprocess.run(link,cwd=build,check=True)
    test_args=[str(root/'test'),str(args.data.resolve()),str(root/'profile')]
    if args.assets: test_args.append(str(args.assets.resolve()))
    test_env=os.environ.copy()
    if args.assets:
        test_env.setdefault("SDL_VIDEODRIVER", "dummy")
        test_env.setdefault("SDL_AUDIODRIVER", "dummy")
    subprocess.run(test_args,check=True,env=test_env)
