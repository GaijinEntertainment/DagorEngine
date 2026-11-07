# gen_one_big_image.py

Makes one big image from a tiled map: a set of `<quadkey>` tiles of one level.
The script is shared by several games. Use it to make a level overview image or
a tactical map image.

Not covered: `gen_tiled_map.py` and `cut_tiled_map.py` in the same folder. They
build the tile pyramid for the daNetGame tiled map UI.

## Requirements

- Python 3, no extra packages.
- `tools/converters/nconvert/nconvert.exe` in the repo (or `--nconvert <path>`).

Check:

```
python prog/daNetGameLibs/tiled_map/gen_one_big_image.py --help
```

## Tiles: names and layout

- Each tile is named by its quadkey: one digit (0-3) per zoom level, so the
  name length is `zlevels`. With `zlevels=2` there are 16 tiles, `00` to `33`.
- Tile `(0,0)` is the top-left corner of the image: minimum world X, maximum
  world Z. North (+Z) is up.
- The stitched image is `tileWidth * 2^zlevels` pixels square.
- The script merges the tiles 2x2 per level at full size, then resizes once.

## Modes

### Scene mode

```
gen_one_big_image.py <game> <scene.blk> [--resolution N] [--output_name NAME]
```

For daNetGame games. Reads `tiled_map__tilesPath` and `tiled_map__zlevels` from
the `tiled_map` entity in `<repo_root>/<game>/prog/gameBase/<scene.blk>`.
Reads the `.avif` tiles that `gen_tiled_map.py` made in
`<repo_root>/<game>/develop/gui/maps/<tilesPath name>/`, and writes
`<NAME>.tif` (default `big_combined.tif`) there.

| Argument | Default | Meaning |
|---|---|---|
| `game` | (required) | Game folder name in the repo root, e.g. `active_matter` |
| `scene` | (required) | Scene `.blk`, as passed to the game |
| `--resolution` | none | Final square size; uses 256 colours when set |
| `--output_name` | `big_combined` | Output base name |
| `--nconvert` | repo nconvert | Path to `nconvert.exe` |

### Folder mode

```
gen_one_big_image.py --tiles_dir DIR --output FILE [options]
```

For any game that saves `<quadkey>` tiles into a folder. It does not read a
scene: `zlevels` comes from the tile names. When the folder has tiles of more
than one `zlevels`, the script uses the newest complete set.

| Argument | Default | Meaning |
|---|---|---|
| `--tiles_dir` | (required) | Folder with the `<quadkey>` tiles |
| `--output` | (required) | Output image: `.jpg`, `.png`, `.tga` or `.tif` |
| `--tile_ext` | `.tga` | Tile file extension |
| `--resolution` | none | Final square size in pixels; without it the image keeps the stitched size |
| `--flip_y` | off | Flip each tile vertically; use it if the image comes out mirrored |
| `--nconvert` | repo nconvert | Path to `nconvert.exe` |
| `--keep_temp` | off | Keep the intermediate tiff files |

## Script errors

| Message | Cause |
|---|---|
| `[ERROR] tiles folder not found: ...` | `--tiles_dir` does not exist or is not a folder |
| `[ERROR] no <quadkey>.tga tiles in ...` | Wrong `--tiles_dir` or `--tile_ext` |
| `[ERROR] N of M tiles missing (zlevels=Z), first: [...]` | No `zlevels` has all its tiles: the scan did not finish, or tiles were deleted |
| `[WARNING] ignoring N tiles of other zlevels` | Tiles of an older run with other `zlevels` are in the folder |
| `[ERROR] unsupported output format` | Output extension is not `.jpg`, `.png`, `.tga` or `.tif` |
| `[ERROR] nconvert not found: ...` | Wrong `--nconvert` path, or no `tools/converters/nconvert/nconvert.exe` in the repo |
| `nconvert failed (...)` | `nconvert` error; its message is printed |
| `[ERROR] tiled_map__tilesPath not found in ...` | Scene mode: the scene has no `tiled_map` entity with `tilesPath` |
| `[ERROR] zlevels not found in ...` | Scene mode: the scene has no `tiled_map` entity with `zlevels` |

