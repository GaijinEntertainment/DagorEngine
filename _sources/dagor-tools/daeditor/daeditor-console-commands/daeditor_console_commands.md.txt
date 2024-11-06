# Overview

To view a list of all registered commands, use the `list` command in the
console. For syntax details and brief descriptions of each command, use the
`help` command.

# camera.dir

Sets the current direction of the camera.

Usage: `camera.dir x y z [x_up = 0 y_up = 1 z_up = 0]`

# camera.pos

Shows and sets the current position of the camera.

Usage: `camera.pos` to know camera position in last active viewport.

Usage: `camera.pos x y z` to set camera position in last active viewport.

# clear

Clears the console output.

Usage: `clear`

# driver.reset

Resets the graphics driver to its default state.

Usage: `driver.reset`

# entity.stat

Outputs statistics for entity pools to console.

Usage: `entity.stat`

# envi.set

Configures the environment settings.

Usage: `envi.set <envi_name>`

# .sun_from_time

Adjusts the sun position based on the specified time.

Usage: `.sun_from_time year month day time lattitude longtitude`

# exit

Exits the *daEditor*.

Usage: `exit [save_project]`

If save_project is set no message box will be shown.

# help

Displays a list of available commands along with their syntax and descriptions.

Usage: `help <command name>`

# land.commit_changes

Commits changes made to the land data.

Usage: `land.commit_changes`

# land.ltmap

Generates or modifies the lightmap for the land.

Usage: `land.ltmap [file_path]` to calculate and save lightmaps; optional
`file_path` is relative to develop; with no file_path builtScene/lightmap.dds is
saved.

# land.rebuild_colors

Rebuilds the color information for the landmesh/heightmap.

Usage: `land.rebuild_colors`

# list

Lists all registered commands.

Usage: `list`

# perf.dump

Dumps performance data for analysis.

Usage: `perf.dump`

# perf.off

Turns off performance monitoring.

Usage: `perf.off`

# perf.on

Enables performance monitoring.

Usage: `perf.on`

# project.export.all

Exports project in all formats.

Usage: `project.export.all level_name_pc level_name_xbox level_name_ps3` to
export project in all formats where `level_name_*` – path to exported level
relative application levels directory.

# project.export.and

Exports project for the Android platform.

Usage: `project.export.and level_name` to export project in Android (Tegra GPU
family) format where level_name – path to exported level relative application
levels directory.

# project.export.ios

Exports project for the iOS platform.

Usage: `project.export.iOS level_name` to export project in iOS format where
`level_name` – path to exported level relative application levels directory.

# project.export.pc

Exports the project for PC platforms.

Usage: `project.export.pc level_name` to export project in PC format where
`level_name` – path to exported level relative application levels directory.

# project.export.ps3

Exports the project for PlayStation 3 platforms.

Usage: `project.export.ps3 level_name` to export project in PS3 format where
level_name – path to exported level relative application levels directory.

# project.export.ps4

Exports the project for PlayStation 4 platforms.

Usage: `project.export.ps4 level_name` to export project in PS4 format where
`level_name` – path to exported level relative application levels directory.

# project.export.xbox

Exports the project for Xbox platforms.

Usage: `project.export.xbox level_name` to export project in XBOX 360 format
where `level_name` – path to exported level relative application levels
directory.

# project.open

Opens a specified project.

