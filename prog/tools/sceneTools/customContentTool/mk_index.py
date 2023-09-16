import os, hashlib, gzip

ff = gzip.GzipFile(filename='', fileobj=open('00index.gz', 'wb'))
for root, dirs, files in os.walk('.'):
  if 'CVS' in dirs:
    dirs.remove('CVS')
  for fn in files:
    if fn.find('.ddsx') > 0 or fn == 'downloadable_decals.blk' or fn == 'downloadable_skins.blk':
      f = os.path.join(root, fn)
      dig = hashlib.sha1(open(f,'rb').read()).hexdigest()
      fpath = f.replace('\\','/').replace('./','')
      st = os.stat(f)
      ff.write('%s %d %d %s\n' % (fpath, st.st_size, st.st_mtime, dig))



