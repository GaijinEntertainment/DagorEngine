module animchar_ui shared

require ecs
require imgui/imgui_boost
require danetlibs.assets_import.das_includes.utility
require danetlibs.assets_import.das_includes.dag_ui


def animchar_viewer()


  view_dag_assets("animChar") <| $(assetName)

    createEntity("custom_animchar") <| $(init)
      set(init, "transform", find_transform())
      set(init, "animchar__res", assetName)
//TODO : add all files generation
