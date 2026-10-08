#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render actual native handheld components using an existing Ninja GUI build."""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

from PIL import Image, ImageDraw, ImageOps

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, required=True, help="Existing native Ninja GUI build")
parser.add_argument("--data", type=Path, required=True, help="OpenRCT2 graphics directory")
parser.add_argument("--output", type=Path, required=True, help="PNG contact sheet destination")
parser.add_argument("--ninja", default="ninja")
parser.add_argument("--background", type=Path, help="Optional park image for an explicitly labelled layout composite")
parser.add_argument("--frames-dir", type=Path, help="Also save individual unscaled 512 x 384 PNG captures")
parser.add_argument("--pages", nargs="+", default=["terrain", "paths", "path-options"],
                    choices=["terrain", "terrain-lower", "paths", "paths-queue", "paths-remove", "paths-ground", "paths-error", "path-options", "path-options-last", "exploration", "construction", "picker", "inspection", "dashboard", "confirmation", "hud-stress", "ride-depth-summary", "ride-depth-operations", "ride-depth-maintenance", "ride-depth-demolition", "ride-depth-draft", "ride-depth-dense", "ride-depth-vehicles", "ride-depth-dispatch", "ride-depth-vehicle-draft", "ride-depth-vehicle-confirm"],
                    help="Native UI states to capture")
args = parser.parse_args()
if args.frames_dir:
    args.frames_dir.mkdir(parents=True, exist_ok=True)
repo = Path(__file__).resolve().parents[2]
build = args.build.resolve()
args.output.parent.mkdir(parents=True, exist_ok=True)
# Reuse the native link command and libraries, replacing only the application entry point.
subprocess.run([args.ninja, "-C", str(build), "openrct2"], check=True)
commands = subprocess.check_output(
    [args.ninja, "-C", str(build), "-t", "commands", "openrct2"], text=True
).splitlines()
compile_command = next(line for line in commands if "/handheld/Explorer.cpp" in line and " -c " in line)
definitions = [value for value in shlex.split(compile_command) if value.startswith("-D")]
compile_tokens = shlex.split(compile_command)
includes = []
for index, value in enumerate(compile_tokens):
    if value.startswith("-I"):
        includes.append(value)
    elif value == "-isystem":
        includes.extend((value, compile_tokens[index + 1]))
link = shlex.split(commands[-1])
start = next(i for i, value in enumerate(link) if value.endswith("c++"))
end = link.index("&&", start) if "&&" in link[start:] else len(link)
link = [value for value in link[start:end] if not value.endswith("/Ui.cpp.o")]
with tempfile.TemporaryDirectory(prefix="pocket-shell-") as directory:
    temporary = Path(directory)
    output_index = link.index("-o")
    link[output_index + 1] = str(temporary / "preview")
    link.insert(output_index, str(repo / "test/handheld/PanelPreview.cpp"))
    link[1:1] = ["-std=c++20", *definitions, *includes, f"-I{repo / 'src'}", f"-I{repo / 'src/thirdparty'}"]
    subprocess.run(link, cwd=build, check=True)
    sheet = Image.new("RGB", (512, 406 * len(args.pages)), (15, 19, 55))
    draw = ImageDraw.Draw(sheet)
    background_args = []
    if args.background:
        background_path = temporary / "background.ppm"
        with Image.open(args.background) as image:
            ImageOps.fit(image.convert("RGB"), (512, 384), method=Image.Resampling.NEAREST).save(background_path)
        background_args = [str(background_path)]
    for index, page in enumerate(args.pages):
        path = temporary / f"{page}.ppm"
        subprocess.run([str(temporary / "preview"), str(args.data.resolve()), str(path), page, *background_args], check=True)
        top = index * 406
        draw.text((12, top + 5), f"{page.upper()} / native UI" + (" / backdrop composite / sample data" if args.background else " / COMPONENT FIXTURE / no live park"), fill=(255, 243, 95))
        with Image.open(path) as image:
            sheet.paste(image, (0, top + 22))
            if args.frames_dir:
                image.save(args.frames_dir / f"{page}.png")
    sheet.save(args.output)
print(f"Saved native shell preview: {args.output}")
