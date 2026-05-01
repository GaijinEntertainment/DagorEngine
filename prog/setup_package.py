import sys, os, math
import shutil
import os.path
import argparse
import glob

PY_VER = sys.version_info
if PY_VER[0] < 3 or PY_VER[1] < 6: #fstrings
  print("PYTHON 3.6 or later is required")
  sys.exit(1)

def _loadLibsList(listOrPath):
  if isinstance(listOrPath, str):
    with open(listOrPath, 'r') as f:
      return [l.strip() for l in f.readlines()]
  return listOrPath or []

def _parseLibEntry(libEntry):
  if isinstance(libEntry, dict):
    return libEntry.get("lib"), libEntry.get("use_in_tools", True)
  return libEntry, True

def red_text(txt):
  if sys.stdout.isatty():
    return f"\033[31m{txt}\033[0m"
  else:
    return txt

def _processLibs(libs, *, kind, basePath, libsBasePath, mountPoint, jamSubdir,
                 forTools, shadersPathDir, vromfsName, extForVromfs,
                 jam, jamAot, shaders, templateOutput, vromfsOutput, pathToVromCfg):
  dasInitRequires = ""
  dasInitLoads = ""

  for libEntry in libs:
    libPath, useInTools = _parseLibEntry(libEntry)

    if forTools and not useInTools:
      continue

    libName = libPath.split('/')[-1]
    libDirectory = os.path.join(libsBasePath, libPath)
    relPathFromVromCfgToLibs = os.path.relpath(libDirectory, start=os.path.dirname(pathToVromCfg)).replace(os.sep, '/')
    prefix = f"[{kind}] " if kind != "danetgamelibs" else ""
    #print(f"{prefix}{libName} {libPath} {libDirectory} {relPathFromVromCfgToLibs} start={pathToVromCfg}")
    print(f"{prefix}{libName} {libPath} {libDirectory}")

    if not os.path.exists(libDirectory):
      print(f"  {red_text('Error')}: directory does not exist!")
      print(f"  lib: {red_text(libName)} is skipped")
      continue

    if os.path.exists(os.path.join(libDirectory, f'{libName}_init.das')):
      requirement = libPath.replace('/', '.')
      dasInitRequires += f'require {mountPoint}.{requirement}.{libName}_init\n'
      dasInitLoads += f'  ok = load_{libName}("%{mountPoint}/{libPath}") && ok\n'

    if forTools:
      continue

    jamFile = os.path.join(libDirectory, '_lib.jam')
    if os.path.exists(jamFile):
      jam.write(f'include $(Root)/prog/{jamSubdir}/{libPath}/_lib.jam ;\n')

    jamAotFile = os.path.join(libDirectory, '_aot.jam')
    if os.path.exists(jamAotFile):
      jamAot.write(f'include $(Root)/prog/{jamSubdir}/{libPath}/_aot.jam ;\n')

    shaderBlockFile = os.path.join(libDirectory, '_shaders.blk').replace('\\', '/')
    if os.path.exists(shaderBlockFile):
      relToShadersPath = os.path.join(os.path.relpath(libDirectory, start=shadersPathDir), '_shaders.blk').replace('\\', '/')
      shaders.write(f'include "{relToShadersPath}"\n')

    templateFolder = os.path.join(libDirectory, 'templates')
    if os.path.isdir(templateFolder):
      for templateName in sorted(os.listdir(templateFolder)):
        template = templateName.replace(os.sep, '/')
        templateOutput.write(f'import:t="%{mountPoint}/{libPath}/templates/{template}"\n')

    sq_stubs = os.path.join(libDirectory, "sq_stubs")
    if os.path.exists(sq_stubs):
      shutil.copytree(sq_stubs, "../stubs", dirs_exist_ok=True)

    wildcards = ""
    for ext in extForVromfs:
      if len(glob.glob(f'{libDirectory}/**/*{ext}', recursive=True)) > 0:
        wildcards += f'  wildcard:t="*{ext}"\n'

    if len(wildcards) > 0:
      vromfsOutput.write(
f'''
folder {{
  output:t="{vromfsName}"
  path:t="{relPathFromVromCfgToLibs}/"
  dest_path:t="{mountPoint}/{libPath}/"
  scan_folder:b=true
  scan_subfolders:b=true
{wildcards}
}}
''')

  return dasInitRequires, dasInitLoads

