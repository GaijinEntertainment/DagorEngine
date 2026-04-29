module asset_manager_events shared
require ecs
require AssetsImport

[cpp_event(broadcast)]
struct AssetAddedEvent
  asset : DagorAsset const?

[cpp_event(broadcast)]
struct AssetChangedEvent
  asset : DagorAsset const?

[cpp_event(broadcast)]
struct AssetRemovedEvent
  asset : DagorAsset const?

[cpp_event(broadcast)]
struct InvalidateClipmap {}
