options no_aot
module verify_macro shared private

require EcsUtils
require ecs
require ast
require rtti
require strings
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost


[call_macro(name="ecs_type_hash")]
class EcsTypeMacro : AstCallMacro
  [unused_argument(mod)] def override visit(prog : ProgramPtr; mod : Module ?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr
    if length(expr.arguments) != 1
      macro_error(prog, expr.at, "ecs_type_hash expecting 1 argument")
      return <- [[ExpressionPtr]]
    if expr.arguments[0]._type == null || expr.arguments[0]._type.isAutoOrAlias
      macro_error(prog, expr.at, "ecs_type_hash requires fully inferred type")
      return <- [[ExpressionPtr]]
    var res = 0u
    let foundCppType = get_underlying_ecs_type(expr.arguments[0]._type, true) <| $(typeName)
      res = ecs_hash(typeName)
    if !foundCppType || res == 0u
      macro_error(prog, expr.at, "ecs_type_hash unable to resolve ecs type for {describe(expr.arguments[0]._type)}")
      return <- [[ExpressionPtr]]
    return <- qmacro($v(res))

def get_argument(arguments : rtti::AnnotationArgumentList; tag : string; default_value = "")
  for arg in arguments
    if string(arg.name) == tag
      return "{arg.sValue}"
  return default_value

def get_ecs_verify_fn(st : StructurePtr; name : string; map : table<string; uint>)
  if empty(name)
    return <- qmacro_function("_verify_{st.name}_") <| $(evt : Event)
      req_template($v(map))

  return <- qmacro_function("_verify_{st.name}_") <| $(evt : Event)
    req_template($v(name), $v(map))

[structure_macro(name="req_template")]
class ReqTemplateAnnotation : AstStructureAnnotation

  [unused_argument(group, args)] def override patch(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    if length(st.fields) == 0 || is_in_aot() || !is_entity_mgr_exists()
      st.fields |> clear()
      return true

    var name = ""
    for arg in args
      if arg.name == "name"
        name = string(arg.sValue)
        if empty(name)
          errors := "name is empty"
          return false

    var map : table<string; uint>
    for field in st.fields
      var typeHash : uint
      let foundCppType = get_underlying_ecs_type(field._type, true) <| $(typeName)
        typeHash = ecs_hash(typeName)
      if !foundCppType || typeHash == 0u
        errors := "unable to resolve ecs type for {describe(field._type)}"
        return false
      map[string(field.name)] = typeHash

    st.fields |> clear()

    var inscope fn <- get_ecs_verify_fn(st, name, map)

    fn |> append_annotation("ecs", "es", [{auto
        [[auto "on_event", [[RttiValue tString="VerifyEcsTemplates"]] ]]
    }])
    compiling_module() |> add_function(fn)
    astChanged = true
    return true
