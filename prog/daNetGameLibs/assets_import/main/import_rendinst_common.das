module import_rendinst_common shared
require ecs
require DagorSystem
require DagorDataBlock
require DagorConsole
require RendInst
require AssetsImport
require danetlibs.assets_import.das_includes.utility




def find_rendinst(name : string) : DagorAsset?
  var found_asset : DagorAsset?
  find_query() <| $ [es] (asset__manager : DagorAssetMgr;
                          asset__rendInstType : int)
    found_asset = asset__manager |> findAsset(name, asset__rendInstType)
    return true
  return found_asset

def find_asset(name : string; asset_type : string) : DagorAsset?
  var found_asset : DagorAsset?
  find_query() <| $ [es] (asset__manager : DagorAssetMgr)
    found_asset = asset__manager |> findAsset(name, asset__manager |> getAssetTypeId(asset_type))
    return true
  return found_asset

def reload_rendinst_resources(ri_name : string)
  let riAsset = find_rendinst(ri_name)
  if riAsset == null
    logerr("Can't find asset <{ri_name}> in asset manger")
    return
  invalidate_asset(*riAsset, false)
  let collisionAsset = find_asset(ri_name + RI_COLLISION_RES_SUFFIX, "collision")
  if collisionAsset != null
    invalidate_asset(*collisionAsset, false)
  rendinst_reloadRIExtraResources(ri_name)


def import_rendinst(filename : string;
                    lodDistances : FloatList;
                    lodFiles : StringList)
  query() <| $ [es] (asset__baseFolderAbsPath : das_string)
    for lodFile in lodFiles
      iterate_dag_textures("{asset__baseFolderAbsPath}/{lodFile}") <| $(texPath : string)
        console_print("tex {texPath}")

    using() <| $(var riBlk : DataBlock)
      for lodDist, lodFile in lodDistances, lodFiles
        riBlk |> datablock_add_new_block("lod") <| $(lod)
          lod |> add("range", lodDist)
          lod |> add("fname", "{lodFile}")
      riBlk |> datablock_save_to_text_file("{filename}")
