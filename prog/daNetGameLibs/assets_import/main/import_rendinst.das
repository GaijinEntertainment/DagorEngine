require ecs
require math
require AssetsImport
require RendInst
require danetlibs.assets_import.main.asset_manager_events
require danetlibs.assets_import.main.import_rendinst_common
require danetlibs.assets_import.das_includes.rendinst_ui

[es]
def on_rendinst_changed(evt : AssetChangedEvent;
                        asset__rendInstType : int)
  if (evt.asset.typeId == asset__rendInstType)
    rendinst_reloadRIExtraResources(getName(*evt.asset))


def destroy_composit_tree(eid : EntityId)

  query(eid) <| $ [es] (composit__entities : EidList)
    for child in composit__entities
      destroy_composit_tree(child)
  destroyEntity(eid)

require DagorSystem
[es]
def on_composit_changed(evt : AssetChangedEvent)
  if true
    let name = *evt.asset |> getName()
    query() <| $ [es] (composit__name : string; eid : EntityId; transform : float3x4)
      if composit__name != name
        return
      destroy_composit_tree(eid)
      create_composit_entity(name, transform)


[es(track=transform)]
def composit_move_track(evt : Event;
                        transform : float3x4;
                        var composit__prevTransform : float3x4;
                        composit__entities : EidList)
  let diff = transform * inverse(composit__prevTransform)

  for eid in composit__entities
    query(eid) <| $ [es] (var transform : float3x4)
      transform = diff * transform
  composit__prevTransform = transform

[es(on_appear, REQUIRE=composit__entities)]
def on_composit_created(evt : Event;
                        composit__name : string;
                        transform : float3x4;
                        var composit__prevTransform : float3x4&;
                        var composit__entities : EidList)
  composit__prevTransform = transform
  query() <| $ [es] (asset__manager : DagorAssetMgr)
    let asset = asset__manager |> findAsset(composit__name)
    composit_tree(asset__manager, composit__entities, asset.props, transform)

[es(on_appear)]
def create_composits_on_asset_manager_appear(evt : Event; asset__manager : DagorAssetMgr)
  query() <| $ [es] (composit__name : string; transform : float3x4; var composit__entities : EidList)
    let asset = asset__manager |> findAsset(composit__name)
    composit_tree(asset__manager, composit__entities, asset.props, transform)

[es(on_disappear, REQUIRE=composit__entities)]
def on_composit_deleted(evt : Event;
                        eid : EntityId)
  destroy_composit_tree(eid)
