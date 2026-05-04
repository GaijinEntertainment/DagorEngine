module utility shared
require daslib/strings_boost
require ecs
require Dacoll
require DagorSystem
require AssetsImport
require math.base
require stddlg

def get_base_name_with_ext(path : string)
  return path |> split_by_chars("/\\") |> back()

def get_base_name_without_ext(path : string) : string
  return (get_base_name_with_ext(path) |> split("."))[0]

def local_path(folder_path, file_path : string)

  let prefixIdx = file_path |> find(folder_path)
  if prefixIdx < 0
    return file_path
  return slice(file_path, prefixIdx + length(folder_path) + 1)

def replace_extension(path, ext : string)

  let dotIdx = path |> find(".")
  if dotIdx >= 0
    return "{slice(path, 0, dotIdx)}.{ext}"
  return path

def get_local_path_to_dag(root : string; var path : das_string | #) : bool

  let file = get_dlg_open_file("", "dag") |> replace("\\", "/")
  if !empty(file)
    path := local_path(root, file)
    return true
  return false

def get_all_assets(type_name : string; b : block<(arg1 : array<string> const) : void>)
  query() <| $ [es] (asset__manager : DagorAssetMgr)
    let typeId = asset__manager |> getAssetTypeId(type_name)
    if typeId < 0
      logerr("AssetManager: unsupported type {type_name}")
      return
    asset__manager |> getFilteredAssets(typeId) <| $(assetIds)
      let assetNames <- [for id in assetIds; asset__manager |> getAsset(id) |> getName()]
      invoke(b, assetNames)

def get_filtered_assets(type_name, filter : string; b : block<(arg1 : array<string> const) : void>)
  query() <| $ [es] (asset__manager : DagorAssetMgr)
    let typeId = asset__manager |> getAssetTypeId(type_name)
    if typeId < 0
      logerr("AssetManager: unsupported type {type_name}")
      return
    asset__manager |> getFilteredAssets(typeId) <| $(assetIds)
      var assetNames : array<string>
      for id in assetIds
        let assetName = asset__manager |> getAsset(id) |> getName()
        if empty(filter) || assetName |> find(filter, 0) >= 0
          assetNames |> push(assetName)
      invoke(b, assetNames)

def find_transform()
  var cameraTm = IDENT_TM
  let cameraWasFound = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if !camera__active
      return false
    cameraTm = transform
    return true
  if !cameraWasFound
    return IDENT_TM
  let cameraPos = cameraTm[3]
  let dir = cameraTm[2]
  var dist = 500.0f
  var norm : float3
  var transfrom = pos_tm(cameraPos)
  //return transform in far if no collision
  traceray_normalized(cameraPos, dir, dist, norm)
  transfrom[3] = cameraPos + dist * dir
  return transfrom