def setup_package(libsListOrPath=None, gamelibs=None, basePath=None, dngLibsPath=None, gamelibsBasePath=None,
  vromfsOutputPath=None, vromfsName=None, templateOutputPath=None, dasInitPath=None, jamPath=None, jamPathAot=None, shadersPath=None,
  forTools=False):

  if libsListOrPath is None or basePath is None or dngLibsPath is None:
    print("libsListOrPath, basePath and dngLibsPath is required. \n  Usage: setup_package(libsListOrPath=, basePath=, dngLibsPath=)")
    sys.exit(1)
  if gamelibsBasePath is None:
    gamelibsBasePath = os.path.join(os.path.dirname(dngLibsPath), "gameLibs").replace("\\","/")

  print("libraries path or libraries list", libsListOrPath)
  print("gamelibs list", gamelibs)
  print("path to game/prog folder", basePath)
  print("path to daNetGameLibs folder", dngLibsPath)
  print("path to gameLibs folder", gamelibsBasePath)
  pathToVromCfg = vromfsOutputPath or basePath
  vromfsOutputPath = vromfsOutputPath or os.path.join(basePath, "libs_prog.vromfs.blk")
  print("path to vromfs output file", vromfsOutputPath)
  templateOutputPath = templateOutputPath or os.path.join(basePath, "gameBase/content/common/gamedata/templates/lib_entities.blk")
  print("path to template output file", templateOutputPath)
  dasInitPath = dasInitPath or os.path.join(basePath, "scripts", "libs_init.das")
  print("path to das init output file", dasInitPath)
  jamPath = jamPath or os.path.join(basePath, "libs.jam")
  print("path to jam output file", jamPath)
  jamPathAot = jamPathAot or os.path.join(basePath, "aot", "libs.jam")
  print("path to jam aot output file", jamPathAot)
  shadersPath = shadersPath or os.path.join(basePath, "shaders", "libs_shaders.blk")
  shadersPathDir = os.path.dirname(shadersPath)
  print("path to shader list output file", shadersPath)

  C_LIKE_CODEGEN_COMMENT = "\n//THIS FILE CREATED BY CODEGEN, DON'T CHANGE THIS!!! USE setup_libs.py INSTEAD!!!\n\n"
  PYTHON_LIKE_CODEGEN_COMMENT = "\n#THIS FILE CREATED BY CODEGEN, DON'T CHANGE THIS!!! USE setup_libs.py INSTEAD!!!\n\n"

  dasInit = open(dasInitPath, 'w')
  dasInit.write(C_LIKE_CODEGEN_COMMENT)

  vromfsOutput = jam = jamAot = shaders = templateOutput = None
  extForVromfs = []
  if not forTools:
    vromfsOutput = open(vromfsOutputPath, 'w')
    vromfsOutput.write(C_LIKE_CODEGEN_COMMENT)

    templateOutput = open(templateOutputPath, 'w')
    templateOutput.write(C_LIKE_CODEGEN_COMMENT)

    jam = open(jamPath, 'w')
    jam.write(PYTHON_LIKE_CODEGEN_COMMENT)

    jamAot = open(jamPathAot, 'w')
    jamAot.write(PYTHON_LIKE_CODEGEN_COMMENT)

    shaders = open(shadersPath, 'w')
    shaders.write(C_LIKE_CODEGEN_COMMENT)

    vromfsName = vromfsName or "danetlibs.vromfs.bin"
    extForVromfs = ['.blk', '.das', '.das_project']

  processKwargs = dict(
    basePath=basePath, forTools=forTools, shadersPathDir=shadersPathDir,
    vromfsName=vromfsName, extForVromfs=extForVromfs,
    jam=jam, jamAot=jamAot, shaders=shaders, templateOutput=templateOutput, vromfsOutput=vromfsOutput,
  )

  dngRequires, dngLoads = _processLibs(_loadLibsList(libsListOrPath),
    kind="danetgamelibs", libsBasePath=dngLibsPath, mountPoint="danetlibs", jamSubdir="daNetGameLibs",
    **processKwargs, pathToVromCfg=pathToVromCfg)

  gameRequires, gameLoads = _processLibs(_loadLibsList(gamelibs),
    kind="gameLibs", libsBasePath=gamelibsBasePath, mountPoint="gameLibs", jamSubdir="gameLibs",
    **processKwargs, pathToVromCfg=pathToVromCfg)

  dasInitRequires = dngRequires + gameRequires
  dasInitLoads = dngLoads + gameLoads
  varOrLet = "var" if len(dasInitLoads) else "let"
  dasInit.write(
f'''
options no_aot = true
{dasInitRequires}
def load_libs() : bool
  {varOrLet} ok = true
{dasInitLoads}
  return ok

[export]
def test_all
  let ok = load_libs()
  assert(ok)

''')

if __name__ == "__main__":
  parser = argparse.ArgumentParser()

  parser.add_argument("-libsListPath", help="relative path to text file with list of libraries, each library on separate line", type=str, required=True)
  parser.add_argument("-basePath", help="relative path to game prog folder", type=str, required=True)
  parser.add_argument("-dngLibsPath", help="relative path to daNetGameLibs folder", type=str, required=True)
  parser.add_argument("-vromfsOutputPath", help="relative path for vromfs output file", type=str)
  parser.add_argument("-templateOutputPath", help="relative path for tempaltes output file", type=str)
  parser.add_argument("-dasInitPath", help="relative path for das init output file", type=str)
  parser.add_argument("-jamPath", help="relative path for .jam output file", type=str)
  parser.add_argument("-jamPathAot", help="relative path for .jam aot output file", type=str)
  parser.add_argument("-shadersPath", help="relative path for a shader list .blk output file", type=str)
  args = parser.parse_args()
  libsListPath = args.libsListPath
  basePath = args.basePath
  dngLibsPath = args.dngLibsPath
  vromfsOutputPath = args.vromfsOutputPath
  templateOutputPath = args.templateOutputPath
  dasInitPath = args.dasInitPath
  jamPath = args.jamPath
  jamPathAot = args.jamPathAot
  shadersPath = args.shadersPath
  setup_package(
    libsListOrPath=libsListPath, basePath=basePath,
    dngLibsPath = dngLibsPath, vromfsOutputPath = vromfsOutputPath,
    templateOutputPath = templateOutputPath,
    dasInitPath = dasInitPath,
    jamPath = jamPath,
    jamPathAot = jamPathAot,
    shadersPath = shadersPath)
