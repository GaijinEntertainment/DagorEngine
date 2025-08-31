options no_aot
options stack = 65536

require ecs
require EcsUtils
require imgui
require DagorImgui
require DagorSystem
require DagorMath
require DagorDebug3D
require imgui/imgui_boost
require daslib.strings_boost
require daslib/safe_addr
require daslib/instance_function
require daslib/defer
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require danetlibs.imgui_daeditor.daEditor.daEditor_common

def getComponentRenderInfoFromComponentRef(compRef : EntityComponentRef)
  unsafe
    let compTypes & = getComponentTypes()
    let compTypeName = compTypes |> component_types_findTypeName(compRef.userType)
    return <- ComponentRenderInfo(rawData = compRef.rawData, roVal = "", dasTypeName = "", typeName = compTypeName, idx = 0u, componentId = compRef.componentId, isValidComponent = true)

[ecs_inspector(type_name="bbox3f")]
def draw_bbox3f(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<bbox3f?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}min", unsafe(addr((val.bmin).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}max", unsafe(addr((val.bmax).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "bbox3f", $() => to_string(*val),
               $(str) { let newValue = to_bbox3f(str); val.bmin = newValue.bmin; val.bmax = newValue.bmax; *changed = true; },
               $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="BBox3")]
def draw_bbox3(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<BBox3?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}min", unsafe(addr(val.boxMin.x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}max", unsafe(addr((val.boxMax).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "BBox3", $() => to_string(val),
               $ [unused_argument(_)] (_) { logerr("Not implemented"); },
               $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="UnknownBindedStructureFieldType")]
