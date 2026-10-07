#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render actual native handheld components using an existing Ninja GUI build."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, required=True, help="Existing native Ninja GUI build")
parser.add_argument("--data", type=Path, required=True, help="OpenRCT2 graphics directory")
parser.add_argument("--output", type=Path, required=True, help="PNG contact sheet destination")
parser.add_argument("--ninja", default="ninja")
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
build = args.build.resolve()
args.output.parent.mkdir(parents=True, exist_ok=True)
# Reuse the native link command and libraries, replacing only the application entry point.
subprocess.run([args.ninja, "-C", str(build), "openrct2"], check=True)
commands = subprocess.check_output(
    [args.ninja, "-C", str(build), "-t", "commands", "openrct2"], text=True
).splitlines()
link = shlex.split(commands[-1])
start = next(i for i, value in enumerate(link) if value.endswith("c++"))
end = link.index("&&", start) if "&&" in link[start:] else len(link)
link = [value for value in link[start:end] if not value.endswith("/Ui.cpp.o")]
with tempfile.TemporaryDirectory(prefix="pocket-shell-") as directory:
    temporary = Path(directory)
    output_index = link.index("-o")
    link[output_index + 1] = str(temporary / "preview")
    link.insert(output_index, str(repo / "test/handheld/PanelPreview.cpp"))
    link[1:1] = ["-std=c++20", "-DDISABLE_TTF", f"-I{repo / 'src'}"]
    subprocess.run(link, cwd=build, check=True)
    sheet = Image.new("RGB", (512, 812), (15, 19, 55))
    draw = ImageDraw.Draw(sheet)
    for index, page in enumerate(("park", "terrain")):
        path = temporary / f"{page}.ppm"
        subprocess.run([str(temporary / "preview"), str(args.data.resolve()), str(path), page], check=True)
        top = index * 406
        draw.text((12, top + 5), f"{page.upper()} / native 512 x 384 / sample park data", fill=(255, 243, 95))
        with Image.open(path) as image:
            sheet.paste(image, (0, top + 22))
    sheet.save(args.output)
print(f"Saved native shell preview: {args.output}")
