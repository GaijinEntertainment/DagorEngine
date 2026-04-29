options no_aot
require DagorConsole
require DagorSystem
require ecs
require AssetsImport
require gameLibs.assets_import.main.import_rendinst_common
require gameLibs.assets_import.main.asset_manager_events
require gameLibs.assets_import.das_includes.utility

[console_cmd(name="asset_manager.reloadRI")]
def reload_rendinst_cmd(ri_name : string)
  reload_rendinst_resources(ri_name)


[es]
def on_rendinst_added_debug(evt : AssetAddedEvent;
                            asset__rendInstType : int)
  if (evt.asset.typeId == asset__rendInstType)
    let name = *evt.asset |> getName()
    console_print("Rendinst {name} was added")

[es]
def on_rendinst_changed_debug(evt : AssetChangedEvent;
                              asset__rendInstType : int)
  if (evt.asset.typeId == asset__rendInstType)
    let name = *evt.asset |> getName()
    console_print("Rendinst {name} was changed")

[es]
def on_rendinst_removed_debug(evt : AssetRemovedEvent;
                              asset__rendInstType : int)
  if (evt.asset.typeId == asset__rendInstType)
    let name = *evt.asset |> getName()
    console_print("Rendinst {name} was removed")
