#!/usr/bin/env python3
import sys
sys.path.append('../../..')
from build_all import run, run_per_platform, DABUILD_CMD, BUILD_COMPONENTS
sys.path.pop()


# build EXE
if 'code' in BUILD_COMPONENTS:
  run('jam')

# build shaders
if 'shaders' in BUILD_COMPONENTS:
  run_per_platform(
    cmds_windows = ['compile_shaders_dx12.bat', 'compile_shaders_dx11.bat', 'compile_shaders_metal.bat', 'compile_shaders_spirv.bat',
                    'compile_shaders_tools.bat'],
    cmds_macOS   = ['./compile_shaders_metal.sh', './compile_tool_shaders_metal.sh'],
    cmds_linux   = ['./compile_shaders_spirv.sh', './compile_tool_shaders_spirv.sh'],
    cwd='./shaders')

# dabuild assets
if 'assets' in BUILD_COMPONENTS:
  run(DABUILD_CMD + ['../application.blk'], cwd='../develop')
