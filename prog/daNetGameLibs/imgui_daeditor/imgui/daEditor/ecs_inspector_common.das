module ecs_inspector_common shared
require ecs
require ecs.common
require imgui
require DagorImgui
require DagorConsole
require DagorSystem
require DagorMath
require DagorEditor
require DagorDataBlock
require DagorDebug3D
require EcsUtils
require imgui/imgui_boost
require strings
require strings_boost
require math.base
require daslib/static_let
require daslib/safe_addr
require daslib/instance_function
require daslib/defer
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require danetlibs.imgui_daeditor.daEditor.daEditor_common
require %danetlibs.imgui_daeditor.imgui.imgui_common


[unused_argument(eid)]
def default_menu(eid : array<EntityId> implicit; name, typeName : string; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>)
  imgui::TableSetColumnIndex(2)
  if imgui::Button("*##{name}")
    imgui::OpenPopup("defaultSubMenuPopup_{name}")

  var res = true
  if imgui::BeginPopup("defaultSubMenuPopup_{name}")

    if imgui::Button("Copy##copyname")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(name)
    imgui::SameLine()
    imgui::Text(name)

    if imgui::Button("Copy##copytypename")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(typeName)
    imgui::SameLine()
    imgui::Text(typeName)

    res = sub_menu |> invoke(changed)

    imgui::EndPopup()
  return res

[unused_argument(eid)]
def default_menu(eid : array<EntityId> implicit;
                 name, typeName : string;
                 val_to_str : block<() : string>;
                 str_to_val : block<(str : string) : void>;
                 blk_val_to_str : block<() : string>;
                 var changed : bool?;
                 sub_menu : block<(var changed : bool?) : bool>)
  imgui::TableSetColumnIndex(2)
  if imgui::Button("*##{name}")
    imgui::OpenPopup("defaultSubMenuPopup_{name}")

  var res = true
  if imgui::BeginPopup("defaultSubMenuPopup_{name}")

    if imgui::Button("Copy##name")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(name)
    imgui::SameLine()
    imgui::Text(name)

    if imgui::Button("Copy##value")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(typeName)
    imgui::SameLine()
    imgui::Text(typeName)

    if imgui::Button("Copy value")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(val_to_str |> invoke())
    if imgui::Button("Paste value")
      imgui::CloseCurrentPopup()
      str_to_val |> invoke(imgui::GetClipboardText())
    if imgui::Button("Copy blk value")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(blk_val_to_str |> invoke())

    res = sub_menu |> invoke(changed)

    imgui::EndPopup()
  return res

def to_string(val : DPoint3)
  return "{val.x}, {val.y}, {val.z}"

def to_string(val)
  return "{val}"

def to_string(val : float3x4)
  return "[[{val[0]}] [{val[1]}] [{val[2]}] [{val[3]}]]"

[no_aot]
def to_string_via_blk(compName : string; eid : EntityId)
  var data = ""
  using() <| $(var blk : DataBlock)
    let mgr & = getEntityManager()
    component_to_blk_param(compName, mgr |> getComponentRef(eid, compName), blk)
    data := string(blk)
  return data

def to_int2(str : string)
  let parts <- split(str, ",")
  return int2(length(parts) > 0 ? to_int(strip(parts[0])) : 0,
              length(parts) > 1 ? to_int(strip(parts[1])) : 0)

def to_int3(str : string)
  let parts <- split(str, ",")
  return int3(length(parts) > 0 ? to_int(strip(parts[0])) : 0,
              length(parts) > 1 ? to_int(strip(parts[1])) : 0,
              length(parts) > 2 ? to_int(strip(parts[2])) : 0)

def to_int4(str : string)
  let parts <- split(str, ",")
  return int4(length(parts) > 0 ? to_int(strip(parts[0])) : 0,
              length(parts) > 1 ? to_int(strip(parts[1])) : 0,
              length(parts) > 2 ? to_int(strip(parts[2])) : 0,
              length(parts) > 3 ? to_int(strip(parts[3])) : 0)

def to_float2(str : string)
  let parts <- split(str, ",")
  return float2(length(parts) > 0 ? to_float(strip(parts[0])) : 0.0,
                length(parts) > 1 ? to_float(strip(parts[1])) : 0.0)

def to_float3(str : string)
  let parts <- split(str, ",")
  return float3(length(parts) > 0 ? to_float(strip(parts[0])) : 0.0,
                length(parts) > 1 ? to_float(strip(parts[1])) : 0.0,
                length(parts) > 2 ? to_float(strip(parts[2])) : 0.0)

