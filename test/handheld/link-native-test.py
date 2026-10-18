#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Link an initialized SDL/native fixture against an existing Ninja GUI build.

Usage: link-native-test.py BUILD TEST.cpp OUTPUT
Uses the build's compiler definitions, include paths, UI objects and native library.
The GUI entry point is replaced by TEST.cpp; no simulation stubs are substituted.
"""
import pathlib
import shlex
import subprocess
import sys

build, fixture, output = map(lambda p: pathlib.Path(p).resolve(), sys.argv[1:])
commands = subprocess.check_output(
    ['ninja', '-C', str(build), '-t', 'commands', 'openrct2'], text=True
).splitlines()
compile_command = next(shlex.split(c) for c in commands if 'Explorer.cpp.o -c' in c)
flags = []
i = 0
while i < len(compile_command):
    argument = compile_command[i]
    if argument.startswith(('-D', '-I', '-std=', '-fno-char8')):
        flags.append(argument)
    elif argument == '-isystem':
        flags.extend(compile_command[i:i + 2])
        i += 1
    i += 1
link = shlex.split(next(
    part for command in reversed(commands) if ' -o openrct2 ' in command
    for part in command.split(' && ') if ' -o openrct2 ' in part
))
link = [argument for argument in link if not argument.endswith('/Ui.cpp.o') and argument != ':']
link[link.index('-o') + 1] = str(output)
link[1:1] = flags + [str(fixture)]
subprocess.run(link, cwd=build, check=True)
output.chmod(0o755)
