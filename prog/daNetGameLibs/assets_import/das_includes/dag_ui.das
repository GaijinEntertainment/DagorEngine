module dag_ui shared

require ecs
require imgui/imgui_boost
require daslib/static_let
require strings
require danetlibs.assets_import.das_includes.utility
require AssetsImport
require DagorShaders
require DagorTexture3D

let textureIconSize = 75
let magnifiedTextureIconSize = 375

def edit_value(text : string; var value : das_string&)
  let buf <- ImGuiInputTextBuffer("{value}", 256)
  if InputText(text, buf, ImGuiInputTextFlags.AutoSelectAll)
    value := buf |> to_string()
    return true
  return false

def imgui_click_to_copy(val : string)
  if imgui::IsItemHovered()
    imgui::BeginTooltip()
    imgui::PushTextWrapPos(GetFontSize() * 35.0f)
    imgui::TextUnformatted("right click to copy {val}")
    imgui::PopTextWrapPos()
    imgui::EndTooltip()
    if imgui::IsMouseClicked(ImGuiMouseButton.Right)
      SetClipboardText("{val}")


def get_asset_status_color(status : AssetLoadingStatus)

  if status == AssetLoadingStatus.LoadedWithErrors
    return float4(1f, 0f, 0f, 1f)
  elif status == AssetLoadingStatus.Loading
    return float4(0f, 0f, 1f, 1f)
  elif status == AssetLoadingStatus.Loaded
    return float4(0f, 1f, 0f, 1f)
  else //status == AssetLoadingStatus NotLoaded
    return float4(1f, 1f, 1f, 1f)

def dag_tex_id_to_imgui(tex_id : D3DRESID&)
  unsafe
    return reinterpret<void?> tex_id

def open_file(path : string)
  //this shell call will open file with default application
  os_shell_execute("open", path, "", "")

def open_folder(path : string)
  //this replace need for windows explorer
  let windowsPath = path |> replace("/", "\\")
  //this shell call will open folder with this file and select file
  os_shell_execute("open", "explorer.exe", "/select,{windowsPath}", "")

def asset_dependencies_tree(var asset : DagorAsset?; asset__manager : DagorAssetMgr; is_root = true)

  let nodeOpened = imgui::TreeNodeEx("{asset.name}:{asset.typeName}##{asset}", is_root ? ImGuiTreeNodeFlags.DefaultOpen : ImGuiTreeNodeFlags.None)
  imgui_click_to_copy(*asset |> getSrcFilePath())
  let status = get_asset_status(*asset, false)
  imgui::SameLine()
  imgui::TextColored(get_asset_status_color(status), "[{status}]")

  if asset.typeName == "tex"

    let texId = status == AssetLoadingStatus.Loaded ? get_managed_texture_id("{asset.name}*") : BAD_TEXTUREID
    if texId != BAD_TEXTUREID
      imgui::SameLine()
      let imguiTexId = dag_tex_id_to_imgui(texId)
      imgui::PushStyleVar(ImGuiStyleVar.FramePadding, float2(0))
      imgui::ImageButton("{asset}_image", imguiTexId, float2(textureIconSize), float2(0), float2(1), float4(1), float4(1))
      imgui::PopStyleVar()

      if imgui::IsItemHovered()
        imgui::BeginTooltip()
        imgui::PushStyleVar(ImGuiStyleVar.FramePadding, float2(0))
        imgui::ImageButton("{asset}_image_magnified", imguiTexId, float2(magnifiedTextureIconSize), float2(0), float2(1), float4(1), float4(1))
        imgui::PopStyleVar()
        imgui::EndTooltip()

  if nodeOpened
    imgui::Text("Source files: ")
    get_asset_source_files(*asset) <| $(srcFiles)
      for file, idx in srcFiles, iter_range(srcFiles)
        imgui::Text("{idx}) {file}")
        imgui_click_to_copy(file)

        if imgui::Button("open file##{file}")
          open_file(file)
        imgui::SameLine()
        if imgui::Button("open folder##{file}")
          open_folder(file)

    get_asset_dependencies(*asset) <| $(assetsNames)
      for depName in assetsNames
        var depAsset = asset__manager |> findAsset(depName)
        if depAsset != null
          asset_dependencies_tree(depAsset, asset__manager, false)
        elif !empty(depName)
          imgui::TextColored(float4(1, 0, 0, 1), "{depName} broken ref")
    imgui::TreePop()
    imgui::Separator()



def view_dag_assets(assset_type : string; b : block<(arg1 : string const) : void>)
  static_let <|
    var selectedAssetIdx = 0

  query() <| $ [es] (var asset__currentSearchFilter : das_string&; asset__manager : DagorAssetMgr)
    edit_value("Filter:", asset__currentSearchFilter)

    get_filtered_assets(assset_type, string(asset__currentSearchFilter)) <| $(assetsNames)

      if selectedAssetIdx >= length(assetsNames)
        selectedAssetIdx = 0

      if (empty(assetsNames))
        imgui::Text("No assets {assset_type}")
        return
      else
        let firstName = assetsNames[selectedAssetIdx]
        if imgui::BeginCombo("##", firstName)
          for name, idx in assetsNames, iter_range(assetsNames)
            let isSelected = selectedAssetIdx == idx
            if imgui::Selectable(name, isSelected)
              selectedAssetIdx = idx

          imgui::EndCombo()
        imgui::SameLine()


        var asset = asset__manager |> findAsset(assetsNames[selectedAssetIdx])

        let name = *asset |> getName()
        let path = *asset |> getSrcFilePath()
        imgui::Spacing()
        imgui::Text("Asset name: {name}")
        imgui_click_to_copy(name)
        imgui::Spacing()

        imgui::Button("Copy path")
        imgui_click_to_copy(path)
        imgui::Spacing()


        let finalAssetStatus = get_asset_status(*asset, true)
        imgui::Text("Dependance assets: ")
        asset_dependencies_tree(asset, asset__manager)




        let spawn = imgui::Button("Spawn")
        imgui::SameLine()
        imgui::TextColored(get_asset_status_color(finalAssetStatus), "[{finalAssetStatus}]")
        if spawn
          invoke(b, assetsNames[selectedAssetIdx])