def to_float4(str : string)
  let parts <- split(str, ",")
  return float4(length(parts) > 0 ? to_float(strip(parts[0])) : 0.0,
                length(parts) > 1 ? to_float(strip(parts[1])) : 0.0,
                length(parts) > 2 ? to_float(strip(parts[2])) : 0.0,
                length(parts) > 3 ? to_float(strip(parts[3])) : 0.0)

def to_DPoint3(str : string)
  let parts <- split(str, ",")
  return DPoint3(length(parts) > 0 ? double(strip(parts[0])) : 0.0lf,
                 length(parts) > 1 ? double(strip(parts[1])) : 0.0lf,
                 length(parts) > 2 ? double(strip(parts[2])) : 0.0lf)

def to_TMatrix(str : string)
  var value : float3x4
  let packedStr = str |> replace(" ", "")
  if packedStr |> starts_with("[[") && packedStr |> ends_with("]]")
    let rows <- split(packedStr |> chop(2, length(packedStr) - 4), "][")
    value[0] = length(rows) > 0 ? to_float3(rows[0]) : float3()
    value[1] = length(rows) > 1 ? to_float3(rows[1]) : float3()
    value[2] = length(rows) > 2 ? to_float3(rows[2]) : float3()
    value[3] = length(rows) > 3 ? to_float3(rows[3]) : float3()
  return value

// [[ 138.485, 99.9427, -648.716, 1; 142.237, 103.802, -645.125, 1]]
def to_bbox3f(str : string)
  var value : bbox3f
  let packedStr = str |> replace(" ", "")
  if packedStr |> starts_with("[[") && packedStr |> ends_with("]]")
    let rows <- split(packedStr |> chop(2, length(packedStr) - 4), ";")
    value.bmin = length(rows) > 0 ? to_float4(rows[0]) : float4()
    value.bmax = length(rows) > 1 ? to_float4(rows[1]) : float4()
  return value


def update_value(new_val;
                 var val_ptr;
                 var changed : bool?)
  *changed ||= (*val_ptr != new_val)
  *val_ptr = new_val

def clone_value(new_val;
                var val_ptr;
                var changed : bool?)
  *changed ||= (*val_ptr != new_val)
  *val_ptr := new_val

def unique_key(keys : array<string>; name = "key")
  var key = name
  if length(keys) == 0
    return key

  for _ in range(10)
    var found = false
    for idx in iter_range(keys)
      if keys[idx] == key
        found = true
    if !found
      break
    key = "_{key}"

  return key

def getEntityComponentRef(val : Array; idx)
  return val[idx] |> getEntityComponentRef()

