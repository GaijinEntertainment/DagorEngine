import sys, os, math
import shutil
import os.path
import argparse
import glob

PY_VER = sys.version_info
if PY_VER[0] < 3 or PY_VER[1] < 6: #fstrings
  print("PYTHON 3.6 or later is required")
  sys.exit(1)

def setup_package(libsListOrPath=None, basePath=None, dngLibsPath=None,
  vromfsOutputPath=None, templateOutputPath=None, dasInitPath=None, jamPath=None, jamPathAot=None, shadersPath=None):

  if libsListOrPath is None or basePath is None or dngLibsPath is None:
    print("libsListOrPath, basePath and dngLibsPath is required. \n  Usage: setup_package(libsListOrPath=, basePath=, dngLibsPath=)")
    sys.exit(1)
  print("libraries path or libraries list", libsListOrPath)
  print("path to game/prog folder", basePath)
  print("path to daNetGameLibs folder", dngLibsPath)
  vromfsOutputPath = vromfsOutputPath or os.path.join(basePath, "dng_prog.vromfs.blk")
  print("path to vromfs output file", vromfsOutputPath)
  templateOutputPath = templateOutputPath or os.path.join(basePath, "gameBase/content/common/gamedata/templates/dng_lib_entities.blk")
  print("path to template output file", templateOutputPath)
  dasInitPath = dasInitPath or os.path.join(basePath, "scripts", "dng_libs_init.das")
  print("path to das init output file", dasInitPath)
  jamPath = jamPath or os.path.join(basePath, "dng_libs.jam")
  print("path to jam output file", jamPath)
  jamPathAot = jamPathAot or os.path.join(basePath, "aot", "dng_libs.jam")
  print("path to jam aot output file", jamPathAot)
  shadersPath = shadersPath or os.path.join(basePath, "shaders", "dng_libs_shaders.blk")
  print("path to shader list output file", shadersPath)

  C_LIKE_CODEGEN_COMMENT = "\n//THIS FILE CREATED BY CODEGEN, DON'T CHANGE THIS!!! USE setup_dng_libs.bat INSTEAD!!!\n\n"
  PYTHON_LIKE_CODEGEN_COMMENT = "\n#THIS FILE CREATED BY CODEGEN, DON'T CHANGE THIS!!! USE setup_dng_libs.bat INSTEAD!!!\n\n"

  vromfsOutput = open(vromfsOutputPath, 'w')
  vromfsOutput.write(C_LIKE_CODEGEN_COMMENT)


  templateOutput = open(templateOutputPath, 'w')
  templateOutput.write(C_LIKE_CODEGEN_COMMENT)

  dasInit = open(dasInitPath, 'w')
  dasInit.write(C_LIKE_CODEGEN_COMMENT)

  dasInitRequires = ""
  dasInitLoads = ""

  jam = open(jamPath, 'w')
  jam.write(PYTHON_LIKE_CODEGEN_COMMENT)

  jamAot = open(jamPathAot, 'w')
  jamAot.write(PYTHON_LIKE_CODEGEN_COMMENT)

  shaders = open(shadersPath, 'w')
  shaders.write(C_LIKE_CODEGEN_COMMENT)

  vromfsName = "danetlibs.vromfs.bin"

  extForVromfs = ['.blk', '.das', '.das_project']

  libs = libsListOrPath
  if isinstance(libsListOrPath, str):
    with open(libsListOrPath, 'r') as f:
      libs = [l.strip() for l in f.readlines()]

  for libPath in libs:
    libName = libPath.split('/')[-1]
    libDirectory = os.path.join(dngLibsPath, libPath)
    relPathFromGameToDNGLibs = os.path.relpath(libDirectory, start = basePath)
    relPathFromGameToDNGLibs = relPathFromGameToDNGLibs.replace(os.sep, '/')
    print(libName, libPath, libDirectory)


    jamFile = os.path.join(libDirectory, '_lib.jam')
    if os.path.exists(jamFile):
      jam.write(f'include $(Root)/prog/daNetGameLibs/{libPath}/_lib.jam ;\n')


    jamAotFile = os.path.join(libDirectory, '_aot.jam')
    if os.path.exists(jamAotFile):
      jamAot.write(f'include $(Root)/prog/daNetGameLibs/{libPath}/_aot.jam ;\n')

    shaderBlockFile = os.path.join(libDirectory, '_shaders.blk').replace('\\', '/')
    if os.path.exists(shaderBlockFile):
      shaders.write(f'include \"{shaderBlockFile}\"\n')

    templateFolder = os.path.join(libDirectory, 'templates')
    if os.path.isdir(templateFolder):
      for templateName in os.listdir(templateFolder):
        template = os.path.join(templateFolder, templateName)
        template = templateName.replace(os.sep, '/')
        templateOutput.write(f'import:t="%danetlibs/{libPath}/templates/{template}"\n')


    if os.path.exists(os.path.join(libDirectory, f'{libName}_init.das')):
      requirement = libPath.replace('/', '.')
      dasInitRequires += f'require danetlibs.{requirement}.{libName}_init\n'
      dasInitLoads += f'  ok = load_{libName}("%danetlibs/{libPath}") && ok\n'

    sq_stubs = os.path.join(libDirectory, "sq_stubs")
    if os.path.exists(sq_stubs):
      shutil.copytree(sq_stubs, "../stubs", dirs_exist_ok=True)

    wildcards = ""
    for ext in extForVromfs:
        if len(glob.glob(f'{libDirectory}/**/*{ext}', recursive=True)) > 0:
          wildcards += f'  wildcard:t="*{ext}"\n'

    if len(wildcards) > 0:
      vromfsOutput.write(
  '''
folder {{
  output:t="{}"
  path:t="{}"
  dest_path:t="{}"
  scan_folder:b=true
  scan_subfolders:b=true
{}
}}
'''.format(vromfsName, f'{relPathFromGameToDNGLibs}/', f'danetlibs/{libPath}/', wildcards))

  dasInit.write(
  '''
options no_aot = true
{}
def load_dng_libs() : bool
  var ok = true
{}
  return ok

[export]
def test_all
  let ok = load_dng_libs()
  assert(ok)

'''.format(dasInitRequires, dasInitLoads))
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

