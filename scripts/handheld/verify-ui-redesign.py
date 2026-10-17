#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Link the UI milestone driver against an existing Ninja GUI build and run it.

Build openrct2 first. For cross builds, --runner-prefix can specify QEMU and the
matching target loader/library path. Captures use public metadata and native
fixture state; they are component renders, not physical device screenshots.
"""
import argparse
from pathlib import Path
import os
import shlex
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--driver", choices=["UiRedesignTests", "ConstructionUxTests"], default="UiRedesignTests")
parser.add_argument("--build", type=Path, required=True)
parser.add_argument("--data", type=Path, required=True)
parser.add_argument("--profile", type=Path, required=True)
parser.add_argument("--captures", type=Path, required=True)
parser.add_argument("--ninja", default=shutil.which("ninja") or "ninja")
parser.add_argument("--runner-prefix", default="")
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
build = args.build.resolve()
commands = subprocess.check_output(
    [args.ninja, "-C", str(build), "-t", "commands", "openrct2"], text=True
).splitlines()
compile_args = shlex.split(next(c for c in commands if "/handheld/Explorer.cpp" in c and " -c " in c))
includes = []
for i, value in enumerate(compile_args):
    if value.startswith(("-I", "-D")):
        includes.append(value)
    elif value == "-isystem":
        includes.extend((value, compile_args[i + 1]))
link = shlex.split(commands[-1])
compiler = compile_args[0]
start = link.index(compiler)
end = link.index("&&", start) if "&&" in link[start:] else len(link)
link = [v for v in link[start:end] if not v.endswith("/Ui.cpp.o")]
out = link.index("-o")
binary = build / ("handheld-ui-redesign-tests" if args.driver == "UiRedesignTests" else "handheld-construction-ux-tests")
link[out + 1] = str(binary)
link.insert(out, str(repo / "test/handheld" / (args.driver + ".cpp")))
link[1:1] = ["-std=c++20", *includes, f"-I{repo / 'src'}", f"-I{repo / 'src/thirdparty'}"]
subprocess.run(link, cwd=build, check=True)
env = os.environ.copy()
env.update(SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
subprocess.run(
    [*shlex.split(args.runner_prefix), str(binary), str(args.data.resolve()),
     str(args.profile.resolve()), str(args.captures.resolve())], env=env, check=True
)