[generic]
def use_this(blk : block<(val : T#) : void>) { using(blk); }
def use_this(blk : block<(val : int#) : void>) { blk |> invoke(0); }
def use_this(blk : block<(val : uint8#) : void>) { blk |> invoke(uint8(0u)); }
def use_this(blk : block<(val : uint16#) : void>) { blk |> invoke(uint16(0u)); }
def use_this(blk : block<(val : uint64#) : void>) { blk |> invoke(uint64(0u)); }
def use_this(blk : block<(val : string#) : void>) { var str : string# = ""; blk |> invoke(str); }
def use_this(blk : block<(val : das_string#) : void>) { using() <| $(var str : das_string) { blk |> invoke(unsafe(reinterpret<das_string#> str)); } }
def use_this(blk : block<(val : EntityId#) : void>) { blk |> invoke(INVALID_ENTITY_ID); }
def use_this(blk : block<(val : float#) : void>) { blk |> invoke(0f); }
def use_this(blk : block<(val : float2#) : void>) { blk |> invoke(float2()); }
def use_this(blk : block<(val : float3#) : void>) { blk |> invoke(float3()); }
def use_this(blk : block<(val : float4#) : void>) { blk |> invoke(float4()); }
def use_this(blk : block<(val : int2#) : void>) { blk |> invoke(int2()); }
def use_this(blk : block<(val : int3#) : void>) { blk |> invoke(int3()); }
def use_this(blk : block<(val : float3x4#) : void>) { var tm : float3x4# = IDENT_TM; blk |> invoke(tm); }
def use_this(blk : block<(val : bool#) : void>) { blk |> invoke(false); }
def use_this(blk : block<(val : E3DCOLOR#) : void>) { blk |> invoke(E3DCOLOR(0x0)); }
def use_this(blk : block<(val : int64#) : void>) { blk |> invoke(0l); }

// TODO: move this to ecs_inspector annotation
def is_cont_type(user_type : uint) : bool
  let typeName = getComponentTypes() |> component_types_findTypeName(user_type)
  return is_cont_type(typeName)

def is_cont_type(typeName : string) : bool
  return (
         typeName == "ecs::Object"
      || typeName == "ecs::Array"
      || typeName == "ecs::IntList"
      || typeName == "ecs::UInt8List"
      || typeName == "ecs::UInt16List"
      || typeName == "ecs::StringList"
      || typeName == "ecs::EidList"
      || typeName == "ecs::FloatList"
      || typeName == "ecs::Point2List"
      || typeName == "ecs::Point3List"
      || typeName == "ecs::Point4List"
      || typeName == "ecs::IPoint2List"
      || typeName == "ecs::IPoint3List"
      || typeName == "ecs::BoolList"
      || typeName == "ecs::TMatrixList"
      || typeName == "ecs::ColorList"
      || typeName == "ecs::Int64List"
      || typeName == "ecs::SharedComponent< ::ecs::Object>"
      || typeName == "ecs::SharedComponent< ::ecs::Array>"
      || typeName == "ecs::SharedComponent< ::ecs::Object>"
      || typeName == "ecs::SharedComponent< ::ecs::Array>"
      || typeName == "ecs::SharedComponent< ::ecs::IntList>"
      || typeName == "ecs::SharedComponent< ::ecs::UInt8List>"
      || typeName == "ecs::SharedComponent< ::ecs::UInt16List>"
      || typeName == "ecs::SharedComponent< ::ecs::StringList>"
      || typeName == "ecs::SharedComponent< ::ecs::EidList>"
      || typeName == "ecs::SharedComponent< ::ecs::FloatList>"
      || typeName == "ecs::SharedComponent< ::ecs::Point2List>"
      || typeName == "ecs::SharedComponent< ::ecs::Point3List>"
      || typeName == "ecs::SharedComponent< ::ecs::Point4List>"
      || typeName == "ecs::SharedComponent< ::ecs::IPoint2List>"
      || typeName == "ecs::SharedComponent< ::ecs::IPoint3List>"
      || typeName == "ecs::SharedComponent< ::ecs::BoolList>"
      || typeName == "ecs::SharedComponent< ::ecs::TMatrixList>"
      || typeName == "ecs::SharedComponent< ::ecs::ColorList>"
      || typeName == "ecs::SharedComponent< ::ecs::Int64List>"
    )


def compare_eids(a, b : array<EntityId>) : int
  let na = length(a)
  let nb = length(b)
  if na != nb
    return na < nb ? -1 : 1

  for x, y in a, b
    if x != y
      return uint(x) < uint(y) ? -1 : 1

  return 0

def render_item(eid : array<EntityId> implicit; flags : uint; name : string implicit; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool? implicit; sub_menu : block<(var changed : bool?) : bool>) : void
  let typeName = refs[0].typeName
  render_item(eid, flags, name, typeName, refs, level, changed, sub_menu)

[no_aot]
def render_item(eid : array<EntityId> implicit; flags : uint; name, type_name : string implicit; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool? implicit; sub_menu : block<(var changed : bool?) : bool>) : void
  var hasInspector = false
  if has_inspector(type_name)
    hasInspector = call_inspector(type_name, eid, flags, name, refs, level, changed, sub_menu)
  if !hasInspector && refs[0].roVal != ""
    hasInspector = get_ecs_enum_values(type_name) <| $(enumValues)
      let mgr & = getEntityManager()
      let compRef = mgr |> getEntityComponentRef(eid[0], refs[0].idx)
      var curItem = find_enum_idx(type_name, compRef)
      imgui::Text("{type_name} {enumValues[curItem]}")
      if ListBox("##{name}", safe_addr(curItem), enumValues)
        update_enum_value(type_name, unsafe(reinterpret<EntityComponentRef> refs[0]), curItem)
        *changed = true

  if !hasInspector
    imgui::Text("{type_name}")
    default_menu(eid, name, type_name, changed) <| $ [unused_argument(sub_changed)] (sub_changed)

      if imgui::Button("copy type name##{name}")
        imgui::CloseCurrentPopup()
        imgui::SetClipboardText("{refs[0].typeName}")

      return sub_menu |> invoke(changed)


def imgui_hint(desc : string implicit)
  imgui_hint <| $
    return desc

def imgui_hint(desc : block<() : string implicit>)
  if IsItemHovered()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(invoke(desc))
    PopTextWrapPos()
    EndTooltip()


def DasHelpMarker(desc : string)
  TextDisabled("(?)")
  if IsItemHovered()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(desc)
    PopTextWrapPos()
    EndTooltip()


def item_flags(flags : uint; name : string implicit) : string
  let tracked = (flags & FLAG_CHANGE_EVENT) != 0u
  let replicated = (flags & FLAG_REPLICATED) != 0u
  return build_string <| $(str)
    if tracked
      str |> write("[t]")
    if replicated
      str |> write("[r]")
    if tracked || replicated
      str |> write(" ")
    str |> write(name)


def match(arr : array<string>; str : string) : bool
  for s in arr
    if find(str, s) >= 0
      return true
  return false

def open_inspector(idx : int; eid : EntityId; filter_str : string)
  if idx < 1 || idx > 3
    logerr("wrong inspector idx {idx}. Expected values are 1, 2, 3")

  imgui_set_blk_val("ecs_inspector_{idx}", "eid", int(uint(eid)))
  imgui_set_blk_val("ecs_inspector_{idx}", "filter", filter_str)
  imgui_window_set_visible("Editor", "Inspector {idx+1}", true)

def das_type_to_ecs_type_str(typ : rtti::Type)
  if typ == Type tBool
    return "bool"
  if typ == Type tInt8
    return "int8_t"
  if typ == Type tUInt8
    return "uint8_t"
  if typ == Type tUInt16
    return "uint16"
  if typ == Type tInt64
    return "int64_t"
  if typ == Type tUInt64
    return "uint64_t"
  if typ == Type tInt
    return "int"
  if typ == Type tInt2
    return "IPoint2"
  if typ == Type tInt3
    return "IPoint3"
  if typ == Type tInt4
    return "IPoint4"
  if typ == Type tFloat
    return "float"
  if typ == Type tFloat2
    return "Point2"
  if typ == Type tFloat3
    return "Point3"
  if typ == Type tFloat4
    return "Point4"
  if typ == Type tString
    return "ecs::string"
  return "UnknownBindedStructureFieldType"

def collect_struct_components(prefix : string; data : uint8?; type_name : string; var comps : array<tuple<name : string; cri : ComponentRenderInfo>>; var annotations : table<string; rtti::BasicStructureAnnotation?>)
  let ann = annotations |> get_value(type_name)
  if ann == null
    return

  *ann |> basic_struct_for_each_field <| $ [unused_argument(cppName)] (field_name, cppName, ti, offset)
    let field_data = unsafe(data + offset)
    if ti.isPod || ti.isConst
      let typ = das_type_to_ecs_type_str(ti.basicType)
      var roVal = ""
      if typ == "UnknownBindedStructureFieldType"
        roVal = "{sprint_data(field_data, unsafe(addr(ti)), [[print_flags]])}"
      comps |> emplace([[auto "{prefix}.{field_name}", ComponentRenderInfo(rawData=field_data, roVal=roVal, dasTypeName="{ti.basicType}", typeName=typ, idx=0u, componentId=uint16(0))]])
    elif ti.isRefType && ti.annotation != null
      collect_struct_components("{prefix}.{field_name}", field_data, "{ti.annotation.name}", comps, annotations)

[no_aot]
def inspect_item(uid : int; var filter : ImGuiTextFilter; eids : array<EntityId>#; new_selection : bool)
  if length(eids) == 0
    return
  static_let <|
    var selectedItem = 0
    var inflateListNum = 0
  if new_selection || selectedItem >= length(eids)
    selectedItem = 0
    inflateListNum = 0

  let eid = eids[selectedItem]

  let templateName = getEntityTemplateName(eid)
  let TEXT_BASE_WIDTH = CalcTextSize("A").x

  PushID(uid)

  if imgui::Button("*##{templateName}")
    imgui::OpenPopup("template_menu_{templateName}")

  SameLine()
  if imgui::Button("X##{templateName}")
    editor_open()
    editor_select_eid(INVALID_ENTITY_ID)
    editor_open(false)
  imgui_hint("Deselect enitity")

  if imgui::BeginPopup("template_menu_{templateName}")

    for i in range(1, 4)
      if i > 1
        imgui::SameLine()
      if imgui::Button("Inspector #{i+1}##{i+1}")
        imgui::CloseCurrentPopup()
        open_inspector(i, eid, imgui_get_blk_val("ecs_inspector_{uid}", "filter", ""))

    if imgui::Button("copy eid")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText("{eid}")

    imgui::SameLine()
    if imgui::Button("copy blk")
      imgui::CloseCurrentPopup()
      using() <| $(var blk : DataBlock)
        blk |> datablock_add_new_block("entity") <| $(sub)
          var tName = getEntityTemplateName(eid)
          tName = remove_sub_template_name(tName, "daeditor_selected")
          eid |> components_to_blk(sub, "", false)
          sub |> add("_template", tName)
        imgui::SetClipboardText(string(blk))

    imgui::SameLine()
    if imgui::Button("copy blk diff")
      imgui::CloseCurrentPopup()
      using() <| $(var blk : DataBlock)
        editor_save_entity(eid, blk)
        imgui::SetClipboardText(string(blk))

    var port = 0
    dgs_get_settings() |> datablock_get_block("debug") <| $(blk)
      port = blk |> datablock_getInt("WebUIPort", 23456)
    let fileName = "{eid}_p{port}.txt"
    if imgui::Button("Serialize to {fileName}")
      imgui::CloseCurrentPopup()
      console_command("ecs.serialize_entity {eid} {fileName}")

    imgui::Separator()

    let fullTmplText = templateName |> replace("+daeditor_selected+", "+") |> replace("+daeditor_selected", "")
    if imgui::Button("copy")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(fullTmplText)

    imgui::SameLine()
    imgui::Text(fullTmplText)


    let names <- split_template(templateName)
    for idx, n in iter_range(names), names
      if imgui::Button("copy##{idx}")
        imgui::CloseCurrentPopup()
        imgui::SetClipboardText(n)
      imgui::SameLine()
      imgui::Text(n)

    imgui::Separator()

    if imgui::Button("Destroy entity")
      imgui::CloseCurrentPopup()
      destroyEntity(eid)

    imgui::EndPopup()

  imgui::SameLine()
  if length(eids) == 1
    imgui::TextUnformatted("{eid}: {templateName}")
    imgui_hint(templateName)
  else
    if BeginCombo("##selectEid", "{eid}: {templateName}", ImGuiComboFlags None) //  NoArrowButton
      for idx, e in iter_range(eids), eids
        var selected = idx == selectedItem
        var selectedAddr = safe_addr(selected)
        if imgui::Selectable("{e}: {getEntityTemplateName(e)}", selectedAddr)
          selectedItem = idx // selected eid will be changed in next frame
      EndCombo()

  imgui::Separator()
  call_editor(/*prefix*/true, eids)

  DasHelpMarker("Filter (\"incl,-excl\")\nt:typeName (t:EntityId, t:EidList)")
  imgui::SameLine()
  SetNextItemWidth(-55.)

  let mgr & = getEntityManager()
  let dataComps & = getDataComponents()
  let compTypes & = getComponentTypes()

  filter |> set() <| imgui_get_blk_val("ecs_inspector_{uid}", "filter", "")

  filter |> Draw("##{eid}")

  SameLine()
  if imgui::Button("Clear")
    filter |> Clear()

  imgui_set_blk_val("ecs_inspector_{uid}", "filter", to_string(filter))

  imgui::BeginChild("sub", float2(0, 0))
  let tmp = getTemplateByName(templateName)
  if BeginTable("split", 3, ImGuiTableFlags Resizable | ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    TableSetupColumn("name", ImGuiTableColumnFlags WidthStretch)
    TableSetupColumn("value", ImGuiTableColumnFlags WidthStretch)
    TableSetupColumn("menu", ImGuiTableColumnFlags WidthFixed, TEXT_BASE_WIDTH * 2f)

    // TODO: We can collect theese once per hot reload and store in a global variable to avoid extra work.
    var basicStructAnnotations : table<string; rtti::BasicStructureAnnotation?>
    program_for_each_registered_module() <| $(mod)
      mod |> module_for_each_annotation <| $(ann)
        if ann.isBasicStructureAnnotation
          unsafe
            basicStructAnnotations |> insert("{ann.name}") <| addr(reinterpret<BasicStructureAnnotation> ann)

    let componentsNum = mgr |> getNumComponents(eid)
    var inscope comps : array<tuple<name : string; cri : ComponentRenderInfo>>
    for idx in urange(componentsNum >= 0 ? componentsNum : 0)
      let compRef = mgr |> getEntityComponentRef(eid, idx)
      let compTypeName = compTypes |> component_types_findTypeName(compRef.userType)
      let name = dataComps |> data_components_getComponentNameById(compRef.componentId)
      if basicStructAnnotations |> key_exists(compTypeName) && !has_inspector(compTypeName)
        let t = compTypes |> component_types_getTypeInfo(compRef.typeId)
        let data = (uint(t.flags) & uint(ComponentTypeFlags COMPONENT_TYPE_BOXED)) != 0u ? *unsafe(reinterpret<uint8 ??> compRef.rawData) : unsafe(reinterpret<uint8?> compRef.rawData)
        collect_struct_components(name, data, compTypeName, comps, basicStructAnnotations)

      let comp = dataComps |> data_components_getComponentById(compRef.componentId)
      if (uint(comp.flags) & uint(IS_COPY)) == 0u
        comps |> emplace([[auto name, ComponentRenderInfo(rawData=compRef.rawData, roVal="", dasTypeName="", typeName=compTypeName, idx=idx, componentId=compRef.componentId)]])

    comps |> sort() <| $(left; right)
      return left.name < right.name

    var inscope filters : array<string>
    var inscope excludeFilters : array<string>
    var inscope typeFilters : array<string>
    var inscope excludeTypeFilters : array<string>
    for filterRange in filter.Filters
      if filterRange |> empty()
        continue
      let filterString = string(filterRange)
      if filterString |> starts_with("t:") && length(filterString) > 2
        typeFilters |> push(unsafe(slice(filterString, 2) |> strip() |> to_lower_in_place()))
      elif filterString |> starts_with("-t:") && length(filterString) > 3
        excludeTypeFilters |> push(unsafe(slice(filterString, 3) |> strip() |> to_lower_in_place()))
      elif filterString |> starts_with("-") && length(filterString) > 1
        excludeFilters |> push(unsafe(slice(filterString, 1) |> strip() |> to_lower_in_place()))
      else
        filters |> push(unsafe(filterString |> to_lower_in_place()))

    var skipItems = max(0, floori(GetScrollY() / imgui::GetTextLineHeightWithSpacing()) - inflateListNum)
    var showItems = ceili(GetWindowHeight() / imgui::GetTextLineHeightWithSpacing()) + inflateListNum
    inflateListNum = 0
    for c in comps
      if showItems <= 0
        TableNextRow()
        TableNextColumn()
        imgui::Text("1")
        TableNextColumn()
        continue

      let name = c.name
      let lowerName = name |> to_lower()
      if (length(filters) > 0 && !match(filters, lowerName)) || (length(excludeFilters) > 0 && match(excludeFilters, lowerName))
        // if skipItems > 0
        //   skipItems -= 1
        continue

      let typeName = c.cri.typeName
      let lowerTypeName = typeName |> to_lower()
      if (length(typeFilters) > 0 && !match(typeFilters, lowerTypeName)) || (length(excludeTypeFilters) > 0 && match(excludeTypeFilters, lowerTypeName))
        // if skipItems > 0
        //   skipItems -= 1
        continue

      if skipItems > 0
        skipItems -= 1
        TableNextRow()
        TableNextColumn()
        imgui::Text("0")
        TableNextColumn()
        continue

      let flags = tmp != null && !empty(name) ? getRegExpInheritedFlags(mgr, tmp, name) : 0u
      let idxBefore = TableGetRowIndex()
      if !is_cont_type(typeName)
        TableNextRow()
        TableNextColumn()
        imgui::Text(item_flags(flags, name))
        imgui_hint(name)
        TableNextColumn()

      var changed = false
      let refs <- [{auto[] clone(c.cri) }]
      render_item(eids, flags, name, typeName, unsafe(reinterpret<array<ComponentRenderInfo#>#> refs), 0, safe_addr(changed)) <| $ [unused_argument(_)] (_) => true
      if changed
        entity_obj_editor_saveComponent(eid, name)
        invalidate_item(eid, unsafe(reinterpret<array<ComponentRenderInfo#>#> refs))

      let rowsNum = TableGetRowIndex() - idxBefore
      inflateListNum += rowsNum > 1 ? rowsNum + 5 : 0 // + 5 works, maybe + 10 works too
      showItems -= rowsNum

    EndTable()

  call_editor(/*prefix*/false, eids)
  imgui::EndChild()

  PopID()


[no_aot]
def invalidate_item(eid : EntityId; refs : array<ComponentRenderInfo#>#)
  for ref in refs
    if ref.roVal != ""
      continue
    nada(getEntityManagerRW() |> getComponentRefRW(eid, ref.componentId))


[sideeffects, unused_argument(ref)]
def nada(var ref : EntityComponentRef)
  pass

