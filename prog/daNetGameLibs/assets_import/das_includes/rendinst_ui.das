module rendinst_ui shared

require ecs
require ecs.extra_set
require EcsUtils
require math
require math.base
require imgui/imgui_boost
require daslib/static_let
require daslib/safe_addr
require daslib/strings_boost
require strings
require danetlibs.assets_import.main.import_rendinst_common
require danetlibs.assets_import.das_includes.utility
require danetlibs.assets_import.das_includes.dag_ui
require AssetsImport
require DagorEditor
require DagorDataBlock
require stddlg

def create_rendinst(name : string; transform : float3x4; template_name : string)
  return createEntity(template_name) <| $(init)
    set(init, "transform", transform)
    set(init, "ri_extra__name", name)

def spawn_rendinst()
  view_dag_assets("rendInst") <| $(assetName)
    init_entity_object_editor()
    let callback <- @ <| (eid : EntityId)
      scene_insert_empty_entity_record(get_active_scene(), eid, "rendinst")
      entity_object_editor_addEntityWithUndo(eid)
      entity_obj_editor_saveComponent(eid, "transform")
      entity_obj_editor_saveComponent(eid, "ri_extra__name")
    createEntity("rendinst", callback) <| $(init)
      set(init, "transform", find_transform())
      set(init, "ri_extra__name", assetName)

def create_gameobj(transform : float3x4; template_name : string)
  if getTemplateByName(template_name) == null
    return INVALID_ENTITY_ID

  let finalTemplateName = add_sub_template_name(template_name, "game_object");
  let eid = createEntity(finalTemplateName) <| $(init)
    set(init, "transform", transform)

  return eid


def create_new_rendinst_asset()
  query() <| $ [es] (asset__baseFolderAbsPath : string;
                     var activeRI__lodDistances : FloatList;
                     var activeRI__lodFiles : StringList)
    var removeLod = -1
    let lodsCount = length(activeRI__lodDistances)

    for lodDist, lodFile, lodNo in activeRI__lodDistances, activeRI__lodFiles, range(lodsCount)
      imgui::Text("lod {lodNo}")
      if empty(lodFile)
        imgui::TextColored(float4(1.0, 0.0, 0.0, 1.0), "empty path")
      //else if () add check of invalid path
      else
        imgui::Text("{lodFile}")
      imgui::SameLine()
      if imgui::Button("change source file##{lodNo}")
        get_local_path_to_dag(asset__baseFolderAbsPath, lodFile)

      if lodNo == lodsCount - 1
        imgui::SameLine()
        if imgui::Button("remove LOD##{lodNo}")
          removeLod = lodNo

      var tmpValue = lodDist
      imgui::InputFloat("LOD distance##{lodNo}", safe_addr(tmpValue))
      lodDist = tmpValue

    if removeLod >= 0
      activeRI__lodDistances |> erase(removeLod)
      activeRI__lodFiles |> erase(removeLod)

    if imgui::Button("add LOD")
      using() <| $(var lodFile : das_string)
        if get_local_path_to_dag(asset__baseFolderAbsPath, lodFile)
          activeRI__lodDistances |> push(100f)
          activeRI__lodFiles |> push(lodFile)

    imgui::NewLine()
    var importedRI = false

    if length(activeRI__lodDistances) > 0
      if imgui::Button("save asset")
        let assetPath = get_dlg_save_file("", "", "rendinst.blk")
        if !empty(assetPath)
          get_base_name_without_ext(assetPath)
          importedRI = true
          import_rendinst(assetPath, activeRI__lodDistances, activeRI__lodFiles)
      imgui::NewLine()

    if imgui::Button("clear") || importedRI
      activeRI__lodDistances |> clear()
      activeRI__lodFiles |> clear()


def composit_tree(asset__manager : DagorAssetMgr; var entities : EidList; node : DataBlock; transform : float3x4)

  let nodeName = datablock_getStr(node, "name", "")
  let localTransform = datablock_getTm(node, "tm", IDENT_TM)
  let nodeTransform = transform * localTransform

  if nodeName != ""
    let typeParsed <- nodeName |> split(":")
    let assetName = typeParsed[0]
    let assetType = length(typeParsed) > 1 ? typeParsed[1] : ""
    let typeIdx = asset__manager |> getAssetTypeId(assetType)

    let asset = typeIdx >= 0 ? asset__manager |> findAsset(assetName, typeIdx) : asset__manager |> findAsset(assetName)
    if asset != null //it's valid situation

      let nodeType = (*asset).typeName
      if nodeType == "rendInst"
        entities |> push(create_rendinst(assetName, nodeTransform, "composit_child_rendinst"))
      elif nodeType == "composit"
        entities |> push(create_composit_entity(assetName, nodeTransform, true))
      elif nodeType == "gameObj"
        let eid = create_gameobj(nodeTransform, assetName)
        if eid != INVALID_ENTITY_ID
          entities |> push(eid)


  datablock_for_each_block(node, "node") <| $(blk)
    composit_tree(asset__manager, entities, blk, nodeTransform)



def create_composit_entity(name : string; transform : float3x4; nested : bool = false)
  let initList = $ <| (var init : ComponentsInitializer)
    set(init, "transform", transform)
    set(init, "composit__name", name)

  if nested
    return createEntity("composit_rendinst", initList)
  else
    init_entity_object_editor()
    // sync creation is required to correctly save components for f12 ecs object editor
    let eid = createEntitySync("composit_rendinst", initList)
    scene_insert_empty_entity_record(get_active_scene(), eid, "composit_rendinst")
    entity_object_editor_addEntityWithUndo(eid)
    entity_obj_editor_saveComponent(eid, "transform")
    entity_obj_editor_saveComponent(eid, "composit__name")
    return eid


def spawn_composit()
  view_dag_assets("composit") <| $(assetName)

    create_composit_entity(assetName, find_transform())

