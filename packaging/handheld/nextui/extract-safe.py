from pathlib import Path
import tarfile,os
for archive,dest in [('gcc.tar.gz','compiler'),('trimui-sdk.tgz','trimui')]:
 root=Path(os.environ['BRICK_SDK'])/dest
 for p in root.rglob('*'):
  if p.is_dir() and not p.is_symlink():p.chmod(0o755)
 hard=[]
 with tarfile.open(root.parent/archive) as tf:
  for m in tf:
   p=root/m.name
   if m.name.startswith('/') or '..' in Path(m.name).parts:raise RuntimeError(m.name)
   p.parent.mkdir(parents=True,exist_ok=True)
   if m.isdir():p.mkdir(exist_ok=True);p.chmod(0o755)
   elif m.isreg():
    if p.exists():p.chmod(0o644)
    with tf.extractfile(m) as src,p.open('wb') as out:
     while b:=src.read(1024*1024):out.write(b)
    p.chmod(0o755 if m.mode&0o111 else 0o644)
   elif m.issym():
    if not p.is_symlink() and not p.exists():p.symlink_to(m.linkname)
   elif m.islnk():hard.append((p,root/m.linkname))
 for p,target in hard:
  if not p.exists():os.link(target,p)
 print(dest,'extracted')
