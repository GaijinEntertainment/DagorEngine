#!/usr/bin/env python3
import sys

if not (sys.version_info.major >= 3 and sys.version_info.minor >= 5):
  print("\nERROR: Python 3.5 or a higher version is required to run this script.")
  exit(1)
if not sys.platform.startswith('linux'):
  print("\nERROR: script is expected to be run on linux.")
  exit(1)

import subprocess
import pathlib
import os
import urllib
import ssl
import ctypes
import zipfile
import shutil
import tarfile
import platform
import glob
import re
from urllib import request

if len(sys.argv) != 2:
  print('\nUsage: make_devtools_linux.py DEVTOOLS_DEST_DIR\nexample: python3 make_devtools_linux.py ~/devtools\n')
  exit(1)


linux_release_name = ''
linux_release_id = ''
linux_release_ver = ''
linux_arch_type = platform.uname().machine

with open('/etc/os-release', 'r') as file:
  for val_pair in re.findall(r'(\w+)=(.+)\n', file.read()):
    if val_pair[0] == 'NAME':
      linux_release_name = val_pair[1]
    elif val_pair[0] == 'ID':
      linux_release_id = val_pair[1]
    elif val_pair[0] == 'VERSION':
      linux_release_ver = val_pair[1]
  file.close()

is_rosa_linux = (linux_release_id == 'rosa')
is_astra_linux = "Astra Linux" in linux_release_name
is_ubuntu = ("Ubuntu" in linux_release_name)
is_centos = ('centos' in linux_release_id)
is_altlinux = (linux_release_id == 'altlinux')
is_elbrus_linux = ('elbrus' in linux_release_id)
print('Detected linux: {0} {1}  ({2}) arch={3}'.format(linux_release_name, linux_release_ver, linux_release_id, linux_arch_type))

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

pkg_to_install = ['nasm']
pkg_install_cmd = ''
if is_rosa_linux:
  pkg_install_cmd = 'dnf install'
  pkg_to_install += ['python3-pip', 'gcc', 'gcc-c++', 'clang']
  pkg_to_install += ['lib64x11-devel', 'lib64xrandr-devel', 'lib64fltk-devel', 'lib64xkbfile-devel', 'lib64udev-devel', 'lib64pulseaudio-devel', 'lib64alsa-oss-devel']
elif is_altlinux:
  pkg_install_cmd = 'apt-get install'
  pkg_to_install += ['python3-module-pip']
  pkg_to_install += ['libX11-devel', 'libXrandr-devel', 'libfltk-devel', 'libxkbfile-devel', 'libudev-devel', 'libpulseaudio-devel', 'libalsa-devel']
elif is_elbrus_linux:
  pkg_install_cmd = 'apt install'
  pkg_to_install  = ['python3-pip']
  pkg_to_install += ['libx11', 'libxrandr', 'fltk', 'libxkbfile', 'pulseaudio', 'alsa-lib']
elif is_ubuntu:
  pkg_install_cmd = 'apt install'
  pkg_to_install += ['python3-pip', 'gcc', 'clang']
  pkg_to_install += ['libx11-dev', 'libxrandr-dev', 'libfltk1.3-dev', 'libxkbfile-dev', 'libudev-dev', 'libpulse-dev']
elif is_astra_linux:
  pkg_install_cmd = 'apt install'
  pkg_to_install += ['python3-pip', 'gcc-mozilla', 'clang-10']
  pkg_to_install += ['libx11-dev', 'libxrandr-dev', 'libfltk1.3-dev', 'libxkbfile-dev', 'libudev-dev', 'libpulse-dev']
elif is_centos:
  pkg_install_cmd = 'yum install'
  pkg_to_install += ['python3-pip', 'devtoolset-11', 'llvm-toolset-7.0']
  pkg_to_install += ['libX11-devel', 'libXrandr-devel', 'libfltk-devel', 'libxkbfile-devel', 'libgudev1-devel', 'pulseaudio-libs-devel', 'alsa-lib-devel']
