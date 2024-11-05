module collision_ui shared

require ecs
require imgui/imgui_boost

require strings
require AssetsImport
require DagorDataBlock
require stddlg
require danetlibs.assets_import.das_includes.utility

def collision_viewer()

  get_all_assets("collision") <| $(collision)
    let assetCount = length(collision)
    if (assetCount == 0)
      imgui::Text("No imported collision.")
      return
    imgui::Text("List of collisions:")
    for name, i in collision, iter_range(collision)
      imgui::Text("{i + 1}) {name}")

  imgui::NewLine()

  query() <| $ [es] (var asset__curCollisionPath : das_string&; asset__baseFolderAbsPath : string)

    imgui::Text("Add new collision")

    if imgui::Button("Select collision resource")
      get_local_path_to_dag(asset__baseFolderAbsPath, asset__curCollisionPath)
    imgui::Text("path: {asset__curCollisionPath}")

    if imgui::Button("Create collision asset")
      let assetPath = get_dlg_save_file("", "", "collision.blk")
      if !empty(assetPath)

        using() <| $(var assetBlk : DataBlock)
          assetBlk |> add("name", "{asset__curCollisionPath}")
          assetBlk |> datablock_save_to_text_file(assetPath)
