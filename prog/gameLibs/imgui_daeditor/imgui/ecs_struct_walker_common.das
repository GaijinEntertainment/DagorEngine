module ecs_struct_walker_common shared
options no_global_variables = false

require ecs
require ecs.common
require EcsUtils
require daslib/rtti
require strings

def get_annotation_name(ann : rtti::Annotation | rtti::TypeAnnotation)
  var name = ""

  // same logic as get_underlying_ecs_type
  peek(ann.cppName) <| $(cppName)
    if length(cppName) > 0
      var startAt = 0
      while startAt < length(cppName) && (cppName.character_at(startAt) == ':' || cppName.character_at(startAt) == ' ')
        startAt += 1
      name = slice(cppName, startAt)

  if name == ""
    name = "{ann.name}"

  return name


var private @ignore_heap_usage basicStructAnnotations : table<string; rtti::BasicStructureAnnotation?>
var private basicStructAnnotationsInited = false

[no_aot]
def private ensure_basic_struct_annotations_inited()
  if basicStructAnnotationsInited
    return
  basicStructAnnotationsInited = true
  program_for_each_registered_module() <| $(mod)
    mod |> module_for_each_annotation <| $(ann)
      if ann.isBasicStructureAnnotation
        unsafe
          let name = get_annotation_name(ann)
          basicStructAnnotations |> insert(name) <| addr(reinterpret<BasicStructureAnnotation> ann)

[no_aot]
def get_basic_struct_annotation(type_name : string) : rtti::BasicStructureAnnotation?
  ensure_basic_struct_annotations_inited()
  return basicStructAnnotations |> get_value(type_name)

[no_aot]
def basic_struct_annotation_exists(type_name : string) : bool
  ensure_basic_struct_annotations_inited()
  return basicStructAnnotations |> key_exists(type_name)

def das_type_to_ecs_type_str(typ : rtti::Type)
  if typ == Type.tBool
    return "bool"
  if typ == Type.tInt8
    return "int8_t"
  if typ == Type.tUInt8
    return "uint8_t"
  if typ == Type.tInt16
    return "int16_t"
  if typ == Type.tUInt16
    return "uint16_t"
  if typ == Type.tInt64
    return "int64_t"
  if typ == Type.tUInt64
    return "uint64_t"
  if typ == Type.tInt
    return "int"
  if typ == Type.tInt2
    return "IPoint2"
  if typ == Type.tInt3
    return "IPoint3"
  if typ == Type.tInt4
    return "IPoint4"
  if typ == Type.tUInt
    return "uint32_t"
  if typ == Type.tFloat
    return "float"
  if typ == Type.tFloat2
    return "Point2"
  if typ == Type.tFloat3
    return "Point3"
  if typ == Type.tFloat4
    return "Point4"
  if typ == Type.tDouble
    return "double"
  if typ == Type.tString
    return "ecs::string"
  return "UnknownBindedStructureFieldType"


// Steps into one field of a struct whose annotation is in basicStructAnnotations.
// Pointer-typed fields are auto-dereferenced.
[no_aot]
def descend_field(data : uint8?; type_name : string; field_name : string;
                  var out_data : uint8?&; var out_type_name : string&) : bool
  let ann = get_basic_struct_annotation(type_name)
  if ann == null
    return false
  var found = false
  *ann |> basic_struct_for_each_field <| $ [unused_argument(cppName)] (fname, cppName, tinfo, offset)
    if found || fname != field_name
      return
    unsafe
      let is_ptr = tinfo.basicType == Type.tPointer && tinfo.firstType != null
      var field_data : uint8?
      if is_ptr
        field_data = *reinterpret<uint8 ??>(data + offset)
      else
        field_data = reinterpret<uint8? -const>(data + offset)
      if field_data == null
        return
      let ti & = is_ptr ? *tinfo.firstType : tinfo
      if (ti.basicType == Type.tHandle || ti.isRefType) && ti.annotation != null
        out_type_name = get_annotation_name(*ti.annotation)
      else
        out_type_name = das_type_to_ecs_type_str(ti.basicType)
      out_data = field_data
      found = true
  return found


// Resolves a dotted path (e.g. "human_net_phys.currentState.location.P") to raw leaf bytes.
// Root is an ECS component; remaining segments walk through annotated nested structs.
[no_aot]
def resolve_component_path(eid : EntityId; full_name : string;
                           var out_data : void?&; var out_type_name : string&) : bool
  let dot = find(full_name, ".")
  let comp_name = dot >= 0 ? slice(full_name, 0, dot) : full_name
  let mgr & = getEntityManager()
  let comp_ref = mgr |> getComponentRef(eid, comp_name)
  if comp_ref.isNull
    return false
  let comp_type_name = getComponentTypes() |> component_types_findTypeName(comp_ref.userType)
  let comp_type_info = getComponentTypes() |> component_types_getTypeInfo(comp_ref.typeId)
  let is_boxed = (uint(comp_type_info.flags) & uint(ComponentTypeFlags.COMPONENT_TYPE_BOXED)) != 0u
  var root_data : uint8?
  unsafe
    root_data = is_boxed ? *reinterpret<uint8 ??> comp_ref.rawData : reinterpret<uint8?> comp_ref.rawData
  if dot < 0
    unsafe
      out_data = reinterpret<void?> root_data
    out_type_name = comp_type_name
    return true
  var cur_data = root_data
  var cur_type_name = comp_type_name
  var rest = slice(full_name, dot + 1)
  while true
    let next_dot = find(rest, ".")
    let field = next_dot >= 0 ? slice(rest, 0, next_dot) : rest
    var next_data : uint8?
    var next_type_name = ""
    if !descend_field(cur_data, cur_type_name, field, next_data, next_type_name)
      return false
    cur_data = next_data
    cur_type_name = next_type_name
    if next_dot < 0
      break
    rest = slice(rest, next_dot + 1)
  unsafe
    out_data = reinterpret<void?> cur_data
  out_type_name = cur_type_name
  return true
