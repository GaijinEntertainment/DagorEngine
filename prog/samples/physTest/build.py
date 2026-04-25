#!/usr/bin/env python3
import sys
sys.path.append('../../..')
from build_all import run, run_per_platform, BUILD_COMPONENTS, JAM_BUILD_TARGET_ARCH_OPTIONS
sys.path.pop()


# build EXE
if 'code' in BUILD_COMPONENTS:
  run(['jam', '-f', 'jamfile-test-bullet'] + JAM_BUILD_TARGET_ARCH_OPTIONS)
  run(['jam', '-f', 'jamfile-test-jolt'] + JAM_BUILD_TARGET_ARCH_OPTIONS)

# build shaders
if 'shaders' in BUILD_COMPONENTS:
  run_per_platform(
    cmds_windows = ['compile_game_shaders-dx11.bat', 'compile_game_shaders-metal.bat', 'compile_game_shaders-spirv.bat'],
    cmds_macOS   = ['./compile_shaders_metal.sh'],
    cmds_linux   = ['./compile_shaders_spirv.sh'],
    cwd='./shaders')