## Usage per project

- `<repo_root>`: the dagor repo root.
- `<project_path>`: the game folder you start the exe from (where its vromfs
  files are).
- `<content_pack>`: a content pack folder in `<project_path>\content\`.
- `<levelName>`, `<scene.blk>`, `<name>`: your level, scene and output names.

### Active Matter (daNetGame)

Uses scene mode. The `tiled_map` DNGL lib is already in the game
(`active_matter/prog/libs/setup_libs.py`). The scenes with a `tiled_map`
entity (`tiled_map__tilesPath`, `tiled_map__zlevels`) are in
`active_matter/prog/gameBase/gamedata/scenes/`, for example
`raid_africa_airport.blk` (`tilesPath` = `maps/forsaken_africa`).

Build the dev exe first: `gen_tiled_map.py` starts
`<repo_root>\active_matter\game\win64\active_matter-dev.exe` by itself.
`gen_one_big_image.py` does not start the exe; it only stitches the tiles.

```
cd <repo_root>
python prog\daNetGameLibs\tiled_map\gen_tiled_map.py active_matter <scene.blk>
python prog\daNetGameLibs\tiled_map\gen_one_big_image.py active_matter <scene.blk> --resolution 4096 --output_name <name>
```

`<scene.blk>` is relative to `<repo_root>\active_matter\prog\gameBase\`, for
example `gamedata/scenes/raid_africa_airport.blk`. The tiles and the big
image go to `<repo_root>\active_matter\develop\gui\maps\<tilesPath name>\`.

### WT (skyquake): satellite map

Renders top-down satellite tiles of a WT level, stitches them with the folder
mode, and shows the image in the tactical map for selected unit types.

Flow: `tiled_map` entity -> scan in game -> `satellite/<quadkey>.tga` ->
`gen_one_big_image.py` folder mode -> one `.jpg` -> `satelliteMap` level key ->
tactical map.

#### Code

| Path | Purpose |
|---|---|
| `prog/gameLibs/render/satelliteRenderer/` | Satellite renderer gameLib (daNetGame and WT) |
| `skyquake/prog/render/frameGraphNodes/wt/satelliteRendererES.cpp.inl` | WT callbacks (landmesh, water, tile save, top-down camera, quit), and the ES that reads the `tiled_map` entity and starts the scan |
| `skyquake/prog/gameBase/gamebase/templates/render/tiled_map.blk` | Entity template |
| `skyquake/prog/gui/hud/hudTacticalMap.cpp` | Shows the level `satelliteMap` image for the HUD types from `hud.blk` |
| `prog/daNetGameLibs/tiled_map/gen_one_big_image.py` | Stitches the tiles (folder mode) |

#### 1. Build and check

1. Build WT (`skyquake/prog`) in dev config. The `render_sat.*` console
   vars work only in dev builds.
2. Build one daNetGame game too, for example `active_matter/prog`. The
   renderer is a shared gameLib, and this build shows that nothing broke there.
3. In `skyquake/prog/gameBase`, run `test_templates.cmd`. It checks the
   `tiled_map` template and the scene files.
4. Check that the script runs (see Requirements).

#### 2. Set up the level

1. Find the level name. It is the map file name without path or extension:
   `levels/hvg_training_ground.bin` gives `hvg_training_ground`.
2. Open `skyquake/prog/gameBase/gamebase/levels/<levelName>.blk` and read the
   rect of the map type that will show the satellite image:

   | HUD type | Who sees it | Rect keys |
   |---|---|---|
   | `infantry` | infantry (human) heroes | `infantryMapCoord0` / `infantryMapCoord1` (default: the tank rect) |
   | `tank` | ground vehicles, support planes and artillery camera from ground | `tankMapCoord0` / `tankMapCoord1` |
   | `air` | aircraft, and ground heroes outside the tank rect | `mapCoord0` / `mapCoord1` |

   For `hvg_training_ground`, the infantry rect is `-512, -512` to `512, 512`.
3. Open or create
   `skyquake/prog/gameBase/gamebase/gamedata/scenes/<levelName>_objects.blk`.
   If the file exists, add to it and do not replace it. Add a `tiled_map`
   entity with the same rect:

   ```
   entity{
     _template:t="tiled_map"
     tiled_map__leftTop:p2=-512, -512
     tiled_map__rightBottom:p2=512, 512
     tiled_map__tileWidth:i=1024
     tiled_map__zlevels:i=2
   }
   ```

4. Repack the vromfs: in `skyquake/prog/gameBase`, run `create_vfsroms.bat`.
   It writes `aces.vromfs.bin` into `<project_path>`.

> The rect must be square, and it must be equal to the map rect of the types
> that show the image. Otherwise the image does not line up with the units and
> the grid.

Choosing the tile size: each tile covers `(rect size) / 2^zlevels` metres.

| Rect | tileWidth | zlevels | Tiles | Stitched image | Tile side |
|---|---|---|---|---|---|
| 1024 m | 1024 | 2 | 16 | 4096 px | 256 m |
| 2048 m | 1024 | 3 | 64 | 8192 px | 256 m |
| 4096 m | 512 | 4 | 256 | 8192 px | 256 m |

Keep each tile at a few hundred metres. Very big tiles lose rendinst detail.

Optional entity fields:

| Field | Default | Effect |
|---|---|---|
| `tiled_map__genExposure:r` | 1 | Fixed exposure; 0 or less keeps adaptation |
| `tiled_map__genHeight:r` | off | Fixed camera height |
| `tiled_map__genHeightOffset:r` | off | Camera height above the land; ignored when `tiled_map__genHeight` is above 0 |
| `tiled_map__genDelay:r` | 20 | Seconds before the scan starts |

#### 3. Quick render check (optional)

This checks only the render, before the full scan. Start the game offline on
any level and type in the console:

```
render_sat.draw_every_frame true
render.save_rt_texture satellite_target sat_test
render_sat.draw_every_frame false
```

`sat_test.tga` should show a top-down view of terrain, rendinsts and water
around the camera.

#### 4. Generate the tiles

```
cd <project_path>
win64\aces-dev.exe -quiet -config:generate_tiled_map:b=yes
```

1. Enter the level offline, for example a test drive or a local mission.
   Scene entities load only with no network or on the server, and never in
   replays.
2. In the log, look for:

   ```
   ECS: Loading additional entities from 'gamedata/scenes/<levelName>_objects.blk'
   ```

   If this line is missing, the entity is not created: wrong level name, a
   network session, or an old vromfs.
3. After `genDelay` seconds (20):
   - exposure is fixed and time stops;
   - the camera goes to a top-down free camera and moves from tile to tile.

   Each tile waits at least `render_sat.scan_settle_frames` (20) frames until
   the clipmap and texture streaming are idle.
4. When the last tile is saved, the log shows `satellite map scan done` and WT
   quits.
5. The tiles are in `<project_path>\satellite\<quadkey>.tga`. Delete old tiles
   before a new run: mixing two runs with the same `zlevels` gives a wrong
   image.

#### 5. Stitch the tiles

```
cd <repo_root>
python prog\daNetGameLibs\tiled_map\gen_one_big_image.py --tiles_dir <project_path>\satellite ^
  --output <project_path>\content\<content_pack>\res\<levelName>_tankmap_sat.jpg --resolution 1024
