#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import argparse,hashlib,json,os,re,shutil,subprocess,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--sysroot',type=Path,required=True);p.add_argument('--assets',type=Path,required=True);p.add_argument('--stage',type=Path,required=True);p.add_argument('--strip',required=True);a=p.parse_args()
repo=Path(__file__).resolve().parents[3];pak=a.stage/'Tools/tg5040/OpenRCT2 Pocket.pak';game=pak/'game';lib=game/'lib';data=game/'data';lib.mkdir(parents=True,exist_ok=True);data.mkdir(parents=True,exist_ok=True)
for name in ['launch.sh','config.ini','INSTALL.md','FIRST-PLAYTEST.md','BUILD.md']:shutil.copy2(Path(__file__).parent/name,pak/name)
(pak/'launch.sh').chmod(0o755)
for name in ['openrct2','openrct2-cli','input-map']:
 src=a.build/name;assert 'AArch64' in subprocess.check_output(['readelf','-h',str(src)],text=True)
 shutil.copy2(src,game/name);(game/name).chmod(0o755);subprocess.run([a.strip,'--strip-unneeded',str(game/name)],check=True)
# Resolve every ELF DT_NEEDED through target SDK roots; never use host libraries.
roots=[a.sysroot/'usr/lib',a.sysroot/'lib'];queue=[game/'openrct2',game/'openrct2-cli',game/'input-map'];copied={}
while queue:
 src=queue.pop();needed=re.findall(r'Shared library: \[([^]]+)\]',subprocess.check_output(['readelf','-d',str(src)],text=True))
 for name in needed:
  if name in copied:continue
  candidates=[root/name for root in roots];found=next((f for f in candidates if f.exists()),None)
  if found is None:raise RuntimeError('Missing target dependency '+name)
  assert 'AArch64' in subprocess.check_output(['readelf','-h',str(found.resolve())],text=True),name
  dest=lib/name;shutil.copy2(found.resolve(),dest);dest.chmod(0o755);copied[name]=str(found.resolve());queue.append(dest)
for name in ['ld-linux-aarch64.so.1','libnss_files.so.2','libnss_dns.so.2','libresolv.so.2']:
 found=next((r/name for r in roots if (r/name).exists()),None)
 if found:shutil.copy2(found.resolve(),lib/name);(lib/name).chmod(0o755)
for name in ['g2.dat','fonts.dat','palettes.dat','tracks.dat']:
 src=a.build/name
 if not src.exists():raise RuntimeError('Supply matching native-generated graphics '+name)
 shutil.copy2(src,data/name)
for folder in ['language','scenario_patches','shaders']:
 shutil.copytree(repo/'data'/folder,data/folder,dirs_exist_ok=True)
# Protected local translation is not a release input: use committed canonical bytes.
(data/'language/eo-ZZ.txt').write_bytes(subprocess.check_output(['git','show','HEAD:data/language/eo-ZZ.txt'],cwd=repo))
shutil.copytree(a.assets/'objects',data/'object',dirs_exist_ok=True);shutil.copytree(a.assets/'title-sequences',data/'title',dirs_exist_ok=True)
(pak/'RCT2').mkdir(exist_ok=True);(pak/'RCT2/PUT_YOUR_ORIGINAL_ASSETS_HERE.txt').write_text('Copy your original RCT2 Data, ObjData and Scenarios folders here. Data/g1.dat must exist. Proprietary assets are not included.\n')
sources=pak/'Sources';sources.mkdir(exist_ok=True);subprocess.run(['git','archive','--format=zip','-o',str(sources/'openrct2-pocket-source.zip'),'HEAD'],cwd=repo,check=True)
for name in ['licence.txt','readme.md']:
 if (repo/name).exists():shutil.copy2(repo/name,sources/name)
(source:=sources/'DEPENDENCY-SOURCES.txt').write_text('''Compiler/sysroot: https://github.com/prepkg/gcc-toolchain/tree/1.0.0-beta (GCC 16.2.0 / glibc 2.26 / crosstool-NG build configuration and patches).
GNU runtime sources/licenses: https://ftp.gnu.org/gnu/glibc/glibc-2.26.tar.xz and https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz (LGPL / GPL with GCC Runtime Library Exception as applicable).
SDL 2.26.1, zlib 1.2.8, libpng 1.2.56: official TrimUI SDK 20231018, https://github.com/trimui/toolchain_sdk_smartpro/releases/tag/20231018. Upstream source/licenses: https://github.com/libsdl-org/SDL/tree/release-2.26.1 ; https://github.com/madler/zlib/tree/v1.2.8 ; https://sourceforge.net/projects/libpng/files/libpng12/1.2.56/ .
libzip 1.11.3: https://libzip.org/download/libzip-1.11.3.tar.xz (BSD).
zstd 1.5.7: https://github.com/facebook/zstd/tree/v1.5.7 (BSD/GPL dual).
ICU 76.1: https://github.com/unicode-org/icu/tree/release-76-1 (Unicode license).
OpenRCT2 data assets: pinned assets.json from bundled source, official objects v1.8.0 and title-sequences v0.4.26. Preserve their bundled license notices.
No RollerCoaster Tycoon proprietary assets are included.
''')
def digest(f):return hashlib.sha256(f.read_bytes()).hexdigest()
manifest={'architecture':'AArch64 / ARMv8 Cortex-A53 / Linux LP64','minimum_kernel_from_ELF':'3.10.108','runtime':'private glibc 2.26 loader; target SDK SDL 2.26.1; dependency closure verified via readelf','source_local_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'source_tree':subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=repo,text=True).strip(),'physical_validation':False,'logical_resolution':[512,384],'physical_resolution':[1024,768],'dependencies':copied,'files':{str(f.relative_to(pak)):digest(f) for f in sorted(pak.rglob('*')) if f.is_file()}}
# Do not expose private build filesystem paths in distributable provenance.
manifest['dependencies']={n:Path(v).name for n,v in copied.items()}
(pak/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(pak)
