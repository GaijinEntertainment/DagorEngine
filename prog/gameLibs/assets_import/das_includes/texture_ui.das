module texture_ui shared

require ecs
require imgui/imgui_boost
require daslib/static_let

require strings
require AssetsImport
require DagorDataBlock
require stddlg
require gameLibs.assets_import.das_includes.utility
require gameLibs.assets_import.das_includes.dag_ui

def texture_viewer()

  static_let <|
    var selected_tex_idx = 0

  get_all_assets("tex") <| $(importedTextures)
    if empty(importedTextures)
      imgui::Text("No imported textures.")
      return
    asset_combo("##select texture", importedTextures, selected_tex_idx)

  if imgui::Button("add new texture")
    let assetPath = get_dlg_open_file("", "png|jpg|tga|tif")
    if !empty(assetPath)
      let assetName = get_base_name_with_ext(assetPath)

      using() <| $(var texBlk : DataBlock)
        texBlk |> add("name", assetName)
        texBlk |> add("convert", true)
        texBlk |> datablock_save_to_text_file(replace_extension(assetPath, "tex.blk"))