Usage: `project.open file_path [lock = false] [save_project]` where `file_path`
– full path to `level.blk` or path relative to project`s `develop` directory,
`lock` – boolean value to determine to lock project or not, `save_project` –
boolean value to supress save question message box

# project.tex_metrics

Computes and outputs tex metrics to console.

Usage: `project.tex_metrics [verbose] [nodlg]`

# screenshot.ortho

Outputs orthographic screenshot using extents from landscape.

Usage: `screenshot.ortho`

# set_workspace

Sets the current workspace configuration.

Usage: `set_workspace application_blk_path [save_project]` to load workspace
from mentioned "application.blk" file where `application_blk_path` – path to
desired `application.blk` file. Must be full or relative to *daEditor* folder
`save_project` – boolean value to disable save question message box.

# shaders.list

Lists all available shaders.

Usage: `shaders.list [name_substr]`

# shaders.reload

Reloads all shaders from disk.

Usage: `shaders.reload [shaders-binary-dump-fname]`.

# shaders.set

Sets a specified shader configuration.

Usage: `shaders.set <shader-var-name> <value>`

# shadervar

Lists shader variables.

Usage: `shadervar`

# tex.hide

Hides the specified texture.

Usage: `tex.hide`

# tex.info full

Gives a detailed list of textures in a .log/debug file.

Usage: `tex.info full`

# tex.refs

Lists references to the specified texture.

Usage: `tex.refs`

# tex.show

Shows the specified texture.

Usage: `tex.show`

# time.speed

Sets the current simulation speed.

Usage: `time.speed <scale>`

# render.shaderVar  <x> [x] [x] [x] [x]

Sets shader variables with specified values.

Usage: `render.shaderVar  <x> [x] [x] [x] [x]`

# render.shaderBlk  <x>

Applies a shader block configuration.

Usage: `render.shaderBlk  <x>`

# app.tex  [x]

Manages application textures.

Usage: `app.tex`

# app.tex_refs

Calculates amount of references for application textures.

Usage: `app.tex_refs`

# app.save_tex  <x> [x]

Saves the specified texture(s).

Usage: `app.save_tex  <x> [x]`

# app.save_all_tex

Saves all application textures.

Usage: `app.save_all_tex`

# app.stcode

Displays shader code for debugging.

Usage: `app.stcode`

# render.reset_device

Resets the rendering device to its default state.

Usage: `render.reset_device`

# render.hang_device  <x>

Causes the rendering device to hang or pause for debugging purposes.

Usage: `render.hang_device  <x>`

# render.send_gpu_dump  <x> [x]

Sends a GPU memory dump for analysis. Parameters specify details.

Usage: `render.send_gpu_dump  <x> [x]`

# render.reload_shaders  [use_fence] [shaders_bindump_name]

Reloads shaders with optional parameters for synchronization and shader binding
dumps.

Usage: `render.reload_shaders  [use_fence] [shaders_bindump_name]`

# water.hq  [x] [x] [x] [x]

Configures high-quality settings for water rendering.

Usage: `water.hq  [x] [x] [x] [x]`

# water.foam_hats  [x] [x] [x]

Adjusts settings for foam effects on water surfaces.

Usage: `water.foam_hats <hats_mul> <hats_threshold> <hats_folding>`

# water.surface_folding_foam  [x] [x] [x]

Configures settings for surface folding foam in water.

Usage: `water.surface_folding_foam <mul> <pow>`

# water.foam_turbulent  [x] [x] [x] [x]

Adjusts settings for turbulent foam effects on water.

Usage: `water.foam_turbulent <generation_threshold> <generation_amount>
<dissipation_speed> <falloff_speed>`

# water.dependency_wind  [x]

Configures water rendering to depend on wind settings.

Usage: `water.wind_dependency <size>`

# water.alignment_wind  [x]

Adjusts water alignment based on wind settings.

Usage: `water.alignment_wind <size>`

# water.choppiness  [x]

Configures the choppiness of water surfaces.

Usage: `water.choppiness <size>`

# water.facet_size  [x]

Sets the facet size for water tessellation.

Usage: `water.facet_size <size>`

# water.amplitude  [x] [x] [x] [x] [x]

Adjusts the amplitude of water waves.

Usage: `water.amplitude <a01234> or water.amplitude <a0> <a1> <a2> <a3> <a4>`

# water.small_wave_fraction  [x]

Configures the fraction of small waves in water rendering.

Usage: `water.smallWaveFraction <size>`

# water.cascade_window_length  [x]

Sets the length of the cascade window for water rendering.

Usage: `water.cascade_window_length <size>`

# water.cascade_facet_size  [x]

Configures facet size for water cascades.

Usage: `water.cascade_facet_size <size>`

# water.roughness  [x] [x]

Adjusts the roughness of water surfaces.

Usage: `water.roughness <base> <cascadesBase>`

# water.fft_resolution  [x]

Sets the FFT resolution for water rendering.

Usage: `water.fft_resolution <size>`

# water.tesselation  <x>

Configures the tessellation level for water surfaces.

Usage: `water.tesselation <size>`

# water.fft_period  [x]

Sets the FFT period for water waves.

Usage: `water.fft_period <size>`

# water.spectrum  [x] [x]

Configures the water spectrum for wave patterns.

Usage: `water.spectrum <spectra index> <bf_scale>` where `<spectra index>`:
phillips = 0, unified_directional = 1.

# water.vs_samplers  <x>

Sets the number of vertex shader samplers for water.

# water.reset_render

Resets water rendering settings to default.

# water.num_cascades  [x]

Configures the number of cascades for water rendering.

Usage: `water.num_cascades <size>`

# debug_mesh.range  <x> <x>

Sets the range for displaying debug mesh information.

# skies.convert_weather_blk_to_entity  <x>

Converts a weather block to an entity format.

Usage: `rendinst.hide_object <rendinst name>`

# rendinst.hide_object  [x]

Hides a specified rendering instance object.

Usage: `rendinst.hide_object <rendinst name>`

# rendinst.list

Lists all rendering instances.

# rendinst.profiling  [x]

Toggles rendinst in profiles by name, or include all ri in profiler (`all`), or
exclude all rendinst from profiler (`none`).

Usage: `rendinst.profiling <rendins tname>|all|none`

# rendinst.verify_lods  [x]

Verifies level of detail (LOD) settings for rendering instances.

# ri_gpu_objects.invalidate

Invalidates GPU objects for the rendering interface, potentially forcing a
reload.

# rigrid.stats

Displays statistics about the rig grid.

# shaded_collision.none

Disables shaded collision rendering.

# shaded_collision.alone

Displays shaded collision alone without any visual elements.

# shaded_collision.with_visual

Displays shaded collision with visual elements.

# shaded_collision.alone_diff

Shows differences in shaded collision alone.

# shaded_collision.with_visual_diff

Displays differences in shaded collision with visual elements.

# shaded_collision.wireframe

Renders shaded collision in wireframe mode.

# shaded_collision.face_orientation

Displays the face orientation for shaded collision.

# gameres.update_pack  <x>

Updates the specified resource pack for the game.

# gameres.list_missing_for_res  <x> [x]

Lists missing resources for a specified category.

# gameres.list_missing_for_tex  <x>

Lists missing textures for the specified category.

# gameres.list_missing_for_all_tex  [x]

Lists missing textures across all categories.

# riUnitedVdata.list

Lists UnitedV data for rendering interface.

# riUnitedVdata.status

Displays the status of UnitedV data.

# riUnitedVdata.dumpMem

Dumps memory for UnitedV data.

# riUnitedVdata.perfStat

Provides performance statistics for UnitedV data.

# dmUnitedVdata.list

Lists UnitedV data for data management.

# dmUnitedVdata.status

Displays the status of UnitedV data in data management.

# dmUnitedVdata.dumpMem

Dumps memory for UnitedV data in data management.

# dmUnitedVdata.perfStat

Provides performance statistics for UnitedV data in data management.

# tex.list  [x] [x] [x] [x] [x] [x] [x] [x] [x]

Lists textures with optional filtering parameters.

# tex.downgrade  <x> [x]

Downgrades the specified texture to a lower quality.

# tex.setMaxLev  <x> [x]

Sets the maximum level for texture detail.

# tex.set  <x>

Sets the specified texture.

# tex.addRef  <x>

Adds a reference to the specified texture.

# tex.delRef  <x>

Removes a reference from the specified texture.

# tex.markLFU  <x> [x]

Marks the specified texture as Least Frequently Used (LFU).


