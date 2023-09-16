import sys
import subprocess
import pathlib
import os
import urllib
import ctypes
import zipfile
import shutil
from urllib import request

if len(sys.argv) < 2:
  print('echo usage: make_devtools.py DEVTOOLS_DEST_DIR')
  exit(1)

dest_dir = sys.argv[1]
dest_dir = dest_dir.replace('\\', '/')
if dest_dir.find(' ') != -1:
  print('bad dest_dir="{0}", spaces are not allowed!'.format(dest_dir))
  exit(1)

def make_symlink(src, dest):
  subprocess.run(['cmd', '/C', 'mklink', '/j', os.path.normpath(dest), os.path.normpath(src)])

def download_url2(url, file):
  file = '{0}/.packages/{1}'.format(dest_dir, file)
  if not pathlib.Path(file).exists():
    print('downloading {0} to {1} ...'.format(url, file));
    response = request.urlretrieve(url, file)
  else:
    print('package {0} present'.format(file));

def download_url(url):
  path, file = os.path.split(os.path.normpath(url))
  download_url2(url, file)

pathlib.Path(dest_dir).mkdir(parents=True, exist_ok=True)
pathlib.Path(dest_dir+'/.packages').mkdir(parents=True, exist_ok=True)

# python3
python_dest_folder = dest_dir+'/python3'
if pathlib.Path(python_dest_folder).exists():
  print('=== Python-3 symlink found at {0}, skipping setup'.format(python_dest_folder))
else:
  python_src_folder = os.path.dirname(sys.executable)
  if not pathlib.Path(python_src_folder+'/python.exe').exists():
    python_src_folder = os.environ.get('LOCALAPPDATA', '') + '/Programs/Python'
  if pathlib.Path(python_src_folder).exists():
    if not pathlib.Path(python_src_folder+'/python.exe').exists():
      for item in pathlib.Path(python_src_folder).glob("Python3*"):
        if item.is_dir():
          if pathlib.Path(os.path.normpath(item)+'/python.exe').exists():
            python_src_folder = os.path.normpath(item)
            break
    print('+++ Python-3 found at {0}'.format(python_src_folder))
    make_symlink(python_src_folder, python_dest_folder)

    subprocess.run([python_dest_folder+'/python.exe', '-m', 'pip', 'install', '--upgrade', 'pip'])
    subprocess.run([python_dest_folder+'/python.exe', '-m', 'pip', '--version'])
    subprocess.run([python_dest_folder+'/python.exe', '-m', 'pip', 'install', 'clang'])
    subprocess.run([python_dest_folder+'/python.exe', '-m', 'pip', 'install', 'cymbal'])
    shutil.copyfile(python_dest_folder+'/python.exe', python_dest_folder+'/python3.exe')

  else:
    print('--- Python-3 not found')

# Visual C/C++ 2015
vc2015_dest_folder = dest_dir+'/vc2015.3'
if pathlib.Path(vc2015_dest_folder).exists():
  print('=== VC2015 symlink found at {0}, skipping setup'.format(vc2015_dest_folder))
else:
  vc2015_src_folder = os.environ.get('VS140COMNTOOLS', '')
  if vc2015_src_folder != '':
    vc2015_src_folder = os.path.normpath(vc2015_src_folder+'/../..')
  else:
    vc2015_src_folder = '{0}/Microsoft Visual Studio 14.0'.format(os.environ['ProgramFiles(x86)'])
  if vc2015_src_folder and pathlib.Path(vc2015_src_folder).exists():
    print('+++ VC2015 found at {0}'.format(vc2015_src_folder))
    make_symlink(vc2015_src_folder+'/VC', vc2015_dest_folder)
  else:
    print('--- VC2015 not found (VS140COMNTOOLS env var is empty), install VisualStudio 2015 upd.3 and re-run setup')
    download_url('https://aka.ms/vs/17/release/vs_buildtools.exe')

# Visual C/C++ 2019
vc2019_dest_folder = dest_dir+'/vc2019_16.10.3'
if pathlib.Path(vc2019_dest_folder).exists():
  print('=== VC2019 symlink found at {0}, skipping setup'.format(vc2019_dest_folder))
