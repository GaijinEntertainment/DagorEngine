require ecs
require AssetsImport
require gameLibs.assets_import.main.asset_manager_events

[es]
def clip_map_invalidation(evt : AssetChangedEvent; asset__clipmapInvalidationEnabled : bool)
  if (asset__clipmapInvalidationEnabled && evt.asset.typeName == "tex")
    broadcastEvent(InvalidateClipmap())

