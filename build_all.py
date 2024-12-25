#!/usr/bin/env python3
# module to help building DagorEngine code, shaders and game data
import sys
import subprocess
import pathlib
import os
import platform
import multiprocessing

DAGOR_ROOT_FOLDER = os.path.dirname(os.path.realpath(__file__))
DAGOR_HOST_ARCH = 'x86_64'

if sys.platform.startswith('win'):
  DAGOR_HOST = 'windows'
  DAGOR_TOOLS_FOLDER = os.path.realpath('{0}/tools/dagor_cdk/windows-{1}'.format(DAGOR_ROOT_FOLDER, DAGOR_HOST_ARCH))
elif sys.platform.startswith('darwin'):
  DAGOR_HOST = 'macOS'
  DAGOR_TOOLS_FOLDER = os.path.realpath('{0}/tools/dagor_cdk/macOS-{1}'.format(DAGOR_ROOT_FOLDER, DAGOR_HOST_ARCH))
elif sys.platform.startswith('linux'):
  DAGOR_HOST = 'linux'
  DAGOR_HOST_ARCH = platform.uname().machine
  DAGOR_TOOLS_FOLDER = os.path.realpath('{0}/tools/dagor_cdk/linux-{1}'.format(DAGOR_ROOT_FOLDER, DAGOR_HOST_ARCH))
else:
  print('\nERROR: unsupported platform {0}\n'.format(sys.platform))
  exit(1)
VROMFS_PACKER_EXE = os.path.realpath('{0}/vromfsPacker-dev{1}'.format(DAGOR_TOOLS_FOLDER, '.exe' if DAGOR_HOST == 'windows' else ''))
DABUILD_EXE = os.path.realpath('{0}/daBuild-dev{1}'.format(DAGOR_TOOLS_FOLDER, '.exe' if DAGOR_HOST == 'windows' else ''))
DABUILD_CMD = [DABUILD_EXE, '-jobs:{}'.format(multiprocessing.cpu_count()), '-q']
FONTGEN_EXE = os.path.realpath('{0}/fontgen2-dev{1}'.format(DAGOR_TOOLS_FOLDER, '.exe' if DAGOR_HOST == 'windows' else ''))

# gather build components from commandline (to allow partial builds)
BUILD_COMPONENTS = []
for s in sys.argv[1:]:
  if not s.startswith('project:') and not s.startswith('arch:'):
    BUILD_COMPONENTS += [s]
# when no components specified we build all of them
if len(BUILD_COMPONENTS) == 0:
  BUILD_COMPONENTS = ['code', 'shaders', 'assets', 'vromfs', 'gui']

# get target architecture from commandline if any (and using default when none specified)
BUILD_TARGET_ARCH = ""
for s in sys.argv[1:]:
  if s.startswith('arch:'):
    BUILD_TARGET_ARCH = s[5:]
    break

def run(cmd, cwd='.'):
  try:
    print('--- Running: {0}  in  {1}'.format(cmd, cwd)); sys.stdout.flush()
    subprocess.run(cmd, shell = True if type(cmd)==str else False, check = True, cwd = cwd)
    return True
  except subprocess.CalledProcessError as e:
    print('subprocess.run failed with a non-zero exit code. Error: {0}'.format(e))
    return False
  except OSError as e:
    print('An OSError occurred, subprocess.run command may have failed. Error: {0}'.format(e))
    return True

def run_per_platform(cmds_windows=[], cmds_macOS=[], cmds_linux=[], cwd='.'):
  for c in cmds_windows if DAGOR_HOST == 'windows' else cmds_macOS if DAGOR_HOST == 'macOS' else cmds_linux if DAGOR_HOST == 'linux' else []:
    if not run(c, cwd=cwd):
      return False
  return True

# build DagorEngine code, shades and vromfs (when script is called directly)
if __name__ == '__main__':
  # gather build projects from commandline (to allow partial builds)
  BUILD_PROJECTS = []
  for s in sys.argv[1:]:
    if s.startswith('project:'):
      BUILD_PROJECTS += [s[8:]]
  # when no projects specified we build all of them
  if len(BUILD_PROJECTS) == 0:
    BUILD_PROJECTS = ['dagorTools', 'dargbox', 'physTest', 'skiesSample', 'testGI', 'outerSpace', 'dngSceneViewer']

  # build command line to run other build scripts
  PY_ADD_CMDLINE = BUILD_COMPONENTS
  if BUILD_TARGET_ARCH != '':
    PY_ADD_CMDLINE += ['arch:'+BUILD_TARGET_ARCH]

  # core CDK (tools and dargbox)
  if 'dagorTools' in BUILD_PROJECTS and 'code' in BUILD_COMPONENTS:
    proj_dir = 'prog/tools'
    if DAGOR_HOST == 'windows':
      if not run('build_dagor_cdk_mini.cmd', cwd=proj_dir):
        print('build_dagor3_cdk_mini.cmd failed, trying once more...')
        if not run('build_dagor_cdk_mini.cmd', cwd=proj_dir):
          print('echo failed to build CDK, stop!')
          exit(1)
    elif DAGOR_HOST == 'macOS':
      run('./build_dagor_cdk_mini_macOS.sh', cwd=proj_dir)
    elif DAGOR_HOST == 'linux':
      run('./build_dagor_cdk_mini_linux.sh', cwd=proj_dir)

  # dargbox tool
  if 'dargbox' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='prog/tools/dargbox')

  # physTest sample
  if 'physTest' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='prog/samples/physTest')

  # skiesSample
  if 'skiesSample' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='samples/skiesSample/prog')

  # testGI sample
  if 'testGI' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='samples/testGI/prog')

  # Outer Space game sample
  if 'outerSpace' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='outerSpace/prog')

  # daNetGame-based Scene Viewer
  if 'dngSceneViewer' in BUILD_PROJECTS:
    run([sys.executable, './build.py'] + PY_ADD_CMDLINE, cwd='samples/dngSceneViewer/prog')