def draw_unknown_binded_struct_field(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  imgui::Text("({refs[0].dasTypeName}): {refs[0].roVal}")
  imgui_hint("No inspector available for this ecs type: {refs[0].dasTypeName}.\nDisplaying read only das formatted data.")
  default_menu(eid, name, "{refs[0].dasTypeName}", changed) <| $(var changed2 : bool?)
    if imgui::Button("Copy das value")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(refs[0].roVal)

    return sub_menu |> invoke(changed2)


[ecs_inspector(type_name="ecs::Tag")]
def draw_tag(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  imgui::Text("<Tag>")
  default_menu(eid, name, "ecs::Tag", changed, sub_menu)


[ecs_inspector(type_name="int")]
def draw_int(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.S32, val, safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "int", $() => to_string(*val), $(str) { update_value(to_int(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="DPoint3")]
def draw_DPoint3(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<DPoint3?> refs[0].rawData)
  let step = 0.1
  let fastStep = 0.5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalarN("##{name}", ImGuiDataType.Double, val, 3, safe_addr(step), safe_addr(fastStep), "%.2lf", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "DPoint3", $() => to_string(*val), $(str) { *val = to_DPoint3(str); *changed = true; }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="int8_t")]
def draw_int8_t(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int8?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.S8, val, safe_addr(step), safe_addr(fastStep), "%u", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "i8", $() => to_string(*val), $(str) { update_value(to_int8(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="uint8_t")]
def draw_uint8_t(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<uint8?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.U8, val, safe_addr(step), safe_addr(fastStep), "%u", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "u8", $() => to_string(*val), $(str) { update_value(uint8(to_uint(str)), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="int64_t")]
def draw_int64(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int64?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.S64, val, safe_addr(step), safe_addr(fastStep), "%lld", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "i64", $() => to_string(*val), $(str) { update_value(to_int64(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="uint32_t")]
def draw_uint32(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<uint?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.U32, val, safe_addr(step), safe_addr(fastStep), "%u", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "u32", $() => to_string(*val), $(str) { update_value(uint(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="uint16_t")]
def draw_uint16(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<uint16?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.U16, val, safe_addr(step), safe_addr(fastStep), "%u", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "u16", $() => to_string(*val), $(str) { update_value(uint16(to_uint(str)), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="uint64_t")]
def draw_uint64(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<uint64?> refs[0].rawData)
  let step = 1
  let fastStep = 5
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.U64, val, safe_addr(step), safe_addr(fastStep), "%llx", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "u64", $() => to_string(*val), $(str) { update_value(to_uint64(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="E3DCOLOR")]
def draw_E3DCOLOR(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<E3DCOLOR?> refs[0].rawData)
  let uColor = uint(*val)
  var color = fixed_array<float>(float((uColor >> 16u) & 0xFF) / 255f, float((uColor >> 8u) & 0xFF) / 255f, float(uColor & 0xFF) / 255f, float(uColor >> 24u) / 255f)
  SetNextItemWidth(-FLT_MIN)
  if ColorEdit4("##{name}", safe_addr(color[0]), ImGuiColorEditFlags.None)
    *changed = true
    let res = uint(color[3] * 255f) << 24u | uint(color[0] * 255f) << 16u | uint(color[1] * 255f) << 8u | uint(color[2] * 255f)
    *val = E3DCOLOR(res)
  default_menu(eid, name, "E3DCOLOR",
               $() => to_string(*val),
               $(str) { let newVal = E3DCOLOR(to_uint(str)); *changed ||= (uint(*val) != uint(newVal)); *val = newVal; },
               $() => to_string_via_blk(name, eid[0]),
               changed, sub_menu)


[ecs_inspector(type_name="float")]
def draw_float(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float?> refs[0].rawData)
  let step = 1f
  let fastStep = 5f
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.Float, val, safe_addr(step), safe_addr(fastStep), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "float", $() => to_string(*val), $(str) { update_value(to_float(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="double")]
def draw_double(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<double?> refs[0].rawData)
  let step = 1f
  let fastStep = 5f
  SetNextItemWidth(-FLT_MIN)
  *changed = InputScalar("##{name}", ImGuiDataType.Double, val, safe_addr(step), safe_addr(fastStep), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "double", $() => to_string(*val), $(str) { update_value(to_double(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="bool")]
def draw_bool(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<bool?> refs[0].rawData)
  *changed = Checkbox("##{name}", val) || *changed
  default_menu(eid, name, "bool", $() => to_string(*val), $(str) { update_value(to_int(str) != 0, val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="Point2")]
def draw_Point2(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float2?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat2("##{name}", unsafe(addr((*val).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "Point2", $() => to_string(*val), $(str) { update_value(to_float2(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="Point3")]
def draw_Point3(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float3?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}", unsafe(addr((*val).x)), "%.4f", ImGuiInputTextFlags.None) || *changed

  default_menu(eid, name, "Point3", $() => to_string(*val), $(str) { update_value(to_float3(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed) <| $ [unused_argument(sub_changed)] (sub_changed)

    if imgui::Button("show me##{name}")
      draw_debug_sphere_buffered(*val, 1.0, E3DCOLOR(0xFFFFFF00), 12, 2500)
      var upPoint = *val
      upPoint.y += 150.0
      draw_debug_line_buffered(*val, upPoint, E3DCOLOR(0xFFFFFF00), 2500)

    if imgui::Button("show as dir##{name}")
      query(eid[0]) <| $ [es] (transform : float3x4)
        draw_debug_arrow_buffered(transform[3] + transform[1] * 0.5, transform[3] + transform[1] * 0.5 + *val, E3DCOLOR(0xFFFFFF00), 2500)

    return sub_menu |> invoke(changed)


[ecs_inspector(type_name="Point4")]
def draw_Point4(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float4?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat4("##{name}", unsafe(addr((*val).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "Point4", $() => to_string(*val), $(str) { update_value(to_float4(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="IPoint2")]
def draw_IPoint2(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int2?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputInt2("##{name}", unsafe(addr((*val).x)), ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "IPoint2", $() => to_string(*val), $(str) { update_value(to_int2(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="IPoint3")]
def draw_IPoint3(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int3?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputInt3("##{name}", unsafe(addr((*val).x)), ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "IPoint3", $() => to_string(*val), $(str) { update_value(to_int3(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="IPoint4")]
def draw_IPoint4(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<int4?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputInt4("##{name}", unsafe(addr((*val).x)), ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "IPoint4", $() => to_string(*val), $(str) { update_value(to_int4(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="vec4f")]
def draw_vec4f(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float4?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat4("##{name}", unsafe(addr((*val).x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "vec4f", $() => to_string(*val), $(str) { update_value(vec4f(to_float4(str)), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="TMatrix")]
def draw_TMatrix(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float3x4?> refs[0].rawData)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}_0", unsafe(addr((*val)[0].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}_1", unsafe(addr((*val)[1].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}_2", unsafe(addr((*val)[2].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat3("##{name}_3", unsafe(addr((*val)[3].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, name, "TMatrix", $() => to_string(*val), $(str) { update_value(to_TMatrix(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed) <| $(var sub_changed)
    let res = sub_menu |> invoke(sub_changed)
    if !res
      return res

    if imgui::Button("Copy position")
      imgui::CloseCurrentPopup()
      (*val)[3] |> to_string |> imgui::SetClipboardText

    if imgui::Button("Copy rotation")
      imgui::CloseCurrentPopup()
      float4(DagorMath::quat(*val)) |> to_string |> imgui::SetClipboardText

    imgui::Separator()

    if imgui::Button("Identity")
      imgui::CloseCurrentPopup()
      let pos = (*val)[3]
      identity(*val)
      (*val)[3] = pos
      *sub_changed = true

    if imgui::Button("Orthonormalize")
      imgui::CloseCurrentPopup()
      orthonormalize(*val)
      *sub_changed = true

    imgui::Separator()

    if imgui::Button("Show me##{name}")
      let pos = (*val)[3]
      draw_debug_sphere_buffered(pos, 1.0, E3DCOLOR(0xFFFFFF00), 12, 2500)
      var upPoint = pos
      upPoint.y += 150.0
      draw_debug_line_buffered(pos, upPoint, E3DCOLOR(0xFFFFFF00), 2500)

    return res


[ecs_inspector(type_name="ecs::string")]
def draw_das_string(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<das_string?> refs[0].rawData)
  var buf <- ImGuiInputTextBuffer("{*val}", length(*val) + 256)
  SetNextItemWidth(-FLT_MIN)
  if InputText("##{name}", buf, ImGuiInputTextFlags.AutoSelectAll)
    *changed = true
    *val := buf |> to_string()
  buf |> clear()
  default_menu(eid, name, "string", $() => to_string(*val), $(str) { clone_value(str, val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="ecs::EntityId")]
def draw_EntityId(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<EntityId?> refs[0].rawData)
  var uVal = uint(*val)
  let TEXT_BASE_WIDTH = CalcTextSize("A").x
  SetNextItemWidth(imgui::GetColumnWidth() - TEXT_BASE_WIDTH * 12f)
  if InputScalar("##{name}", ImGuiDataType.U32, safe_addr(uVal), default<int?>, default<int?>, "%d", ImGuiInputTextFlags.None)
    *val = EntityId(uVal)
    *changed = true
  if !!*val
    imgui_hint <| $
      return getEntityTemplateName(*val)
  SameLine()
  if imgui::Button("Select##{name}{eid[0]}")
    editor_select_eid(*val)
  SameLine()
  if imgui::Button("*##{name}{eid[0]}")
    imgui::OpenPopup("eid_menu_{name}{eid[0]}")

  if imgui::BeginPopup("eid_menu_{name}{eid[0]}")
    for i in range(1, 4)
      if imgui::Button("Inspector #{i+1}##{i+1}")
        imgui::CloseCurrentPopup()
        open_inspector(i, *val, "")
    imgui::EndPopup()

  default_menu(eid, name, "Eid", $() => to_string(*val), $(str) { update_value(EntityId(to_uint(str)), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)

[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Object>"), no_aot]
def draw_shared_object(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var sharedVal = unsafe(reinterpret<ecs::SharedObject?> refs[0].rawData)
  draw_object(eid, flags, name, "shared<Object>", sharedVal.get, level, changed, sub_menu)


[ecs_inspector(type_name="ecs::Object"), no_aot]
def draw_Object(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<Object?> refs[0].rawData)
  draw_object(eid, flags, name, "Object", val, level, changed, sub_menu)


[no_aot]
def draw_object(eid : array<EntityId> implicit; flags : uint; name, typeName : string; var val : Object?; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  imgui::PushID(name)
  defer <|
    imgui::PopID()
  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  let len = length(*val)
  let isEmpty = len == 0
  var open = false
  let fullName = level == 0 ? item_flags(flags, name) : name
  if isEmpty && level == 0
    imgui::Text(fullName)
    imgui_hint(name)
  else
    let treeflags = isEmpty ? ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet : ImGuiTreeNodeFlags.None
    open = TreeNodeEx(fullName, treeflags)
    imgui_hint(name)
  imgui::TableSetColumnIndex(1)

  var keys : array<string>
  for it in *val { keys |> push(clone_string(it.key)); }

  if imgui::Button("+ i")
    *val |> set(unique_key(keys), 0)
  imgui::SameLine()
  if imgui::Button("+ r")
    *val |> set(unique_key(keys), 0f)
  imgui::SameLine()
  if imgui::Button("+ eid")
    *val |> set(unique_key(keys), INVALID_ENTITY_ID)
  imgui::SameLine()
  if imgui::Button("+ p3")
    *val |> set(unique_key(keys), float3())
  imgui::SameLine()
  if imgui::Button("+ t")
    *val |> set(unique_key(keys), "")

  if isEmpty
    imgui::SameLine()
    imgui::Text("{typeName}: 0")
  else
    imgui::SameLine()
    imgui::Text("{typeName}: {len}")

  if !default_menu(eid, name, typeName, $() => to_string(*val), $(str) { logerr("Object to string not implemented! str={str}"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)
    if open && !isEmpty
      imgui::TreePop()
    return

  if isEmpty
    return

  delete keys
  for it in *val { keys |> push(clone_string(it.key)); }

  if !isEmpty && open
    var idx = -1
    while ++idx < length(keys)
      let key = keys[idx]
      var t = (*val)[keys[idx]]
      if t == null
        continue
      let it & = unsafe(*t)
      imgui::PushID(key)

      let isCont = is_cont_type(getEntityComponentRef(it).userType)

      if !isCont
        imgui::TableNextRow()
        imgui::TableSetColumnIndex(0)

        let itFlags = ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet
        TreeNodeEx(key, itFlags)
        imgui_hint(key)

        imgui::TableSetColumnIndex(1)

      let itRefs <- [ getComponentRenderInfoFromComponentRef(getEntityComponentRef(it)) ]
      render_item(eid, 0u, key, unsafe(reinterpret<array<ComponentRenderInfo#>#> itRefs), level + 1, changed) <| $(sub_changed)
        if imgui::Button("dup")
          imgui::CloseCurrentPopup()
          using() <| $(var cop : ChildComponent)
            cop := it
            *val |> set(unique_key(keys, key), cop)
            *sub_changed = true

        if imgui::Button("x")
          imgui::CloseCurrentPopup()
          *val |> erase(key)
          *sub_changed = true
          return false
        return true

      imgui::PopID()

    imgui::TreePop()


[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Array>"), no_aot]
def draw_shared_array(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var sharedVal = unsafe(reinterpret<ecs::SharedArray?> refs[0].rawData)
  draw_array(eid, flags, name, "shared<Array>", sharedVal.get, level, changed, sub_menu)


[ecs_inspector(type_name="ecs::Array"), no_aot]
def draw_Array(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<Array?> refs[0].rawData)
  draw_array(eid, flags, name, "Array", val, level, changed, sub_menu)


[no_aot]
def draw_array(eid : array<EntityId> implicit; flags : uint; name, typeName : string; var val : Array?; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  imgui::PushID(name)
  defer <|
    imgui::PopID()
  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  let len = length(*val)
  let isEmpty = len == 0
  var open = false
  let fullName = level == 0 ? item_flags(flags, name) : name
  if isEmpty && level == 0
    imgui::Text(fullName)
    imgui_hint(name)
  else
    let treeflags = isEmpty ? ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet : ImGuiTreeNodeFlags.None
    open = TreeNodeEx(fullName, treeflags)
    imgui_hint(name)
  imgui::TableSetColumnIndex(1)

  if imgui::Button("+ i")
    *val |> push(0)
  imgui::SameLine()
  if imgui::Button("+ r")
    *val |> push(0f)
  imgui::SameLine()
  if imgui::Button("+ eid")
    *val |> push(INVALID_ENTITY_ID)
  imgui::SameLine()
  if imgui::Button("+ p3")
    *val |> push(float3())
  imgui::SameLine()
  if imgui::Button("+ t")
    *val |> push("")

  if isEmpty
    imgui::SameLine()
    imgui::Text("{typeName}: 0")
  else
    imgui::SameLine()
    imgui::Text("{typeName}: {len}")

  if !default_menu(eid, name, typeName, $() => to_string(*val), $(str) { logerr("Array to string not implemented! str={str}"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)
    if open && !isEmpty
      imgui::TreePop()
    return

  if isEmpty
    return

  if !isEmpty && open
    var idx = -1
    while ++idx < length(*val)
      let key = "[{idx}]"
      let it & = unsafe((*val)[idx])
      imgui::PushID(key)

      let isCont = is_cont_type(getEntityComponentRef(it).userType)

      if !isCont
        imgui::TableNextRow()
        imgui::TableSetColumnIndex(0)

        let itFlags = ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet
        TreeNodeEx(key, itFlags)

        imgui::TableSetColumnIndex(1)

      let itRefs <- [ getComponentRenderInfoFromComponentRef(getEntityComponentRef(it)) ]
      render_item(eid, 0u, key, unsafe(reinterpret<array<ComponentRenderInfo#>#> itRefs), level + 1, changed) <| $(sub_changed)
        if imgui::Button("dup")
          using() <| $(var cop : ChildComponent)
            cop := it
            *val |> push(cop)
            *sub_changed = true

        if imgui::Button("x")
          *val |> erase(idx)
          *sub_changed = true
          return false
        return true

      imgui::PopID()

    imgui::TreePop()


[generic, no_aot]
def draw_any_list(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<ContType?> refs[0].rawData)
  imgui::PushID(name)
  defer <|
    imgui::PopID()
  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  let len = length(*val)
  let isEmpty = len == 0
  var open = false
  let fullName = level == 0 ? item_flags(flags, name) : name
  if isEmpty && level == 0
    imgui::Text(fullName)
    imgui_hint(name)
  else
    let treeflags = isEmpty ? ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet : ImGuiTreeNodeFlags.None
    open = TreeNodeEx(fullName, treeflags)
    imgui_hint(name)
  imgui::TableSetColumnIndex(1)

  if imgui::Button("+")
    let t : SubType = default<SubType>
    *val |> push(t)


  if isEmpty
    imgui::SameLine()
    imgui::Text("<empty {typeinfo typename(type<ContType>)}>")

  if !isEmpty
    imgui::SameLine()
    imgui::Text("{typeinfo typename(type<ContType>)} size: {len}")
    imgui::SameLine()
    if imgui::Button("Clear##{name}")
      *val |> clear()

  if !default_menu(eid, name, "{typeinfo typename(type<ContType>)}", $() => to_string(*val), $(str) { logerr("List to string not implemented! str={str}"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)
    if open && !isEmpty
      imgui::TreePop()
    return

  if isEmpty
    return

  if !isEmpty && open
    var idx = -1
    while ++idx < length(*val)
      let key = "[{idx}]"
      imgui::PushID(key)

      let isCont = is_cont_type(getEntityComponentRef(*val, uint(idx)).userType)

      if !isCont
        imgui::TableNextRow()
        imgui::TableSetColumnIndex(0)

        let itFlags = ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet
        TreeNodeEx(key, itFlags)

        imgui::TableSetColumnIndex(1)

      let itRefs <- [ getComponentRenderInfoFromComponentRef(getEntityComponentRef(*val, uint(idx))) ]
      render_item(eid, 0u, key, unsafe(reinterpret<array<ComponentRenderInfo#>#> itRefs), level + 1, changed) <| $(sub_changed)
        if imgui::Button("dup")
          *val |> push((*val)[idx])
          *sub_changed = true

        if imgui::Button("x")
          *val |> erase(idx)
          *sub_changed = true
          return false
        return true

      imgui::PopID()

    imgui::TreePop()


[generic, no_aot]
def draw_any_shared_list(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var sharedVal = unsafe(reinterpret<ContType?> refs[0].rawData)
  var val = sharedVal.get
  imgui::PushID(name)
  defer <|
    imgui::PopID()
  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  let len = length(*val)
  let isEmpty = len == 0
  var open = false
  let fullName = level == 0 ? item_flags(flags, name) : name
  if isEmpty && level == 0
    imgui::Text(fullName)
    imgui_hint(name)
  else
    let treeflags = isEmpty ? ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet : ImGuiTreeNodeFlags.None
    open = TreeNodeEx(fullName, treeflags)
    imgui_hint(name)
  imgui::TableSetColumnIndex(1)

  if imgui::Button("+")
    let t : SubType = default<SubType>
    *val |> push(t)

  if isEmpty
    imgui::SameLine()
    imgui::Text("{typeinfo typename(type<ContType>)}: 0")

  if !isEmpty
    imgui::SameLine()
    imgui::Text("{typeinfo typename(type<ContType>)}: {len}")
    imgui::SameLine()
    if imgui::Button("Clear##{name}")
      *val |> clear()

  if !default_menu(eid, name, "{typeinfo typename(type<ContType>)}", $() => to_string(*val), $(str) { logerr("SharedList to string not implemented! str={str}"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)
    if open && !isEmpty
      imgui::TreePop()
    return

  if isEmpty
    return

  if !isEmpty && open
    var idx = -1
    while ++idx < length(*val)
      let key = "[{idx}]"
      imgui::PushID(key)

      let isCont = is_cont_type(getEntityComponentRef(*val, uint(idx)).userType)

      if !isCont
        imgui::TableNextRow()
        imgui::TableSetColumnIndex(0)

        let itFlags = ImGuiTreeNodeFlags.Leaf | ImGuiTreeNodeFlags.NoTreePushOnOpen | ImGuiTreeNodeFlags.Bullet
        TreeNodeEx(key, itFlags)

        imgui::TableSetColumnIndex(1)

      let itRefs <- [ getComponentRenderInfoFromComponentRef(getEntityComponentRef(*val, uint(idx))) ]
      render_item(eid, 0u, key, unsafe(reinterpret<array<ComponentRenderInfo#>#> itRefs), level + 1, changed) <| $(sub_changed)
        if imgui::Button("dup")
          *val |> push((*val)[idx])
          *sub_changed = true

        if imgui::Button("x")
          *val |> erase(idx)
          *sub_changed = true
          return false
        return true

      imgui::PopID()

    imgui::TreePop()


[ecs_inspector(type_name="ecs::IntList"), no_aot, instance_function::instance_function(draw_any_list, ContType = IntList, SubType = "int")]
def draw_IntList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::UInt8List"), no_aot, instance_function::instance_function(draw_any_list, ContType = UInt8List, SubType = "uint8")]
def draw_UInt8List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::UInt16List"), no_aot, instance_function::instance_function(draw_any_list, ContType = UInt16List, SubType = "uint16")]
def draw_UInt16List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::UInt64List"), no_aot, instance_function::instance_function(draw_any_list, ContType = UInt64List, SubType = "uint64")]
def draw_UInt64List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::StringList"), no_aot, instance_function::instance_function(draw_any_list, ContType = StringList, SubType = "string")]
def draw_StringList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::EidList"), no_aot, instance_function::instance_function(draw_any_list, ContType = EidList, SubType = EntityId)]
def draw_EidList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::FloatList"), no_aot, instance_function::instance_function(draw_any_list, ContType = FloatList, SubType = "float")]
def draw_FloatList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::Point2List"), no_aot, instance_function::instance_function(draw_any_list, ContType = Point2List, SubType = "float2")]
def draw_Point2List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::Point3List"), no_aot, instance_function::instance_function(draw_any_list, ContType = Point3List, SubType = "float3")]
def draw_Point3List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::Point4List"), no_aot, instance_function::instance_function(draw_any_list, ContType = Point4List, SubType = "float4")]
def draw_Point4List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::IPoint2List"), no_aot, instance_function::instance_function(draw_any_list, ContType = IPoint2List, SubType = "int2")]
def draw_IPoint2List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::IPoint3List"), no_aot, instance_function::instance_function(draw_any_list, ContType = IPoint3List, SubType = "int3")]
def draw_IPoint3List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::BoolList"), no_aot, instance_function::instance_function(draw_any_list, ContType = BoolList, SubType = "bool")]
def draw_BoolList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::TMatrixList"), no_aot, instance_function::instance_function(draw_any_list, ContType = TMatrixList, SubType = "float3x4")]
def draw_TMatrixList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::ColorList"), no_aot, instance_function::instance_function(draw_any_list, ContType = ColorList, SubType = "E3DCOLOR")]
def draw_ColorList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::Int64List"), no_aot, instance_function::instance_function(draw_any_list, ContType = Int64List, SubType = "int64")]
def draw_Int64List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}


[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::IntList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedIntList, SubType = "int")]
def draw_SharedIntList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::UInt8List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedUInt8List, SubType = "uint8")]
def draw_SharedUInt8List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::UInt16List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedUInt16List, SubType = "uint16")]
def draw_SharedUInt16List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::UInt64List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedUInt64List, SubType = "uint64")]
def draw_SharedUInt64List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::StringList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedStringList, SubType = "string")]
def draw_SharedStringList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::EidList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedEidList, SubType = EntityId)]
def draw_SharedEidList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::FloatList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedFloatList, SubType = "float")]
def draw_SharedFloatList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Point2List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedPoint2List, SubType = "float2")]
def draw_SharedPoint2List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Point3List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedPoint3List, SubType = "float3")]
def draw_SharedPoint3List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Point4List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedPoint4List, SubType = "float4")]
def draw_SharedPoint4List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::IPoint2List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedIPoint2List, SubType = "int2")]
def draw_SharedIPoint2List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::IPoint3List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedIPoint3List, SubType = "int3")]
def draw_SharedIPoint3List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::BoolList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedBoolList, SubType = "bool")]
def draw_SharedBoolList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::TMatrixList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedTMatrixList, SubType = "float3x4")]
def draw_SharedTMatrixList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::ColorList>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedColorList, SubType = "E3DCOLOR")]
def draw_SharedColorList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
[ecs_inspector(type_name="ecs::SharedComponent< ::ecs::Int64List>"), no_aot, instance_function::instance_function(draw_any_shared_list, ContType = SharedInt64List, SubType = "int64")]
def draw_SharedInt64List(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void {}
