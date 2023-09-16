options no_aot = true//currently, as of 08.10.2021 there is a bug in AOT codegen for E3DCOLOR
module ecs_template shared private
require ecs
require DagorMath
require DagorSystem
require daslib/strings_boost
require daslib/templates
require daslib/templates_boost
require daslib/ast_boost
require daslib/defer


let show_completion = true

bitfield TemplateComponentFlags
  TRACKED
  REPLICATED
  IGNORED

def private template_flags(annotations : AnnotationArgumentList)
  var ret : TemplateComponentFlags
  for a in annotations
    if a.basicType != Type tBool
      continue
    if a.name == "t"
      ret |= TemplateComponentFlags TRACKED
    elif a.name == "tr" || a.name == "rt"
      ret |= (TemplateComponentFlags TRACKED) | (TemplateComponentFlags REPLICATED)
    elif a.name == "r"
      ret |= (TemplateComponentFlags REPLICATED)
    elif a.name == "i"
      ret |= (TemplateComponentFlags IGNORED)
  return ret

def private is_undef(arg)
  return (arg is tBool) || (arg is tString)

def private is_undef(annotations : AnnotationArgumentList)
  return is_undef(find_arg(annotations, "undef"))

def follow_the_yellow_brick_road(expr : ExpressionPtr; var res : ExpressionPtr&) : void
  if expr is ExprRef2Value
    (expr as ExprRef2Value).subexpr |> follow_the_yellow_brick_road(res)
    return
  elif expr is ExprVar
    var ev = expr as ExprVar
    if !ev.varFlags.local && !ev.varFlags.argument && !ev.varFlags._block
      if ev.variable != null && ev.variable._type.flags.constant && ev.variable.init != null
        ev.variable.init |> follow_the_yellow_brick_road(res)
        return
  res := expr

def get_eval_any(econst : ExpressionPtr; var errors : das_string; var any_errors : bool&; var output : auto(TT)&)
  if econst is ExprCall
    if econst.flags.noSideEffects
      var ok = true
      unsafe
        var res : variant<dummy : float4; id : TT-&-const>
        res.dummy = eval_single_expression(econst, ok)
        if !ok
          errors := "{errors}\nexpression failed to evaluate {describe(econst)}"
          any_errors = true
        static_if typeinfo(can_move res.id)
          output = res.id
        else
          output := res.id
        return
  errors := "{errors}\nunsupported initializer {describe(econst)}"
  any_errors = true

def get_const_expr(expr : ExpressionPtr; var res : ExpressionPtr&) : void
  if expr == null
    return
  var inscope econst : ExpressionPtr
  expr |> follow_the_yellow_brick_road(econst)
  if econst != null && econst |> is_expr_const()
    res |> move() <| econst

options no_unused_function_arguments = false

def get_templ_const(var expr : ExpressionPtr; var errors : das_string; var any_errors : bool&; var output : auto&)
  if expr == null
    return
  var inscope econst : ExpressionPtr
  expr |> follow_the_yellow_brick_road(econst)
  econst |> get_eval_any(errors, any_errors, output)

def def_string(v : das_string?)
  return v != null ? string(*v) : ""

