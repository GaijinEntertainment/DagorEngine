import os, sys, argparse, zipfile
from shutil import copyfile, copytree, rmtree, make_archive
from glob import glob


GDEVTOOL = os.environ['GDEVTOOL']
if GDEVTOOL is None or len(GDEVTOOL) == 0:
  print(f'GDEVTOOL environment variable is not set')
  sys.exit(1)

SYMSTORE_EXE = os.path.join(GDEVTOOL, 'symstore.exe')
BASE_GDK_PATH = 'C:\\Program Files (x86)\\Microsoft GDK'
DEFAULT_SYMSRV_PATH = '\\\\fs.gaijin.lan\\symbols'


available_gdk_versions = [os.path.basename(x) for x in glob(f'{BASE_GDK_PATH}\\*', recursive = False) if os.path.isdir(x) and os.path.basename(x).isdigit()]


argsparser = argparse.ArgumentParser()
argsparser.add_argument('version', choices = available_gdk_versions, help = 'GDK version')
argsparser.add_argument('outdir', help = 'Output directory')
argsparser.add_argument('--upload-symbols', dest = 'upload', action = 'store_true', help = 'Upload symbols to store')
argsparser.add_argument('--symsrv', required = False, default = DEFAULT_SYMSRV_PATH, help = 'Symbol Server address, optional')
args = argsparser.parse_args()

print(f'Selected version: {args.version}')
print(f'Output directory: {args.outdir}')
print(f'Upload symbols: {args.upload}')
print(f'SymSrv path: {args.symsrv}')


sdk_dir = os.path.join(args.outdir, f'xbox.gdk.{args.version}')
if not os.path.exists(args.outdir):
  os.makedirs(args.outdir)
os.makedirs(sdk_dir)


bins_to_copy = [
  'xbapp.exe',
  'xbconnect.exe',
  'xbconfig.exe',
  'xbdbgmon.exe',
  'xbreboot.exe',
  'xbuser.exe',
  'makepkg.exe',
  'xvdsign.exe',
  'xcihash.exe',
  'xcitree.exe',
  'PackagingServices.dll',
  'xbtp.dll',
  'xcapi.dll',
  'xcrdapi.dll',
  'xsapi.dll',
  'XtfApi.dll',
  'XtfApplication.dll',
  'XtfBaseServices.dll',
  'XtfCleanup.dll',
  'XtfConsoleControl.dll',
  'XtfConsoleManager.dll',
  'XtfDebugCapture.dll',
  'XtfDebugMonitor.dll',
  'XtfDiagInfo.dll',
  'XtfFileIO.dll',
  'XtfInternal.dll',
  'XtfProvision.dll',
  'XtfRemoteRun.dll',
  'XtfStreaming.dll',
  'XtfUpdateT.dll',
  'XtfUser.dll'
]

print('Copying SDK')
copytree(os.path.join(BASE_GDK_PATH, args.version), os.path.join(sdk_dir, args.version))
os.mkdir(os.path.join(sdk_dir, 'bin'))
for binary in bins_to_copy:
  copyfile(os.path.join(BASE_GDK_PATH, 'bin', binary), os.path.join(sdk_dir, 'bin', binary))

grdk_path = os.path.join(sdk_dir, args.version, 'GRDK')
gxdk_path = os.path.join(sdk_dir, args.version, 'GXDK')
extension_libs = os.path.join(grdk_path, 'ExtensionLibraries')


content_to_remove = [
  f'{grdk_path}\\redist', # for windows GDK
  f'{grdk_path}\\Plugins', # Unity/Unreal related
  f'{grdk_path}\\VS2019', # garbage
  f'{grdk_path}\\VS2022', # garbage
  f'{grdk_path}\\bin', # garbage
  f'{grdk_path}\\grdk.ini', # garbage
  f'{grdk_path}\\GameKit\\symbols', # symbols

  # we don't use playfab services.
  f'{extension_libs}\\PlayFab.Party.Cpp',
  f'{extension_libs}\\PlayFab.Services.C',
  f'{extension_libs}\\PlayFab.Multiplayer.Cpp',
  f'{extension_libs}\\PlayFab.PartyXboxLive.Cpp',
  f'{extension_libs}\\Xbox.Game.Chat.2.Cpp.API', # we don't use GameChat2
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\ExtensionLibrary.props', # garbage
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\ExtensionLibraryClassic.props', # garbage
  f'{extension_libs}\\Xbox.XCurl.API\\DesignTime\\CommonConfiguration\\neutral\\ExtensionLibrary.props', # garbage
  f'{extension_libs}\\Xbox.XCurl.API\\DesignTime\\CommonConfiguration\\neutral\\XCurlDll.props', # garbage
  f'{extension_libs}\\Xbox.XCurl.API\\DesignTime\\CommonConfiguration\\neutral\\XCurlLibrary.props', # garbage

  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Debug\\v141', # not used
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Release\\v141', # not used
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Debug\\Microsoft.Xbox.Services.141.GDK.C.Thunks.dll', # not used
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Debug\\Microsoft.Xbox.Services.141.GDK.C.Thunks.lib', # not used
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Release\\Microsoft.Xbox.Services.141.GDK.C.Thunks.dll', # not used
  f'{extension_libs}\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\Lib\\Release\\Microsoft.Xbox.Services.141.GDK.C.Thunks.lib', # not used

  f'{gxdk_path}\\bin\\GXDKEditionShortcutResources.dll', # garbage
  f'{gxdk_path}\\Documentation', # docs are available in ndasoft
  f'{gxdk_path}\\EraShim', # needed for simplification of porting from XDK, we don't use it
  f'{gxdk_path}\\toolKit\\tasks', # garbage
  f'{gxdk_path}\\VS2019', # garbage
  f'{gxdk_path}\\VS2022', # garbage
  f'{gxdk_path}\\gxdk.ini', # garbage
  f'{gxdk_path}\\gameKit\\symbols', # symbols
  f'{gxdk_path}\\toolKit\\symbols' # symbols
]


pdb_files = glob(f'{sdk_dir}\\**\\*.pdb', recursive = True)
symbol_files = []

for pdb_file in pdb_files:
  pdb_name, _ = os.path.splitext(pdb_file)
  symbol_files.append(pdb_file)
  for ext_to_check in ['dll', 'exe']:
    check_name = f'{pdb_name}.{ext_to_check}'
    if os.path.exists(check_name):
      symbol_files.append(check_name)

if args.upload:
  for file in symbol_files:
    print(f'Uploading {file}...')
    os.system(f'{SYMSTORE_EXE} add /f {file} /s {args.symsrv} /t {os.path.basename(file)} /v /compress')


for path in content_to_remove:
  if os.path.isdir(path):
    rmtree(path)
  else:
    os.unlink(path)

print('Zipping')
with zipfile.ZipFile(os.path.join(args.outdir, '{}.zip'.format(os.path.basename(sdk_dir))), 'w', zipfile.ZIP_DEFLATED, compresslevel = 9) as f:
  for root, dirs, files in os.walk(sdk_dir):
    for file in files:
      f.write(os.path.join(root, file), os.path.relpath(os.path.join(root, file), args.outdir))

print('Done')