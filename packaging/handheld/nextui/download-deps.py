from pathlib import Path
import urllib.request,tarfile,os
root=Path(os.environ['BRICK_SDK'])/'deps';root.mkdir(exist_ok=True)
urls={'libzip':'https://libzip.org/download/libzip-1.11.3.tar.xz','zstd':'https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz','icu':'https://github.com/unicode-org/icu/releases/download/release-76-1/icu4c-76_1-src.tgz'}
for name,url in urls.items():
 p=root/(name+'.tar')
 with urllib.request.urlopen(url,timeout=40) as src,p.open('wb') as out:
  while b:=src.read(1024*1024):out.write(b)
 with tarfile.open(p) as tf:
  tf.extractall(root,filter='data')
 print(name,'downloaded',flush=True)