else:
  pkg_to_install += ['python3-pip', 'gcc', 'gcc-c++', 'clang']
  pkg_to_install += ['libx11-dev', 'libxrandr-dev', 'libfltk1.3-dev', 'libxkbfile-dev', 'libudev-dev', 'libpulse-dev', 'libalsa-devel']

if pkg_install_cmd != '':
  print('--- will try to install required packages:\n  '+' '.join(pkg_to_install))
  run('sudo {0} {1}'.format(pkg_install_cmd, ' '.join(pkg_to_install)))
else:
  print('NOTE: you have to install these (or similar) packages manually:\n  ' + ' '.join(pkg_to_install) + '\n\n')

if is_rosa_linux:
  if not pathlib.Path('/usr/lib64/libclang.so').exists() and pathlib.Path('/usr/lib64/libclang.so.12').exists():
    run('sudo ln -s libclang.so.12 /usr/lib64/libclang.so')

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


# FMOD
fmod_dest_folder = dest_dir+'/fmod-studio-2.xx.xx'
if pathlib.Path(fmod_dest_folder).exists():
  print('=== FMOD symlinks found at {0}, skipping setup'.format(fmod_dest_folder))
else:
  for fmod_src_folder in glob.glob('{0}/fmodstudioapi2*linux'.format(os.environ['HOME'])):
    if pathlib.Path(fmod_src_folder+'/api').exists():
      print('+++ FMOD found at {0}'.format(fmod_src_folder))

      pathlib.Path(fmod_dest_folder+'/core/linux64').mkdir(parents=True, exist_ok=True)
      make_directory_symlink('"{0}/api/core/inc"'.format(fmod_src_folder), fmod_dest_folder+'/core/linux64/inc')
      make_directory_symlink('"{0}/api/core/lib"'.format(fmod_src_folder), fmod_dest_folder+'/core/linux64/lib')

      pathlib.Path(fmod_dest_folder+'/studio/linux64').mkdir(parents=True, exist_ok=True)
      make_directory_symlink('"{0}/api/studio/inc"'.format(fmod_src_folder), fmod_dest_folder+'/studio/linux64/inc')
      make_directory_symlink('"{0}/api/studio/lib"'.format(fmod_src_folder), fmod_dest_folder+'/studio/linux64/lib')
      run('cp "{0}/doc/LICENSE.TXT" {1}'.format(fmod_src_folder, fmod_dest_folder))
      run('cp "{0}/doc/revision.txt" {1}'.format(fmod_src_folder, fmod_dest_folder))
      break
  if not pathlib.Path(fmod_dest_folder).exists():
    print('--- FMOD not found, creating stub folders')
    print('consider downloading and installing https://www.fmod.com/download#fmodengine - Linux Download')
    pathlib.Path(fmod_dest_folder+'/core/linux64/inc').mkdir(parents=True, exist_ok=True)
    pathlib.Path(fmod_dest_folder+'/studio/linux64/inc').mkdir(parents=True, exist_ok=True)


# DXC-1.7.2207
dxc_dest_folder = dest_dir+'/DXC-1.7.2207'
if pathlib.Path(dxc_dest_folder).exists():
  print('=== DXC Jul 2022 found at {0}, skipping setup'.format(dxc_dest_folder))
else:
  download_url('https://github.com/GaijinEntertainment/DXC-prebuilt/releases/download/dxc-1.7.2207/DXC-1.7.2207.tar.gz')
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/DXC-1.7.2207.tar.gz'), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
    tar_file.close()
    shutil.rmtree(dxc_dest_folder+'/lib/macosx')
    shutil.rmtree(dxc_dest_folder+'/lib/win64')
    print('+++ DXC Jul 2022 installed at {0}'.format(dxc_dest_folder))
    if linux_arch_type == 'e2k':
      print('!!! arch={0} differs from x86_64, you should rebuild {1} for\n'
            '    your arch and place result binary to {2}/lib/linux64/libdxcompiler.so\n\n'
            .format(linux_arch_type, 'https://github.com/microsoft/DirectXShaderCompiler', dxc_dest_folder))

