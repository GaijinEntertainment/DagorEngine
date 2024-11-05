require ecs
require DagorImgui
require imgui
require imgui/imgui_boost
require daslib/safe_addr
require danetlibs.assets_import.main.import_rendinst_common
require danetlibs.assets_import.main.asset_manager_events
require danetlibs.assets_import.das_includes.rendinst_ui
require danetlibs.assets_import.das_includes.texture_ui
require danetlibs.assets_import.das_includes.collision_ui
require danetlibs.assets_import.das_includes.animchar_ui
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
          broadcastEvent([[InvalidateClipmap]])
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
      spawn_rendinst()
      imgui::EndTabItem()

    if imgui::BeginTabItem("Composit")
      spawn_composit()
      imgui::EndTabItem()

    imgui::EndTabBar()