else:
  vc2019_src_folder = '{0}/Microsoft Visual Studio/2019'.format(os.environ['ProgramFiles(x86)'])
  if not pathlib.Path(vc2019_src_folder).exists():
    vc2019_src_folder = '{0}/Microsoft Visual Studio/2022/BuildTools'.format(os.environ['ProgramFiles(x86)'])
  if vc2019_src_folder and pathlib.Path(vc2019_src_folder).exists():
    for nm in ['/Community', '/Enterprise', '/Professional', '']:
      if pathlib.Path(vc2019_src_folder+nm+'/VC/Tools/MSVC').exists():
        vc2019_src_folder = vc2019_src_folder+nm+'/VC/Tools/MSVC'
        for item in pathlib.Path(vc2019_src_folder).glob("*"):
          if item.is_dir():
            if pathlib.Path(os.path.normpath(item)+'/bin').exists():
              vc2019_src_folder = os.path.normpath(item)
              break
        break

    print('+++ VC2019 found at {0}'.format(vc2019_src_folder))
    make_symlink(vc2019_src_folder, vc2019_dest_folder)
  else:
    print('--- VC2019 not found, install VisualStudio 2019 16.10.3+ and re-run setup')
    download_url('https://aka.ms/vs/17/release/vs_buildtools.exe')

# Microsoft SDK 10.0
winsdk_dest_folder = dest_dir+'/win.sdk.100'
if pathlib.Path(winsdk_dest_folder).exists():
  print('=== Windows SDK 10.0 symlink found at {0}, skipping setup'.format(winsdk_dest_folder))
else:
  winsdk_src_folder = '{0}/Windows Kits/10'.format(os.environ['ProgramFiles(x86)'])
  if pathlib.Path(winsdk_src_folder+'/bin').exists():
    print('+++ Windows SDK 10.0 found at {0}'.format(winsdk_src_folder))
    make_symlink(winsdk_src_folder, winsdk_dest_folder)
  else:
    print('--- Windows SDK 10.0 not found, install Windows SDK and re-run setup')
    download_url('https://aka.ms/vs/17/release/vs_buildtools.exe')

# Microsoft SDK 8.1
winsdk_dest_folder = dest_dir+'/win.sdk.81'
if pathlib.Path(winsdk_dest_folder).exists():
  print('=== Windows SDK 8.1 symlink found at {0}, skipping setup'.format(winsdk_dest_folder))
else:
  winsdk_src_folder = '{0}/Windows Kits/8.1'.format(os.environ['ProgramFiles(x86)'])
  if pathlib.Path(winsdk_src_folder+'/bin').exists():
    print('+++ Windows SDK 8.1 found at {0}'.format(winsdk_src_folder))
    make_symlink(winsdk_src_folder, winsdk_dest_folder)
  else:
    print('--- Windows SDK 8.1 not found, install Windows SDK and re-run setup')
    download_url('https://aka.ms/vs/17/release/vs_buildtools.exe')

# LLVM 15.0.7
llvm_dest_folder = dest_dir+'/LLVM-15.0.7'
if pathlib.Path(llvm_dest_folder).exists():
  print('=== LLVM 15.0.7 symlink found at {0}, skipping setup'.format(llvm_dest_folder))
else:
  llvm_src_folder = '{0}/LLVM'.format(os.environ['ProgramFiles']) #(x86)
  if not pathlib.Path(llvm_src_folder+'/bin').exists():
    print('--- LLVM 15.0.7 not found, trying to install')
    download_url('https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/LLVM-15.0.7-win64.exe')
    ctypes.windll.shell32.ShellExecuteW(None, "runas", os.path.normpath(dest_dir+'/.packages/LLVM-15.0.7-win64.exe'), '/s', None, 1)

  if pathlib.Path(llvm_src_folder+'/bin').exists():
    print('+++ LLVM 15.0.7 found at {0}'.format(llvm_src_folder))
    make_symlink(llvm_src_folder, llvm_dest_folder)
  else:
    print('--- LLVM 15.0.7 not found, skipped setup')

# nasm
nasm_dest_folder = dest_dir+'/nasm'
if pathlib.Path(nasm_dest_folder).exists():
  print('=== NASM symlink found at {0}, skipping setup'.format(nasm_dest_folder))