# astcenc-4.5.1
astcenc_dest_folder = dest_dir+'/astcenc-4.6.1'
if pathlib.Path(astcenc_dest_folder).exists():
  print('=== ASTC encoder 4.6.1 {0}, skipping setup'.format(astcenc_dest_folder))
elif linux_arch_type == 'e2k':
  pathlib.Path(astcenc_dest_folder+'/linux64').mkdir(parents=True, exist_ok=True)
  print('+++ ASTC encoder 4.6.1 folder created at {0}'.format(astcenc_dest_folder))
  print('!!! arch={0} differs from x86_64, you should rebuild {1}\n'
        '    for your arch and place result binary to {2}/linux64/astcenc-native\n\n'
        .format(linux_arch_type, 'https://github.com/ARM-software/astc-encoder', astcenc_dest_folder))
else:
  download_url('https://github.com/ARM-software/astc-encoder/releases/download/4.6.1/astcenc-4.6.1-linux-x64.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/astcenc-4.6.1-linux-x64.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir+'/astcenc-4.6.1')
    os.rename(os.path.normpath(dest_dir+'/astcenc-4.6.1/bin'), os.path.normpath(dest_dir+'/astcenc-4.6.1/linux64'));
    run('chmod 755 '+dest_dir+'/astcenc-4.6.1/linux64/astcenc*')
    print('+++ ASTC encoder 4.6.1 installed at {0}'.format(astcenc_dest_folder))

# ispc-v1.23.0
ispc_dest_folder = dest_dir+'/ispc-v1.23.0-linux'
if pathlib.Path(ispc_dest_folder).exists():
  print('=== ISPC v1.23.0 {0}, skipping setup'.format(ispc_dest_folder))
elif linux_arch_type == 'e2k':
  pathlib.Path(ispc_dest_folder).mkdir(parents=True, exist_ok=True)
  print('+++ ISPC v1.23.0 skipped (stub created at {0})'.format(ispc_dest_folder))
  print('!!! arch={0} differs from x86_64, you should rebuild {1}\n'
        '    for your arch and place result binary to {2}/bin/ispc\n\n'
        .format(linux_arch_type, 'https://github.com/ispc/ispc', ispc_dest_folder))
else:
  download_url('https://github.com/ispc/ispc/releases/download/v1.23.0/ispc-v1.23.0-linux-oneapi.tar.gz')
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/ispc-v1.23.0-linux-oneapi.tar.gz'), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
    tar_file.close()
    print('+++ ISPC v1.23.0 installed at {0}'.format(ispc_dest_folder))


try:
  subprocess.run(['jam', '-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
except FileNotFoundError:
  print("jam not found, installing jam...")
  jam_zip = 'jam-centOS-7-x86_64.tar.gz'
  if linux_arch_type == 'e2k':
    if is_altlinux:
      jam_zip = 'jam-AltLinux-10-e2k-v3.tar.gz'
    elif is_elbrus_linux:
      jam_zip = 'jam-ElbrusLinux-8-e2k-v3.tar.gz'

  download_url('https://github.com/GaijinEntertainment/jam-G8/releases/download/2.5-G8-1.3-2024%2F04%2F01/'+jam_zip)
  with tarfile.open(os.path.normpath(dest_dir+'/.packages/'+jam_zip), 'r:gz') as tar_file:
    tar_file.extractall(dest_dir)
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
    if linux_arch_type == 'e2k':
      fd.write('PlatformArch = e2k ;\n')
      fd.write('PlatformSpec = gcc ;\n')
      fd.write('WError = no ;\n')
      fd.write('RemoveCompilerSwitches_linux/gcc = -mno-recip -minline-all-stringops -fconserve-space ;\n')
    if is_astra_linux:
      fd.write('PlatformSpec = clang ;\n')
    if is_astra_linux or is_rosa_linux:
      fd.write('MArch = -default- ; #remove it to build for haswell\n') # to avoid building daNetGame for haswell arch
    fd.close()
