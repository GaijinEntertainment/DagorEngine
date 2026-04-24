danetgamelibs = [
  "render_debug",
  "cables",
  "imgui_daeditor",
  "screen_vhs",
   {"lib": "console_commands", "use_in_tools": False},
  "native_dasevents",
  "dascript_base",
  "renderer",
  "frame_graph_renderer",
  "ground_holes",
  "flash_blind",
  "light_flicker",
  "underwater_postfx",
  "terraform",
  "projectors",
  "semi_trans_render",
  "distant_haze",
  "bloom",
  "puddles_manager",
  "blurred_ui",
  "caustics",
  "water_ripples",
  "adaptation",
  "water_effects",
  "heat_haze",
  "water_flowmap_obstacles",
  "puddle_query",
  "dynamic_details",
  "nbs_volumes",
  "screen_droplets",
  "effect_area",
  "storm",
  "dust",
  "timelapse_screener",
  "daGdp",
  "custom_envi_probe",
  "local_tone_mapping",
  "paint_color",
  "lens_flare",
  "object_motion_blur",
  "screen_snowflakes",
  "sound/common_sounds",
  "sound/environment_sounds",
  "sound/sound_utils",
  "sound/sound_utils_net",
  "composite_entity",
]

gamelibs = [
  "fast_grass",
  "render_events",
  "pufd_events",
  "upscale_sampling",
]

import sys
import os
sys.path.append('../../../prog')
from setup_package import setup_package

setup_package(libsListOrPath=danetgamelibs, gamelibs=gamelibs, basePath="./", dngLibsPath="../../../prog/daNetGameLibs",
  templateOutputPath="./gameBase/gamedata/templates/lib_entities.blk",
  jamPath="_libs.jam",
  vromfsOutputPath="./_libs_prog.vromfs.blk", vromfsName="dng_scene_viewer.vromfs.bin",
  jamPathAot="./_aot/_aot_libs.jam")

toolScriptsMountPoint = './prog/tools'
dasInitPath = os.path.join("..",toolScriptsMountPoint,"libs_init.das")
setup_package(libsListOrPath=danetgamelibs, gamelibs=gamelibs, basePath="./", dngLibsPath="../../../prog/daNetGameLibs",
  dasInitPath=dasInitPath, forTools=True)