else:
  download_url('https://www.nasm.us/pub/nasm/releasebuilds/2.16/win64/nasm-2.16-win64.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/nasm-2.16-win64.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir)
    make_symlink(dest_dir+'/nasm-2.16', nasm_dest_folder)
    shutil.copyfile(nasm_dest_folder+'/nasm.exe', nasm_dest_folder+'/nasmw.exe')
    shutil.copyfile(nasm_dest_folder+'/ndisasm.exe', nasm_dest_folder+'/ndisasmw.exe')
    print('+++ NASM 2.16 installed at {0}'.format(nasm_dest_folder))

# ducible
ducible_dest_file = dest_dir+'/ducible.exe'
if pathlib.Path(ducible_dest_file).exists():
  print('=== Ducible tool found at {0}, skipping setup'.format(ducible_dest_file))
else:
  download_url('https://github.com/jasonwhite/ducible/releases/download/v1.2.2/ducible-windows-x64-Release.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/ducible-windows-x64-Release.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir)
    print('+++ Ducible tool installed at {0}'.format(dest_dir))

# boost-1.62
def install_boost_1_62():
  boost_dest_folder = dest_dir+'/boost-1.62'
  if pathlib.Path(boost_dest_folder).exists():
    print('=== boost-1.62 found at {0}, skipping setup'.format(boost_dest_folder))
  else:
    boost_src_folder = ''
    for loc in ['c:/local']:
      for item in pathlib.Path(loc).glob("boost_1_62*"):
        if item.is_dir():
          if pathlib.Path(os.path.normpath(item)+'/boost').exists():
            boost_src_folder = os.path.normpath(item)
            break

    if boost_src_folder != '':
      print('+++ boost-1.62 found at {0}'.format(boost_src_folder))
      make_symlink(boost_src_folder, boost_dest_folder)
    else:
      download_url('https://sourceforge.net/projects/boost/files/boost/1.62.0/boost_1_62_0.zip')
      with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/boost_1_62_0.zip'), 'r') as zip_file:
        zip_file.extractall(dest_dir)
        pathlib.Path(dest_dir+'/boost-1.62/libs').mkdir(parents=True, exist_ok=True)
        subprocess.run(['cmd', '/C', 'move',
          os.path.normpath(dest_dir+'/boost_1_62_0/boost'), os.path.normpath(dest_dir+'/boost-1.62/')])
        for d in ['date_time', 'filesystem', 'system', 'thread']:
          pathlib.Path(dest_dir+'/boost-1.62/libs/'+d).mkdir(parents=True, exist_ok=True)
          subprocess.run(['cmd', '/C', 'move',
            os.path.normpath(dest_dir+'/boost_1_62_0/libs/'+d+'/src'), os.path.normpath(dest_dir+'/boost-1.62/libs/'+d)])
        subprocess.run(['cmd', '/C', 'rmdir', '/S', '/Q', os.path.normpath(dest_dir+'/boost_1_62_0')])
        print('+++ boost-1.62 installed at {0}'.format(boost_dest_folder))

#install_boost_1_62()

# 3ds Max SDK
def install_3ds_Max_SDK(ver, url):
  maxsdk_dest_folder = dest_dir+'/max'+ver+'.sdk'
  if pathlib.Path(maxsdk_dest_folder).exists():
    print('=== 3ds Max SDK {1} symlink found at {0}, skipping setup'.format(maxsdk_dest_folder, ver))
  else:
    maxsdk_src_folder = '{0}/Autodesk/3ds Max {1} SDK'.format(os.environ['ProgramFiles'], ver)
    if not pathlib.Path(maxsdk_src_folder+'/maxsdk').exists():
      print('--- 3ds Max SDK '+ver+' not found, trying to install')
      download_url(url)
      path, file = os.path.split(os.path.normpath(url))
      ctypes.windll.shell32.ShellExecuteW(None, "open", os.path.normpath(dest_dir+'/.packages/'+file), '', None, 1)

    if pathlib.Path(maxsdk_src_folder+'/maxsdk').exists():
      print('+++ 3ds Max SDK {1} found at {0}'.format(maxsdk_src_folder, ver))
      make_symlink(maxsdk_src_folder+'/maxsdk', maxsdk_dest_folder)
    else:
      print('--- 3ds Max SDK {1} not found at {0}, skipped setup'.format(maxsdk_src_folder, ver))

install_3ds_Max_SDK('2024',
  'https://autodesk-adn-transfer.s3.us-west-2.amazonaws.com/ADN+Extranet/M%26E/Max/Autodesk+3ds+Max+2024/SDK_3dsMax2024.msi')
install_3ds_Max_SDK('2023',
  'https://autodesk-adn-transfer.s3.us-west-2.amazonaws.com/ADN+Extranet/M%26E/Max/Autodesk+3ds+Max+2023/SDK_3dsMax2023.msi')
#install_3ds_Max_SDK('2019',
#  'https://autodesk-adn-transfer.s3.us-west-2.amazonaws.com/ADN%20Extranet/M%26E/Max/Autodesk%203ds%20Max%202019/SDK_3dsMax2019.msi')

# FMOD
fmod_dest_folder = dest_dir+'/fmod-studio-2.xx.xx'
if pathlib.Path(fmod_dest_folder).exists():
  print('=== FMOD symlinks found at {0}, skipping setup'.format(fmod_dest_folder))
else:
  fmod_src_folder = '{0}/FMOD SoundSystem/FMOD Studio API Windows'.format(os.environ['ProgramFiles(x86)'])
  if pathlib.Path(fmod_src_folder).exists():
    print('+++ FMOD found at {0}'.format(fmod_src_folder))

    pathlib.Path(fmod_dest_folder+'/core/win32').mkdir(parents=True, exist_ok=True)
    make_symlink(fmod_src_folder+'/api/core/inc', fmod_dest_folder+'/core/win32/inc')
    make_symlink(fmod_src_folder+'/api/core/lib/x86', fmod_dest_folder+'/core/win32/lib')

    pathlib.Path(fmod_dest_folder+'/core/win64').mkdir(parents=True, exist_ok=True)
    make_symlink(fmod_src_folder+'/api/core/inc', fmod_dest_folder+'/core/win64/inc')
    make_symlink(fmod_src_folder+'/api/core/lib/x64', fmod_dest_folder+'/core/win64/lib')

    pathlib.Path(fmod_dest_folder+'/studio/win32').mkdir(parents=True, exist_ok=True)
    make_symlink(fmod_src_folder+'/api/studio/inc', fmod_dest_folder+'/studio/win32/inc')
    make_symlink(fmod_src_folder+'/api/studio/lib/x86', fmod_dest_folder+'/studio/win32/lib')

    pathlib.Path(fmod_dest_folder+'/studio/win64').mkdir(parents=True, exist_ok=True)
    make_symlink(fmod_src_folder+'/api/studio/inc', fmod_dest_folder+'/studio/win64/inc')
    make_symlink(fmod_src_folder+'/api/studio/lib/x64', fmod_dest_folder+'/studio/win64/lib')
  else:
    print('--- FMOD not found at {0}, creating stub folders'.format(fmod_src_folder))
    print('consider downloading and installing https://www.fmod.com/download#fmodengine - Windows Download')
    pathlib.Path(fmod_dest_folder+'/core/win32/inc').mkdir(parents=True, exist_ok=True)
    pathlib.Path(fmod_dest_folder+'/core/win64/inc').mkdir(parents=True, exist_ok=True)
    pathlib.Path(fmod_dest_folder+'/studio/win32/inc').mkdir(parents=True, exist_ok=True)
    pathlib.Path(fmod_dest_folder+'/studio/win64/inc').mkdir(parents=True, exist_ok=True)

# OpenXR 1.0.16
openxr_dest_folder = dest_dir+'/openxr-1.0.16'
if pathlib.Path(openxr_dest_folder).exists():
  print('=== OpenXR symlink found at {0}, skipping setup'.format(openxr_dest_folder))
else:
  download_url('https://github.com/KhronosGroup/OpenXR-SDK-Source/releases/download/release-1.0.16/openxr_loader_windows-1.0.16.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/openxr_loader_windows-1.0.16.zip'), 'r') as zip_file:
    zip_file.extractall(openxr_dest_folder)
    make_symlink(openxr_dest_folder+'/openxr_loader_windows/include', openxr_dest_folder+'/include')
    make_symlink(openxr_dest_folder+'/openxr_loader_windows/Win32', openxr_dest_folder+'/win32')
    make_symlink(openxr_dest_folder+'/openxr_loader_windows/x64', openxr_dest_folder+'/win64')
    print('+++ OpenXR 1.0.16 installed at {0}'.format(openxr_dest_folder))

# FidelityFX-FSR2 compiler
ffxsc_dest_folder = dest_dir+'/FidelityFX_SC'
if pathlib.Path(ffxsc_dest_folder).exists():
  print('=== FidelityFX_SC symlink found at {0}, skipping setup'.format(ffxsc_dest_folder))
else:
  download_url2('https://github.com/GPUOpen-Effects/FidelityFX-FSR2/archive/refs/tags/v2.2.1.zip', 'FidelityFX-FSR2.zip')
  with zipfile.ZipFile(os.path.normpath(dest_dir+'/.packages/FidelityFX-FSR2.zip'), 'r') as zip_file:
    zip_file.extractall(dest_dir+'/.packages/')
    make_symlink(dest_dir+'/.packages/FidelityFX-FSR2-2.2.1/tools/sc', ffxsc_dest_folder)
    print('+++ FidelityFX_SC 2.2.1 installed at {0}'.format(ffxsc_dest_folder))


# re-write platform.jam
if pathlib.Path('prog/platform.jam').exists():
  with open('prog/platform.jam', 'w') as fd:
    fd.write('_DEVTOOL = {0} ;\n'.format(dest_dir))
    fd.write('CODE_CHECK = rem ;\n')
    fd.write('FmodStudio = 2.xx.xx ;\n')
    fd.write('BulletSdkVer = 3 ;\n')
    fd.write('OpenSSLVer = 3.x ;\n')
    fd.close()
