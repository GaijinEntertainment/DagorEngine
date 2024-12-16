#!/usr/bin/env python3
import sys

if not (sys.version_info.major >= 3 and sys.version_info.minor >= 5):
  print("\nERROR: Python 3.5 or a higher version is required to run this script.")
  exit(1)
if not sys.platform.startswith('darwin'):
  print("\nERROR: script is expected to be run on macOS.")
  exit(1)

import subprocess
import pathlib
import os
import urllib
import ssl
import ctypes
import shutil
import zipfile
import tarfile
import platform
from urllib import request

if len(sys.argv) != 2:
  print('\nUsage: make_devtools_macOS.py DEVTOOLS_DEST_DIR\nexample: python3 make_devtools_macOS.py ~/devtools\n')
  exit(1)

mac_proc_arm = (platform.processor() == 'arm')

def error(s):
  print("\nERROR: {0}\n".format(s))
  exit(1)

def contains_spaces(s):
  return ' ' in s

def contains_non_ascii(s):
  try:
    s.encode('ascii')
  except UnicodeEncodeError:
    return True
  return False

dest_dir = sys.argv[1].replace('\\', '/').rstrip('/')

if contains_spaces(dest_dir):
  error("The destination directory contains spaces, which are not allowed in a file path.")

if contains_non_ascii(dest_dir):
  error("The destination directory contains non-ASCII characters.")

if not os.path.isabs(dest_dir):
  error("The destination directory must be an absolute path.")


def make_directory_symlink(src, dest):
  try:
    subprocess.run('ln -s {0} {1}'.format(os.path.normpath(src), os.path.normpath(dest)), shell = True, check = True)
  except subprocess.CalledProcessError as e:
    error("Symlink command failed with a non-zero exit code. Error: {0}, Source = '{1}', Destination = '{2}'".format(e, src, dest))
  except OSError as e:
    print("An OSError occurred. Symlink command may have failed. Error: {0}, Source = '{1}', Destination = '{2}'".format(e, src, dest))

def run(cmd):
  try:
    print("Running: {0}".format(cmd))
    subprocess.run(cmd, shell = True, check = True)
  except subprocess.CalledProcessError as e:
    print("subprocess.run failed with a non-zero exit code. Error: {0}".format(e))
  except OSError as e:
    print("An OSError occurred, subprocess.run command may have failed. Error: {0}".format(e))


ssl._create_default_https_context = ssl._create_unverified_context

def download_url2(url, file):
  file = "{0}/.packages/{1}".format(dest_dir, file)
  if not pathlib.Path(file).exists():
    print("Downloading '{0}' to '{1}' ...".format(url, file));
    response = request.urlretrieve(url, file)
  else:
    print("Package '{0}' already exists".format(file));

def download_url(url):
  path, file = os.path.split(os.path.normpath(url))
  download_url2(url, file)

pathlib.Path(dest_dir).mkdir(parents=True, exist_ok=True)
pathlib.Path(dest_dir+'/.packages').mkdir(parents=True, exist_ok=True)

# python3
python_dest_folder = dest_dir+'/python3'
if pathlib.Path(python_dest_folder).exists():
  print('=== Python 3 symlink found at {0}, skipping setup'.format(python_dest_folder))
else:
  python_src_folder = os.path.dirname(sys.executable)
  print('+++ Python 3 found at {0}'.format(python_src_folder))
  make_directory_symlink(python_src_folder, python_dest_folder)

  subprocess.run([sys.executable, '-m', 'pip', 'install', '--upgrade', 'pip'])
  subprocess.run([sys.executable, '-m', 'pip', '--version'])
  subprocess.run([sys.executable, '-m', 'pip', 'install', 'clang==14.0.6'])
  subprocess.run([sys.executable, '-m', 'pip', 'install', 'cymbal'])


# nasm
nasm_dest_folder = dest_dir+'/nasm'
if pathlib.Path(nasm_dest_folder).exists():
  print('=== NASM symlink found at {0}, skipping setup'.format(nasm_dest_folder))