[structure_macro(name=ecs_template)]
class CreateTemplateAnnotation : AstStructureAnnotation
  [unused_argument(group, args, errors)] def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    if show_completion && is_in_completion()
      return true
    for field in st.fields
      let arg = field.annotation |> find_arg("undef")
      if is_undef(arg)
        let typeName = describe(field._type)
        let undefType = field._type.baseType == Type tVoid || field._type.baseType == Type autoinfer
        field._type := null
        if undefType && !(arg is tString)
          field._type |> move_new() <| new [[TypeDecl() baseType = Type tInt]]
        else
          field._type |> move_new() <| new [[TypeDecl() baseType = Type tInt, alias := (arg ?as tString) ?? typeName]]
        field.init := null
    return true

  def override finish(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    if !is_entity_mgr_exists() || (show_completion && is_in_completion())
      return true
    if is_in_aot() //|| find_index_if(each(args), $(a:AnnotationArgument) {return a.basicType == Type tString && a.name == "tag" && filter_pass_component_tag(group, a.sValue);}) != -1
      st.fields |> clear()
      return true
    var inscope parents : array<string>
    var tags : array<uint>
    var hadErrors = false
    for a in args
      if a.name == "extends"
        if a.basicType != Type tString
          errors := "{errors}\nextends argument {a} has to be of string type"
          hadErrors = true
        else
          parents |> push(string(a.sValue))
      elif a.name == "tag"
        if a.basicType != Type tString
          errors := "{errors}\tag argument {a} has to be of string type"
          hadErrors = true
        else
          tags |> push(ecs_hash(string(a.sValue)))
    createTemplate2(group, ecs_hash(string(st.at.fileInfo.name)), string(st.name), parents) <| $(var cmp : CreatingTemplate)
      for tag in tags
        cmp |> creating_template_addTag(tag)
      for field in st.fields
        let fieldName = string(field.name)
        for a in field.annotation
          if a.name == "tag"
            if a.basicType != Type tString
              errors := "{errors}\tag argument {a} has to be of string type"
              hadErrors = true
            else
              cmp |> creating_template_addCompTag(ecs_hash(fieldName), ecs_hash(string(a.sValue)))
        //filter out
        //if field.annotation |> each |> map ( @(a) : bool => a.basicType == Type tString && a.name == "tag" && filter_pass_component_tag(cmp, a.sValue)) |> any//map-reduce is a bit slower
        // if find_index_if(each(field.annotation), $(a:AnnotationArgument) {return a.basicType == Type tString && a.name == "tag" && filter_pass_component_tag_templ(cmp, a.sValue);}) != -1
        //   continue
        let f = template_flags(field.annotation)
        let undefType = is_undef(field.annotation)
        if undefType
          set_undef(cmp, f, fieldName, string(field._type.alias))
        else
          var inscope econst <- [[ExpressionPtr]]
          field.init |> get_const_expr(econst)
          if field._type.baseType == Type tBool
            set(cmp, f, fieldName, (econst ?as ExprConstBool)?.value ?? false)
          elif field._type.baseType == Type tInt
            set(cmp, f, fieldName, (econst ?as ExprConstInt)?.value ?? 0)
          elif field._type.baseType == Type tUInt
            set(cmp, f, fieldName, (econst ?as ExprConstUInt)?.value ?? 0u)
          elif field._type.baseType == Type tUInt64
            set(cmp, f, fieldName, (econst ?as ExprConstUInt64)?.value ?? uint64(0))
          elif field._type.baseType == Type tInt2
            set(cmp, f, fieldName, (econst ?as ExprConstInt2)?.value ?? int2(0))
          elif field._type.baseType == Type tInt3
            set(cmp, f, fieldName, (econst ?as ExprConstInt3)?.value ?? int3(0))
          elif field._type.baseType == Type tFloat
            set(cmp, f, fieldName, (econst ?as ExprConstFloat)?.value ?? 0.)
          elif field._type.baseType == Type tFloat2
            set(cmp, f, fieldName, (econst ?as ExprConstFloat2)?.value ?? float2(0.))
          elif field._type.baseType == Type tFloat3
            set(cmp, f, fieldName, (econst ?as ExprConstFloat3)?.value ?? float3(0.))
          elif field._type.baseType == Type tFloat4
            set(cmp, f, fieldName, (econst ?as ExprConstFloat4)?.value ?? float4(0.))
          elif field._type.baseType == Type tFloat4
            set(cmp, f, fieldName, (econst ?as ExprConstFloat4)?.value ?? float4(0.))
          elif field._type.baseType == Type tString
            set(cmp, f, fieldName, def_string((econst ?as ExprConstString)?.value))
          elif field._type.baseType == Type tHandle
            if field._type.annotation.name == "EntityId"
              set(cmp, f, fieldName, INVALID_ENTITY_ID)
            elif field._type.annotation.name == "E3DCOLOR"
              var color : E3DCOLOR
              get_templ_const(field.init, errors, hadErrors, color)
              set(cmp, f, fieldName, color)
            elif field._type.annotation.name == "Array"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Array")
            elif field._type.annotation.name == "Object"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Object")
            elif field._type.annotation.name == "Tag"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Tag")
            elif field._type.annotation.name == "UInt8List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::UInt8List")
            elif field._type.annotation.name == "UInt16List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::UInt16List")
            elif field._type.annotation.name == "UInt32List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::UInt32List")
            elif field._type.annotation.name == "UInt64List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::UInt64List")
            elif field._type.annotation.name == "StringList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::StringList")
            elif field._type.annotation.name == "EidList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::EidList")
            elif field._type.annotation.name == "FloatList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::FloatList")
            elif field._type.annotation.name == "Point2List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Point2List")
            elif field._type.annotation.name == "Point3List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Point3List")
            elif field._type.annotation.name == "Point4List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Point4List")
            elif field._type.annotation.name == "IPoint2List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::IPoint2List")
            elif field._type.annotation.name == "IPoint3List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::IPoint3List")
            elif field._type.annotation.name == "BoolList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::BoolList")
            elif field._type.annotation.name == "TMatrixList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::TMatrixList")
            elif field._type.annotation.name == "ColorList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::ColorList")
            elif field._type.annotation.name == "Int8List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Int8List")
            elif field._type.annotation.name == "Int16List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Int16List")
            elif field._type.annotation.name == "IntList"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::IntList")
            elif field._type.annotation.name == "Int64List"
              assert(field.init == null)
              set_undef(cmp, f, fieldName, "ecs::Int64List")
            else
              logerr("Template {st.name} declared at {st.at.fileInfo.name} has field <{fieldName}> of unknown handleType <{field._type.annotation.name}> fullName <{describe(field._type)}>")
    return !hadErrors
