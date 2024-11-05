module texture_ui shared

require ecs
require imgui/imgui_boost
require daslib/static_let

require strings
require AssetsImport
require DagorDataBlock
require stddlg
require danetlibs.assets_import.das_includes.utility

def texture_viewer()

  static_let <|
    var selected_tex_idx = 0

  get_all_assets("tex") <| $(importedTextures)
    let assetCount = length(importedTextures)
    if (assetCount == 0)
      imgui::Text("No imported textures.")
      return

    let selectedName = importedTextures[selected_tex_idx]
    if imgui::BeginCombo("##select texture", selectedName)
      for name, idx in importedTextures, iter_range(importedTextures)
        let isSelected = selected_tex_idx == idx
        if imgui::Selectable(name, isSelected)
          selected_tex_idx = idx
      imgui::EndCombo()
    imgui::SameLine()

    if imgui::Button("Edit")
      pass


  if imgui::Button("add new texture")
    let assetPath = get_dlg_open_file("", "png|jpg|tga|tif")
    if !empty(assetPath)
      let assetName = get_base_name_with_ext(assetPath)

      using() <| $(var texBlk : DataBlock)
        texBlk |> add("name", assetName)
        texBlk |> add("convert", true)
        texBlk |> datablock_save_to_text_file(replace_extension(assetPath, "tex.blk"))
