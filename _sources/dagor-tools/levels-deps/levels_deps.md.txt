# levels.deps

## Overview

The *levels.deps* tool provides two logs that are essential for tracking asset
and texture dependencies:

1. **Texture-to-Asset Dependency Log**: This log shows which assets are using
   specific textures. With this information, you can precisely identify which
   assets need to have a texture removed to ensure it no longer appears in a
   certain location (or, conversely, determine which assets should be removed
   from a location).

2. **Asset-to-Texture Dependency Log**: This log identifies which textures are
   used by specific assets. It also helps you determine whether a texture is
   used exclusively by a particular asset or is shared across multiple assets.
   Additionally, this log displays the vertex buffer for each asset, as well as
   the global buffer for the map.

## Usage

The usage process is consistent across *all* projects:

1. Navigate to the `<engine_root>/tools/dagor3_cdk/util64/` directory.

2. Run the appropriate batch file.

3. Wait for the tool to process all the maps.

4. Once processing is complete, navigate to the output directory:
   `levels.deps/<project_name>`.

5. Open the `.deps` file for the desired map to access the two types of logs:

   - **Texture-to-Asset Dependency Log**
   - **Asset-to-Texture Dependency Log**

### Texture-to-Asset Dependency Log

This is the first log in the file, which indicates which assets are using a
specific texture.

The log format typically looks like this:

```
[texture size] texture name <- asset(s)
```

**Example**

```
referenced 752 DDSx (4 root, legend: [WxH memSz] TEX <- RES...):
[ 256x256 85K] african_fabric_cover_d_tex_d* <- debris_clothes_f;
[ 256x256 85K] african_fabric_cover_h_tex_d* <- debris_clothes_e;
[ 256x256 85K] african_fabric_cover_tex_n*   <- debris_clothes_e; debris_clothes_f;
.........................
total 874M for 752 texture(s)
```

### Asset-to-Texture Dependency Log

This is the second log in the file, which provides the following information:

- Which textures are used by a specific asset.
- Whether these textures are unique to that asset or shared across multiple
  assets.
- The vertex buffer for each asset and the global vertex buffer for the map.

The log format typically looks like this:

```
[asset vertex buffer + unique texture size] asset name -> unique textures; shared textures;
```

**Example**

```
referenced 3428 resources (1704 root, legend: [resVB + uniqueTex] RES -> unique{TEX...}; shared{TEX...}):
* [ 1345K+ 0K] 88mm_flak_36_canon_a            -> shared[2]={ flak_36_set_tex_d*; flak_36_set_tex_n*; };
               88mm_flak_36_canon_a_collision

* [ 1345K+ 0K] 88mm_flak_36_canon_b            -> shared[2]={ flak_36_set_tex_d*; flak_36_set_tex_n*; };
               88mm_flak_36_canon_b_collision

* [ 71K+ 170K] wooden_power_pole_a             -> unique[2]={ insulator_tex_d*; insulator_tex_n*; }; shared[7]={ decal_modulate_leaks_bottom_tex_m*;};
.........................
total 322.4M (VB/IB) for 3428 resource(s)      // total vertex buffer for the location.
```

This log is particularly useful because of the vertex buffer data, which helps
identify potential issues with excessive texture channels (e.g., if the vertex
buffer size is very large).

## Tool Internals

Understanding the internal structure of the tool is crucial for effective usage.

### File Structure

The tool consists of the following components:

- `dbldUtil-dev.exe`: Located at `<engine_root>/tools/dagor3_cdk/util/`.

- Batch Files (`build_deps.cmd`): Located at
  `<engine_root>/<project_name>/develop/levels.deps/`.

- Configuration Files (`game_env.blk`): Located in the same directory as the
  batch files.

While there's no need to modify the executable, you might need to adjust the
batch files.

### Batch Files

The batch files consist of command lines that perform specific tasks. For
example:

```cmd
@for %%f in (..\..\game\content\<project_name>\levels\*.bin) do ..\..\..\tools\dagor3_cdk\util\dbldUtil-dev.exe PC %%f -deps:game_env-cr.blk ><project_name>\%%~nxf.deps
```

This command means:

- `@for %%f in (..\..\game\content\<project_name>\levels\*.bin)`: For all files
  with a `.bin` extension in the specified directory,
- `do ..\..\..\tools\dagor3_cdk\util\dbldUtil-dev.exe PC %%f
  -deps:game_env-cr.blk ><project_name>\%%~nxf.deps`: run the tool with the
  <project_name> configuration on a PC, and output the results to the
  `<project_name>` directory with the same filename but with a `.deps`
  extension.

If you don’t want the tool to process all binaries, you can modify the batch
file to target specific files.

For instance, to process a specific file:

```
..\..\..\tools\dagor3_cdk\util\dbldUtil-dev.exe PC ..\..\game\content\<project_name>\levels\cr_menu_background.bin -deps:game_env-cr.blk ><project_name>\cr_menu_background.bin.deps
```

This command instructs the tool to:

- process a specific file (`cr_menu_background.bin`),
- using the `<project_name>` configuration,
- output the result to the designated directory and file.

### Tool Configurations

If you encounter any errors while using the tool, it’s essential to check the
configuration files.



The `game_env.blk` file defines the "working directories" and, crucially, the
[packages](../resource-building/resource_building.md#what-is-a-package) required
for the map to function correctly. Therefore, if the tool doesn’t process your
map, don’t worry. Check that all packages the map depends on are listed in the
configuration file and read the error logs carefully.