```

Check the image before you continue (see Tiles: names and layout). Compare a
road or a building with the level. If the image is upside down, run the script
again with `--flip_y`.

Existing WT maps are 1024x1024 (a few are 4096x4096).

#### 6. Use the image in the map

Per level, the image: in
`skyquake/prog/gameBase/gamebase/levels/<levelName>.blk`, next to the
`customLevel*Map` keys:

```
satelliteMap:t = "res\<levelName>_tankmap_sat"
```

The file is `<project_path>\content\<content_pack>\res\<levelName>_tankmap_sat.jpg`
from step 5. The other map keys stay unchanged. A level without
`satelliteMap` keeps its old maps.

Global, which unit types see it: in
`skyquake/prog/gameBase/gamebase/config/hud.blk`, inside `tacticalMap{}`:

```
satelliteMapTypes{
  type:t = "infantry"
}
```

To add a type, add a line: `type:t = "tank"` or `type:t = "air"`. The list is
global for all levels. No code change is needed. An unknown name logs an
error. The `air` type is not good for this renderer, because aircraft map
rects are too big (see Known limits).

Repack the vromfs (`create_vfsroms.bat`) after you change either file.

> The `res\<name>` form (a loose file, no tex-pack asset) is not yet confirmed
> in game. If the map is empty, add the image as a texture asset to the level
> map tex pack and write the asset name with `*`, the same form as the
> `customLevel*Map` keys:
>
> ```
> satelliteMap:t = "<levelName>_tankmap_sat*"
> ```

#### 7. Test in game

1. Start WT normally, without `generate_tiled_map`. Enter the level.
2. As infantry: open the tactical map and the minimap. You should see the
   satellite image.
3. In a tank and in an aircraft: you should see the old maps, because only
   `infantry` is in `satelliteMapTypes`.
4. Alignment: walk along a road or a wall. The player marker must follow the
   same feature in the image, in all corners of the map.
5. Grid: the grid cells and their labels must match the old infantry map.
6. Check the briefing and the mission summary maps too. They load the map the
   same way.

Errors to look for in game:

| What you see | Cause |
|---|---|
| `satelliteRenderer: scan rect ... is not square or empty, no scan` in the log | The `tiled_map` rect is not square or is empty; the scan does not start, and the time and the exposure stay normal |
| `view proj was changed during this scope` in the log | The view/proj restore in `SatelliteRenderer::renderFromPos` missed a case |
| Black tiles, or terrain missing in tiles | A problem in the landmesh or resolve callbacks in `satelliteRendererES.cpp.inl` |
| `hud.blk tacticalMap/satelliteMapTypes: unknown type` in the log | A type name other than `air`, `tank`, `infantry` |
| Infantry map is empty | The `satelliteMap` texture did not load: wrong path, file not in the vromfs content, or the loose-file form does not work |
| Image mirrored top to bottom | Stitch again with `--flip_y` |
| Marker offset from roads | The scan rect and the map rect are not equal |

#### 8. Clean up

- Remove the `tiled_map` entity from `<levelName>_objects.blk` before you
  commit, unless you want to keep it for later runs. Without
  `generate_tiled_map` it does nothing, but it still creates an entity on every
  load of that level.
- Delete the `<project_path>\satellite\` folder with the raw tiles.

#### Known limits

- The WT water and landmesh shaders do not have `use_satellite_rendering`, so
  WT renders them with full shading, not the simplified daNetGame path.
- WT resolves with `deferred_simple`, which takes the view vector from one
  camera point. In an ortho tile this makes the envi reflection and the fog
  change across the tile and jump at tile seams. The fix is a WT
  `satellite_render_resolve` variant like the daNetGame one (vertical view,
  no fog). The gbuffer shaders are not checked for perspective-only code
  (normal mapping, view-dependent terms).
- Tiles are saved as raw render target bytes (`save_rt_image_as_tga`). The
  tile alpha is dropped when stitching.
- Aircraft map rects are too big for this renderer: usually +-16 to 32 km on
  ground levels, and +-32 to 65 km (up to 131 km) on air levels. Use it for
  tank and infantry maps.
- The map-preferences thumbnail (`<levelName>_thumb`) is not replaced by
  `satelliteMap`.
