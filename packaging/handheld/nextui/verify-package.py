#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check a ready-to-transfer NextUI ZIP without running device graphics."""
import argparse, hashlib, io, json, re, struct, subprocess, tempfile, zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('archive',type=Path)
p.add_argument('--qemu',help='Optional qemu-aarch64 executable for packaged --version checks')
a=p.parse_args()
prefix='Tools/tg5040/OpenRCT2 Pocket.pak/'
with zipfile.ZipFile(a.archive) as z:
 assert z.testzip() is None,'Archive CRC failure'
 names=set(z.namelist())
 assert not any(n.startswith('/') or '..' in Path(n).parts for n in names),'Unsafe archive path'
 assert not any('.userdata/' in n or n.endswith('/Data/g1.dat') or '/ObjData/' in n for n in names),'User/proprietary data included'
 required=['launch.sh','config.ini','INSTALL.md','manifest.json','game/openrct2','game/openrct2-cli','game/input-map','game/lib/ld-linux-aarch64.so.1','game/data/g2.dat','game/data/fonts.dat','game/data/palettes.dat','game/data/tracks.dat']
 for n in required:assert prefix+n in names,'Missing '+n
 for n in ['launch.sh','game/openrct2','game/openrct2-cli','game/input-map','game/lib/ld-linux-aarch64.so.1']:
  assert (z.getinfo(prefix+n).external_attr>>16)&0o111,'Not executable: '+n
 manifest=json.loads(z.read(prefix+'manifest.json'))
 assert names=={prefix+n for n in manifest['files']}|{prefix+'manifest.json'},'Archive contains unlisted or missing files'
 assert len(z.namelist())==len(names),'Duplicate archive entries'
 for n,digest in manifest['files'].items():assert hashlib.sha256(z.read(prefix+n)).hexdigest()==digest,'Hash mismatch '+n
 assert manifest['logical_resolution']==[512,384] and manifest['physical_resolution']==[1024,768]
 for folder in ['language/','object/','sequence/']:
  assert any(n.startswith(prefix+'game/data/'+folder) for n in names),'Missing resources '+folder
 # Check the engine's actual resource lookup directory and the default sequence contents.
 title=prefix+'game/data/sequence/openrct2.parkseq'
 assert title in names,'Missing default title sequence at engine lookup path'
 with zipfile.ZipFile(io.BytesIO(z.read(title))) as sequence:
  assert sequence.testzip() is None,'Corrupt default title sequence'
  assert sequence.read('script.txt').strip(),'Empty title sequence script'
  assert any(n.endswith(('.park','.sv6','.sc6')) for n in sequence.namelist()),'Title sequence has no park'
 config=z.read(prefix+'config.ini').decode();assert 'window_scale = 2.0' in config and 'window_width = 1024' in config
 launcher=z.read(prefix+'launch.sh').decode();assert '.userdata/tg5040/openrct2-pocket' in launcher and '--openrct2-data-path="$GAME_DIR/data"' in launcher
 with tempfile.TemporaryDirectory(prefix='pocket-package-check-') as d:
  z.extractall(d);pak=Path(d)/prefix;game=pak/'game';lib=game/'lib'
  for f in [game/'openrct2',game/'openrct2-cli',game/'input-map',*lib.iterdir()]:
   f.chmod(0o755)
   b=f.read_bytes();assert b[:4]==b'\x7fELF' and int.from_bytes(b[18:20],'little')==183,'Non-AArch64 ELF '+str(f)
   assert b[4:6]==b'\x02\x01','Not ELF64 little-endian: '+str(f)
   phoff=struct.unpack_from('<Q',b,32)[0];phsize,phnum=struct.unpack_from('<HH',b,54)
   assert phoff+phsize*phnum<=len(b),'Truncated ELF headers: '+str(f)
   for i in range(phnum):
    offset=phoff+i*phsize
    start=struct.unpack_from('<Q',b,offset+8)[0];size=struct.unpack_from('<Q',b,offset+32)[0]
    assert start+size<=len(b),'Truncated ELF segment: '+str(f)

   needs=re.findall(r'Shared library: \[([^]]+)\]',subprocess.check_output(['readelf','-d',str(f)],text=True))
   for name in needs:assert (lib/name).is_file(),'Missing runtime dependency '+name
  if a.qemu:
   for n in ['openrct2','openrct2-cli']:
    out=subprocess.check_output([a.qemu,'-B','0x100000000',str(lib/'ld-linux-aarch64.so.1'),'--library-path',str(lib),str(game/n),'--version'],text=True,stderr=subprocess.STDOUT)
    assert 'AArch64' in out;print(n+': '+out.splitlines()[0])
 print('PASS: archive CRC, safe paths, modes, hashes, ARM64 ELF/dependency closure, resources, launcher, isolated saves')
 if a.qemu:print('PASS: packaged GUI/CLI version execution under QEMU; device graphics/gameplay unverified')