else:
  download_url('https://www.nasm.us/pub/nasm/releasebuilds/2.16/macosx/nasm-2.16-macosx.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/nasm-2.16-macosx.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir)
    run('chmod 755 '+dest_dir+'/nasm-2.16/nasm')
    run('chmod 755 '+dest_dir+'/nasm-2.16/ndisasm')
    print('--- will copy nasm to /usr/local/bin using sudo:')
    run('sudo cp '+dest_dir+'/nasm-2.16/nasm /usr/local/bin/nasm')
    print('+++ NASM 2.16 installed at {0}'.format(nasm_dest_folder))


# FMOD
fmod_dest_folder = dest_dir+'/fmod-studio-2.xx.xx'
if pathlib.Path(fmod_dest_folder).exists():
  print('=== FMOD symlinks found at {0}, skipping setup'.format(fmod_dest_folder))
else:
  fmod_src_folder = '{0}/FMOD Programmers API'.format(os.environ['HOME'])
  if pathlib.Path(fmod_src_folder).exists():
    print('+++ FMOD found at {0}'.format(fmod_src_folder))

    pathlib.Path(fmod_dest_folder+'/core/macosx').mkdir(parents=True, exist_ok=True)
    make_directory_symlink('"{0}/api/core/inc"'.format(fmod_src_folder), fmod_dest_folder+'/core/macosx/inc')
    make_directory_symlink('"{0}/api/core/lib"'.format(fmod_src_folder), fmod_dest_folder+'/core/macosx/lib')

    pathlib.Path(fmod_dest_folder+'/studio/macosx').mkdir(parents=True, exist_ok=True)
    make_directory_symlink('"{0}/api/studio/inc"'.format(fmod_src_folder), fmod_dest_folder+'/studio/macosx/inc')
    make_directory_symlink('"{0}/api/studio/lib"'.format(fmod_src_folder), fmod_dest_folder+'/studio/macosx/lib')
    run('cp "{0}/doc/LICENSE.TXT" {1}'.format(fmod_src_folder, fmod_dest_folder))
    run('cp "{0}/doc/revision.txt" {1}'.format(fmod_src_folder, fmod_dest_folder))
  else:
    print('--- FMOD not found at {0}, creating stub folders'.format(fmod_src_folder))
    print('consider downloading and installing https://www.fmod.com/download#fmodengine - Mac Download')
    pathlib.Path(fmod_dest_folder+'/core/macosx/inc').mkdir(parents=True, exist_ok=True)
    pathlib.Path(fmod_dest_folder+'/studio/macosx/inc').mkdir(parents=True, exist_ok=True)


# DXC-1.7.2207
dxc_dest_folder = dest_dir+'/DXC-1.7.2207'
if pathlib.Path(dxc_dest_folder).exists():
  print('=== DXC Jul 2022 found at {0}, skipping setup'.format(dxc_dest_folder))
else:
  download_url('https://github.com/GaijinEntertainment/DXC-prebuilt/releases/download/dxc-1.7.2207/DXC-1.7.2207.tar.gz')
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/DXC-1.7.2207.tar.gz'), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
    tar_file.close()
    shutil.rmtree(dxc_dest_folder+'/lib/linux64')
    shutil.rmtree(dxc_dest_folder+'/lib/win64')
    print('+++ DXC Jul 2022 installed at {0}'.format(dxc_dest_folder))

# astcenc-4.5.1
astcenc_dest_folder = dest_dir+'/astcenc-4.6.1'
if pathlib.Path(astcenc_dest_folder).exists():
  print('=== ASTC encoder 4.6.1 {0}, skipping setup'.format(astcenc_dest_folder))
else:
  download_url('https://github.com/ARM-software/astc-encoder/releases/download/4.6.1/astcenc-4.6.1-macos-universal.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/astcenc-4.6.1-macos-universal.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir+'/astcenc-4.6.1')
    os.rename(os.path.normpath(dest_dir+'/astcenc-4.6.1/bin'), os.path.normpath(dest_dir+'/astcenc-4.6.1/macosx'));
    run('chmod 755 '+dest_dir+'/astcenc-4.6.1/macosx/astcenc*')
    print('+++ ASTC encoder 4.6.1 installed at {0}'.format(astcenc_dest_folder))

# ispc-v1.23.0
ispc_dest_folder = dest_dir+'/ispc-v1.23.0-macOS.x86_64'
if pathlib.Path(ispc_dest_folder).exists():
  print('=== ISPC v1.23.0 {0}, skipping setup'.format(ispc_dest_folder))
else:
  download_url('https://github.com/ispc/ispc/releases/download/v1.23.0/ispc-v1.23.0-macOS.universal.tar.gz')
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/ispc-v1.23.0-macOS.universal.tar.gz'), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
    tar_file.close()
    print('+++ ISPC v1.23.0 installed at {0}'.format(ispc_dest_folder))


try:
  subprocess.run(["jam", "-v"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
except FileNotFoundError:
  print("jam not found, installing jam...")
  jam_zip = 'jam-macOS-11.0-arm64.tar.gz' if mac_proc_arm else 'jam-macOS-10.9-x64_86.tar.gz'
  download_url('https://github.com/GaijinEntertainment/jam-G8/releases/download/2.5-G8-1.3-2024%2F04%2F01/'+jam_zip)
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/'+jam_zip), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
    run('chmod 755 '+dest_dir+'/jam')
    print('--- will copy jam to /usr/local/bin using sudo:')
    run('sudo cp '+dest_dir+'/jam /usr/local/bin/jam')

# re-write platform.jam
if pathlib.Path("prog").exists():
  with open('prog/platform.jam', 'w') as fd:
    fd.write('_DEVTOOL = {0} ;\n'.format(dest_dir))
    if pathlib.Path(fmod_dest_folder+'/LICENSE.TXT').exists():
      fd.write('FmodStudio = 2.xx.xx ;\n')
    else:
      fd.write('FmodStudio = none ;\n')
    fd.close()
  print('--- will prepare {0}/mac folder using jamfile:'.format(dest_dir))
  run('sudo jam -sRoot=. -f prog/engine/kernel/jamfile mkdevtools');
