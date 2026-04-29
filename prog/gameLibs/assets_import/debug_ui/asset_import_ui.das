require ecs
require ecs.safe
require DagorImgui
require imgui
require imgui/imgui_boost
require daslib/safe_addr
require daslib/static_let
require gameLibs.assets_import.main.import_rendinst_common
require gameLibs.assets_import.main.asset_manager_events
require gameLibs.assets_import.das_includes.rendinst_ui
require gameLibs.assets_import.das_includes.texture_ui
require gameLibs.assets_import.das_includes.collision_ui
require gameLibs.assets_import.das_includes.animchar_ui
require AssetsImport



[imgui_window(name="Asset Import", group="Editor")]
def asset_import_viewer()
  if IsWindowCollapsed()
    return
  if !(find_query() <| $ [es(REQUIRE=asset__manager)] () => true)

    if imgui::Button("load whole develop/assets")
      createEntity("asset_manager") <| $(init)
        init |> set("asset__baseFolder", "../develop/assets")
    imgui::Text("It might take a long time to scanning all assets (half of minute).")

    let gameSpecificAssetManagersTemplate = getTemplateByName("game_specific_asset_managers")
    if gameSpecificAssetManagersTemplate != null
      let gameSpecificAssetManagers = get_ecs_array(getTemplateComponent(*gameSpecificAssetManagersTemplate, "game_specific_asset_managers"))
      for assetManager in *gameSpecificAssetManagers
        let assetManagerObj = assetManager as Object
        if imgui::Button(assetManagerObj?.buttonText ?? "")
          let assetManagerTemplate = assetManagerObj?["template"] ?? ""
          if !empty(assetManagerTemplate)
            createEntity(assetManagerTemplate)
        imgui::Text(assetManagerObj?.info ?? "")

    if imgui::Button("load custom folder")
      createEntity("asset_manager")
    imgui::Text("It will load folder from \"asset_manager\" template")

  query() <| $ [es(REQUIRE=asset__manager)] (var asset__clipmapInvalidationEnabled : bool&)

    imgui::BeginTabBar("Tabs")
    if imgui::BeginTabItem("Common")

      var clipmapInvalidation = asset__clipmapInvalidationEnabled
      if imgui::Checkbox("Enable clipmap invalidation on texture asset changes", safe_addr(clipmapInvalidation))
        asset__clipmapInvalidationEnabled = clipmapInvalidation
        if clipmapInvalidation
          broadcastEvent(InvalidateClipmap())
      imgui::EndTabItem()


    if imgui::BeginTabItem("Create Asset")
      create_new_rendinst_asset()
      imgui::EndTabItem()

    if imgui::BeginTabItem("Textures")
      texture_viewer()
      imgui::EndTabItem()

    if imgui::BeginTabItem("Collision")
      collision_viewer()
      imgui::EndTabItem()

    if imgui::BeginTabItem("Animchars")
      animchar_viewer()
      imgui::EndTabItem()

    if imgui::BeginTabItem("Rendinst")
      static_let <|
        var spawn_destructible_ri = false
      spawn_rendinst(spawn_destructible_ri ? "game_rendinst" : "rendinst")
      if getTemplateByName("game_rendinst") != null
        imgui::SameLine()
        imgui::Checkbox("Destructible", safe_addr(spawn_destructible_ri))
      imgui::EndTabItem()

    if imgui::BeginTabItem("Composit")
      spawn_composit()
      imgui::EndTabItem()

    imgui::EndTabBar()