require("daRg")
local behaviors = {
    Minimap = "Minimap"
    MinimapInput = "MinimapInput"
    TiledMap = "TiledMap"
    TiledMapInput = "TiledMapInput"
}
if (getconsttable()?.Behaviors)
  getconsttable().Behaviors = getconsttable().Behaviors.__merge(behaviors)

global const ROBJ_MINIMAP = "ROBJ_MINIMAP"
global const ROBJ_MINIMAP_BACK = "ROBJ_MINIMAP_BACK"
global const ROBJ_HITCAMERA = "ROBJ_HITCAMERA"
global const ROBJ_MINIMAP_REGIONS_GEOMETRY = "ROBJ_MINIMAP_REGIONS_GEOMETRY"
global const ROBJ_MINIMAP_VIS_CONE = "ROBJ_MINIMAP_VIS_CONE"
global const ROBJ_TILED_MAP = "ROBJ_TILED_MAP"
global const ROBJ_TILED_MAP_VIS_CONE = "ROBJ_TILED_MAP_VIS_CONE"
global const ROBJ_TILED_MAP_FOG_OF_WAR = "ROBJ_TILED_MAP_FOG_OF_WAR"
