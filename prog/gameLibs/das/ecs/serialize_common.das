options indenting = 2
module serialize_common shared
options no_aot

require ecs
require ecs.common
require EcsUtils
require DagorDataBlock
require DagorSystem
require strings
require daslib/strings_boost
require math.base
require daslib/rtti
require ecs.ecs_struct_walker_common
require daslib/temp_strings

let private MAX_STRUCT_DEPTH = 50
let private UNLIMITED_CONTAINER_ITEMS = 2147483647


def entity_to_blk_text(eid : EntityId) : string
  let templateName = getEntityTemplateName(eid)
  return wrap_entity_blk_text(entity_to_filtered_compact_blk_text(eid, templateName, "", UNLIMITED_CONTAINER_ITEMS), templateName)


def components_list_to_blk_text(list : ComponentsList; templateName : string) : string
  var annotations : table<string; BasicStructureAnnotation?>
  let body = build_string <| $(result)
    find_component(list) <| $(compName : string; comp : ChildComponent const#)
      let compRef = getEntityComponentRef(comp)
      let typeName = getComponentTypes() |> component_types_findTypeName(compRef.userType)
      result |> write(compact_component_to_blk_text(compName, typeName, compRef, UNLIMITED_CONTAINER_ITEMS, annotations, true))
      return false
  return wrap_entity_blk_text(body, templateName)


def wrap_entity_blk_text(body, templateName : string) : string
  let indentedBody = empty(body) ? "" : "  {replace(strip_right(body), "\n", "\n  ")}\n"
  let tName = remove_sub_template_name(templateName, "daeditor_selected")
  return "entity\{\n  _template:t=\"{tName}\"\n{indentedBody}\}\n"

// For long arrays it reports only the length
def entity_to_compact_blk_text(eid : EntityId; maxContainerItems : int = 10; dumpStructFields : bool = true) : string
  return entity_to_filtered_compact_blk_text(eid, getEntityTemplateName(eid), "", maxContainerItems, dumpStructFields)


// filterStr uses the ecs inspector filter syntax ("name,-name,t:type,-t:type,f:t,-f:r"),
// name and type filters also match nested fields of das-bound structs ("comp.field.subfield")
def entity_to_filtered_compact_blk_text(eid : EntityId; templateName, filterStr : string; maxContainerItems : int = 10;
                                        dumpStructFields : bool = true) : string
  var annotations : table<string; BasicStructureAnnotation?>
  return entity_to_filtered_compact_blk_text(eid, templateName, filterStr, annotations, maxContainerItems, dumpStructFields)


def entity_to_filtered_compact_blk_text(eid : EntityId; templateName, filterStr : string;
                                        var annotations : table<string; BasicStructureAnnotation?>;
                                        maxContainerItems : int = 10; dumpStructFields : bool = true) : string
  let mgr & = getEntityManager()
  let tmpl = getTemplateByName(templateName)

  var filters : NameTypeFilters
  var flagFilters : array<string>
  var excludeFlagFilters : array<string>
  let filterParts <- filterStr |> split(",")
  for rawFilter in filterParts
    let f = rawFilter |> strip() |> to_lower()
    if f |> empty()
      continue
    if f |> starts_with("t:") && length(f) > 2
      filters.types |> push(slice(f, 2) |> strip())
    elif f |> starts_with("-t:") && length(f) > 3
      filters.excludeTypes |> push(slice(f, 3) |> strip())
    elif f |> starts_with("f:") && length(f) > 2
      flagFilters |> push(slice(f, 2) |> strip())
    elif f |> starts_with("-f:") && length(f) > 3
      excludeFlagFilters |> push(slice(f, 3) |> strip())
    elif f |> starts_with("-") && length(f) > 1
      filters.exclude |> push(slice(f, 1) |> strip())
    else
      filters.includes |> push(f)

  let compsNum = mgr |> getNumComponents(eid)
  return build_string <| $(result)
    for idx in urange(compsNum >= 0 ? compsNum : 0)
      let compRef = mgr |> getEntityComponentRef(eid, idx)
      let comp = getDataComponents() |> data_components_getComponentById(compRef.componentId)
      if (uint(comp.flags) & uint(IS_COPY)) != 0u
        continue

      let compName = getDataComponents() |> data_components_getComponentNameById(compRef.componentId)
      let typeName = getComponentTypes() |> component_types_findTypeName(compRef.userType)

      if length(filters.includes) + length(filters.exclude) + length(filters.types) + length(filters.excludeTypes) > 0
        let lowerName = compName |> to_lower()
        var passes = name_type_passes(filters, lowerName, typeName)
        if !passes && dumpStructFields
          if empty(annotations)
            collect_basic_struct_annotations(annotations)
          if annotations |> key_exists(typeName)
            let data = component_data(compRef)
            var visited : table<string>
            visited |> insert(struct_visit_key(data, typeName))
            passes = struct_fields_pass_filters(lowerName, data, typeName, 1, annotations, visited, filters)
        if !passes
          continue

      if length(flagFilters) > 0 || length(excludeFlagFilters) > 0
        let flags = tmpl != null && !empty(compName) ? getRegExpInheritedFlags(mgr, tmpl, compName) : 0u
        let nameWithFlags = item_flags(flags, "")
        if (length(flagFilters) > 0 && !contains_any_substring(flagFilters, nameWithFlags)) || (length(excludeFlagFilters) > 0 && contains_any_substring(excludeFlagFilters, nameWithFlags))
          continue

      result |> write(compact_component_to_blk_text(compName, typeName, compRef, maxContainerItems, annotations, dumpStructFields))

struct private NameTypeFilters
  includes : array<string>
  exclude : array<string>
  types : array<string>
  excludeTypes : array<string>

def private name_type_passes(filters : NameTypeFilters; lowerName, typeName : string | #) : bool
  if (length(filters.includes) > 0 && !contains_any_substring(filters.includes, lowerName)) || contains_any_substring(filters.exclude, lowerName)
    return false
  if length(filters.types) == 0 && length(filters.excludeTypes) == 0
    return true
  let lowerType = typeName |> to_lower()
  return (length(filters.types) == 0 || contains_any_substring(filters.types, lowerType)) && !contains_any_substring(filters.excludeTypes, lowerType)

def private struct_fields_pass_filters(lowerPrefix : string | #; data : uint8?; type_name : string; depth : int;
                                       var annotations : table<string; BasicStructureAnnotation?>; var visited : table<string>;
                                       filters : NameTypeFilters) : bool
  var passes = false
  *(annotations |> get_value(type_name)) |> basic_struct_for_each_field <| $ [unused_argument(cppName)] (field_name, cppName, tinfo, offset)
    if passes
      return
    resolve_struct_field(data, tinfo, offset) <| $ [unused_argument(ti)] (var fieldData : uint8?; ti : TypeInfo const?; isStruct : bool; fieldTypeName : string)
      temp_string("{lowerPrefix}.{field_name |> to_lower()}") <| $(lowerName)
        passes = name_type_passes(filters, lowerName, fieldTypeName)
        if passes || fieldData == null || !isStruct || !(annotations |> key_exists(fieldTypeName)) || depth >= MAX_STRUCT_DEPTH
          return
        let key = struct_visit_key(fieldData, fieldTypeName)
        if !(visited |> key_exists(key))
          visited |> insert(key)
          passes = struct_fields_pass_filters(lowerName, fieldData, fieldTypeName, depth + 1, annotations, visited, filters)
  return passes


def compact_component_to_blk_text(compName, typeName : string; compRef : EntityComponentRef; maxContainerItems : int;
                                  var annotations : table<string; BasicStructureAnnotation?>; dumpStructFields : bool) : string
  if typeName == "ecs::EntityId"
    return entity_id_component_to_blk_text(compName, compRef)

  if typeName == "ecs::EidList"
    return eid_list_component_to_blk_text(compName, compRef, maxContainerItems)

  if typeName == "ecs::SharedComponent< ::ecs::EidList>"
    return shared_eid_list_component_to_blk_text(compName, compRef, maxContainerItems)

  if !is_component_blk_serializable(compRef.userType)
    return unserializable_component_to_blk_text(compName, typeName, compRef, annotations, dumpStructFields)

  var result = ""
  using() <| $(var blk : DataBlock)
    component_to_blk_param(compName, compRef, blk, false)
    var elemCount = 0
    var blockName = ""
    if is_cont_type(compRef.userType) && blk.blockCount > 0u
      datablock_get_block(blk, 0u) <| $(contBlk)
        elemCount = int(contBlk.paramCount) + int(contBlk.blockCount)
        blockName = contBlk |> datablock_getBlockName()
    if elemCount < maxContainerItems
      result = string(blk)
    else
      result = "\"{blockName}\"\{\} // [{elemCount} items]\n"

  return result


def unserializable_component_to_blk_text(compName, typeName : string; compRef : EntityComponentRef;
                                         var annotations : table<string; BasicStructureAnnotation?>; dumpStructFields : bool) : string
  // not the global cache from ecs_struct_walker_common: non-persistent contexts free the heap after each call
  if empty(annotations)
    collect_basic_struct_annotations(annotations)
  if !(annotations |> key_exists(typeName))
    return "\"{compName}:{typeName}\"\{\} // [unserializable]\n"
  if !dumpStructFields
    return "\"{compName}:{typeName}\"\{\} // [C++ struct]\n"

  let data = component_data(compRef)
  var visited : table<string>
  visited |> insert(struct_visit_key(data, typeName))
  return build_string <| $(result)
    result |> write("\"{compName}:{typeName}\"\{\} // [C++ struct]\n")
    write_struct_fields_as_comments(result, data, typeName, 1, annotations, visited)

def component_data(compRef : EntityComponentRef) : uint8?
  let typeInfo = getComponentTypes() |> component_types_getTypeInfo(compRef.typeId)
  let isBoxed = (uint(typeInfo.flags) & uint(ComponentTypeFlags.COMPONENT_TYPE_BOXED)) != 0u
  return isBoxed ? *unsafe(reinterpret<uint8 ??> compRef.rawData) : unsafe(reinterpret<uint8?> compRef.rawData)

def write_struct_fields_as_comments(var writer : StringBuilderWriter; data : uint8?; type_name : string; depth : int;
                                    var annotations : table<string; BasicStructureAnnotation?>; var visited : table<string>)
  let indent = repeat("  ", depth)
  *(annotations |> get_value(type_name)) |> basic_struct_for_each_field <| $ [unused_argument(cppName)] (field_name, cppName, tinfo, offset)
    resolve_struct_field(data, tinfo, offset) <| $(var fieldData : uint8?; ti : TypeInfo const?; isStruct : bool; fieldTypeName : string)
      if fieldData == null
        writer |> write("//{indent}{field_name} = null\n")
        return

      if isStruct && annotations |> key_exists(fieldTypeName)
        let key = struct_visit_key(fieldData, fieldTypeName)
        if visited |> key_exists(key)
          writer |> write("//{indent}{field_name}: {fieldTypeName} [C++ struct, already dumped]\n")
        elif depth >= MAX_STRUCT_DEPTH
          writer |> write("//{indent}{field_name}: {fieldTypeName} [C++ struct, max depth {MAX_STRUCT_DEPTH} reached]\n")
        else
          visited |> insert(key)
          writer |> write("//{indent}{field_name}: {fieldTypeName} [C++ struct]\n")
          write_struct_fields_as_comments(writer, fieldData, fieldTypeName, depth + 1, annotations, visited)
        return

      let value = replace(sprint_data(fieldData, ti, default<print_flags>), "\n", "\n//{indent}  ")
      writer |> write("//{indent}{field_name} = {value}\n")

def struct_visit_key(data : uint8?; type_name : string) : string
  return "{intptr(data)}:{type_name}"


def entity_id_component_to_blk_text(compName : string; compRef : EntityComponentRef) : string
  let val = unsafe(reinterpret<EntityId?> compRef.rawData)
  return "\"{compName}:eid\"\{ value:i={int(uint(*val))} \}\n"

def eid_list_component_to_blk_text(compName : string; compRef : EntityComponentRef; maxContainerItems : int) : string
  let val = unsafe(reinterpret<EidList?> compRef.rawData)
  return eid_list_to_blk_text(compName, *val, maxContainerItems, false)

def shared_eid_list_component_to_blk_text(compName : string; compRef : EntityComponentRef; maxContainerItems : int) : string
  let sharedVal = unsafe(reinterpret<SharedEidList?> compRef.rawData)
  let val = sharedVal.get
  return eid_list_to_blk_text(compName, *val, maxContainerItems, true)

def eid_list_to_blk_text(compName : string; val : EidList; maxContainerItems : int; isShared : bool) : string
  let elemCount = length(val)
  let blockName = isShared ? "{compName}:shared:list<eid>" : "{compName}:list<eid>"
  if elemCount >= maxContainerItems
    return "\"{blockName}\"\{\} // [{elemCount} items]\n"

  return build_string <| $(result)
    result |> write("\"{blockName}\"\{\n")
    for eid in val
      result |> write("  \"item:eid\"\{ value:i={int(uint(eid))} \}\n")
    result |> write("\}\n")


def contains_any_substring(arr : array<string>; str : string | #) : bool
  for s in arr
    if find(str, s) >= 0
      return true
  return false

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
      || typeName == "ecs::IPoint4List"
      || typeName == "ecs::BoolList"
      || typeName == "ecs::TMatrixList"
      || typeName == "ecs::ColorList"
      || typeName == "ecs::Int64List"
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
      || typeName == "ecs::SharedComponent< ::ecs::IPoint4List>"
      || typeName == "ecs::SharedComponent< ::ecs::BoolList>"
      || typeName == "ecs::SharedComponent< ::ecs::TMatrixList>"
      || typeName == "ecs::SharedComponent< ::ecs::ColorList>"
      || typeName == "ecs::SharedComponent< ::ecs::Int64List>")
