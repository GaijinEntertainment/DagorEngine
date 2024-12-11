#!/usr/bin/env python3
import sys
sys.path.append('../../..')
from build_all import run, run_per_platform, VROMFS_PACKER_EXE, BUILD_COMPONENTS, DAGOR_HOST, BUILD_TARGET_ARCH
sys.path.pop()


# build EXE
if 'code' in BUILD_COMPONENTS:
  AOT_COMPILER_JAM_OPTIONS = []
  PROJ_JAM_OPTIONS = []
  if BUILD_TARGET_ARCH != '':
    PROJ_JAM_OPTIONS += ['-sPlatformArch='+BUILD_TARGET_ARCH]
    if DAGOR_HOST == 'macOS' and BUILD_TARGET_ARCH == 'arm64': # macOS can run both x86_64 and arm64
      AOT_COMPILER_JAM_OPTIONS += ['-sPlatformArch='+BUILD_TARGET_ARCH]
    elif DAGOR_HOST == 'windows' and BUILD_TARGET_ARCH == 'arm64': # we don't know host arch for sure so we build AOT as x86_64
      AOT_COMPILER_JAM_OPTIONS += ['-sPlatformArch=x86_64']
    else: # otherwise just use the same arch to match main project settings
      AOT_COMPILER_JAM_OPTIONS = PROJ_JAM_OPTIONS

  run(['jam'] + AOT_COMPILER_JAM_OPTIONS, cwd='./_aot')
  run(['jam', '-sNeedDasAotCompile=yes'] + PROJ_JAM_OPTIONS)

# build shaders
if 'shaders' in BUILD_COMPONENTS:
  run_per_platform(
    cmds_windows = ['compile_shaders_dx12.bat', 'compile_shaders_dx11.bat',
                    'compile_shaders_metal.bat', 'compile_shaders_spirv.bat',
                    'compile_shaders_tools.bat'],
    cmds_macOS   = ['./compile_shaders_metal.sh', './compile_tool_shaders_metal.sh'],
    cmds_linux   = ['./compile_shaders_spirv.sh', './compile_tool_shaders_spirv.sh'],
    cwd='./shaders')

#build vromfs
if 'vromfs' in BUILD_COMPONENTS:
  run([VROMFS_PACKER_EXE, 'prog.vromfs.blk', '-platform:PC', '-quiet', '-addpath:.'])
