module soa_template shared private
options no_aot

require ecs
require strings
require daslib/ast_boost
require daslib/remove_call_args
require DagorSystem


let
  show_completion = true


[macro_function]
def has_annotation(annotations : rtti::AnnotationList; tag : string)
  for ann in annotations
    if string(ann.annotation.name) == tag
      return true
  return false

[macro_function]
def fn_or_block_apply(var func; body; var errors : das_string)
  if show_completion && is_in_completion()
    return true
  var usedTypes : table<string; bool>
  var toReplace : array<tuple<name : string; fields : array<string>>>
  var i = 0
  while i < length(func.arguments)
    assume arg = func.arguments[i]
    if arg._type.baseType == Type tStructure && arg._type.structType.name |> ends_with("_SOA")
      let desc = arg._type |> describe(false, false, true)
      if usedTypes |> key_exists(desc)
        errors := "Multiple arguments with same soa type '{desc}'"
        return false
      usedTypes[desc] = true
      for field in arg._type.structType.fields
        if true
          var inscope fieldType <- clone_type(field._type)
          if arg._type.flags.constant
            fieldType.flags |= TypeDeclFlags constant
          var inscope newArg <- new [[Variable() at=arg.at,
            name := field.name,
            _type <- fieldType,
            _aka := "{field.name}_SOA"
          ]]
          for ann in field.annotation
            newArg.annotation |> add_annotation_argument(ann)
          func.arguments |> emplace(newArg)
      toReplace |> emplace <| [[auto string(arg.name), [{ for f in arg._type.structType.fields; string(f.name) }] ]]
      func.arguments |> erase(i)
    else
      i += 1

  for replace in toReplace
    replate_index_and_field(replace.name, replace.fields, body)
  return true

[macro_function]
def patch_soa_blocks(fn : smart_ptr<Function>; ast_changed : bool?)
  var astVisitor = new PatchSoaBlocks(ast_changed)
  var inscope astVisitorAdapter <- make_visitor(*astVisitor)
  visit(fn, astVisitorAdapter)
  astVisitorAdapter := null
  unsafe
    delete astVisitor

[macro_interface]
class PatchSoaBlocks : AstVisitor
  astChanged : bool?

  def PatchSoaBlocks(ast_changed : bool?)
    astChanged = ast_changed

  def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr
    for ann in blk.annotations
      if ann.annotation.name == "soa_blk"
        block_patch(get_ptr(blk), *astChanged)
    return <- blk


[macro_function]
def fn_or_block_patch(var func; blk; var astChanged : bool&)
  patch_soa_blocks(func, unsafe(addr(astChanged)))

  var fnBlock = blk as ExprBlock
  if length(fnBlock.list) > 0
    assume firstBlk = fnBlock.list[0]
    if firstBlk is ExprCall
      let call = firstBlk as ExprCall
      if call.name == "ecs::process_view" && length(call.arguments) > 1 && call.arguments[1] is ExprMakeBlock
        let makeEsBlk = call.arguments[1] as ExprMakeBlock
        return block_patch(makeEsBlk._block as ExprBlock, astChanged)

  return fn_patch(func, astChanged)


[macro_function]
def fn_patch(var fn : smart_ptr<Function>; var astChanged : bool&)
  patch_soa_blocks(fn, unsafe(addr(astChanged)))

  for ann in fn.annotations
    if ann.annotation.name == "remove_call_args"
      return true

  var removeArgs : array< tuple<argname : string; argvalue : RttiValue> >
  var i = length(fn.arguments)
  while --i >= 0
    assume arg = fn.arguments[i]
    if !arg.access_flags.access_pass && !arg.access_flags.access_get && !arg.access_flags.access_ref && !arg.access_flags.access_init && ends_with("{arg._aka}", "_SOA")
      astChanged = true
      removeArgs |> emplace([[auto "arg", [[RttiValue tInt=i]] ]])

  if length(removeArgs) > 0
    fn |> append_annotation("remove_call_args", "remove_call_args", removeArgs)

  return true


[macro_function]
def block_patch(var blk : ExprBlock?; var astChanged : bool&)
  var indexes : array<int>
  var i = length(blk.arguments)
  while --i >= 0
    assume arg = blk.arguments[i]
    if !arg.access_flags.access_pass && !arg.access_flags.access_get && !arg.access_flags.access_ref && !arg.access_flags.access_init && ends_with("{arg._aka}", "_SOA")
      blk.arguments |> erase(i)
      astChanged = true
      indexes |> push(i)
  return true


[function_macro(name="soa_def")]
class SoaDefCallMacro : AstFunctionAnnotation
  [unused_argument(group, args)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    return fn_or_block_apply(func, func.body, errors)

  [unused_argument(group, args, progArgs, errors)] def override patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    return fn_patch(func, astChanged)

[function_macro(name="soa_es")]
class SoaEsCallMacro : AstFunctionAnnotation
  [unused_argument(group, args)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    return fn_or_block_apply(func, func.body, errors)

  [unused_argument(group, args, progArgs, errors)] def override patch(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool &) : bool
    return fn_or_block_patch(func, func.body, astChanged)


[block_macro(name="soa_blk")]
class SoaEsBlockMacro : AstBlockAnnotation
  [unused_argument(group, args)] def override apply(var blk : smart_ptr<ExprBlock>; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    return fn_or_block_apply(blk, blk, errors)

[function_macro(name="_soa_fn")]
class SoaFnMarkerMacro : AstFunctionAnnotation
  [unused_argument(func, group, args, errors)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    return true

[block_macro(name="_soa_blk")]
class SoaBlkMarkerMacro : AstBlockAnnotation
  [unused_argument(blk, group, args, errors)] def override apply(var blk : smart_ptr<ExprBlock>; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    return true


[macro_function]
def replate_index_and_field(prefix : string; var fields : array<string>; blk : ExpressionPtr)
  var astVisitor = new ReplaceIndexAndFieldsVisitor(prefix, fields)
  var inscope astVisitorAdapter <- make_visitor(*astVisitor)
  visit(blk, astVisitorAdapter)
  astVisitorAdapter := null
  unsafe
    delete astVisitor

class ReplaceIndexAndFieldsVisitor : AstVisitor
  it_prefix : string
  fields : array<string>

  def ReplaceIndexAndFieldsVisitor(prefix : string; var fields_ : array<string>)
    it_prefix = prefix
    fields <- fields_

  [unused_argument(expr)] def override canVisitWithAliasSubexpression(expr : smart_ptr<ExprAssume>) : bool
    return true

  def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr
    if expr.value is ExprAt
      assume at = expr.value as ExprAt
      if (at.subexpr is ExprVar)
        if (at.subexpr as ExprVar).name == it_prefix
          let ename = string(expr.name)
          return <- new [[ExprAt()
            at=expr.at,
            subexpr <- new [[ExprVar() at=expr.at, name := ename]],
            index <- clone_expression(at.index)
            ]]
    return <- expr

  def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr
    let argsNum = length(expr.arguments)
    if (expr.name == "length" || expr.name == "iter_range") && argsNum == 1
      assume arg = expr.arguments[0]
      if (arg is ExprVar) && (arg as ExprVar).name == it_prefix
        var inscope len <- clone_expression(expr)
        var lenCall = len as ExprCall
        lenCall.arguments[0] := null
        lenCall.arguments[0] |> move_new() <| new [[ExprVar() at=arg.at, name := fields[0]]]
        return <- len
    elif (argsNum == 3 && (expr.name == "erase")
      || (argsNum == 2 && (expr.name == "push" || expr.name == "push_clone" || expr.name == "emplace" || expr.name == "erase" || expr.name == "resize" || expr.name == "reserve"))
      || (argsNum == 1 && expr.name == "clear"))
      assume arg = expr.arguments[0]
      if (arg is ExprVar) && (arg as ExprVar).name == it_prefix
        var inscope pushExpr <- clone_expression(expr)
        var pushCall = pushExpr as ExprCall
        pushCall.arguments |> erase(0)
        for field in fields
          pushCall.arguments |> emplace_new <| new [[ExprVar()
            at=arg.at,
            name := field
          ]]
        return <- pushExpr
    else
      var hasSoaArg = false
      for arg in expr.arguments
        if (arg is ExprVar) && (arg as ExprVar).name == it_prefix
          hasSoaArg = true
          break
      if hasSoaArg
        var idx = length(expr.arguments)
        var inscope pushExpr <- clone_expression(expr)
        var pushCall = pushExpr as ExprCall
        while --idx >= 0
          assume arg = expr.arguments[idx]
          if (arg is ExprVar) && (arg as ExprVar).name == it_prefix
            pushCall.arguments |> erase(idx)
            for field in fields
              pushCall.arguments |> emplace_new <| new [[ExprVar()
                at=arg.at,
                name := field
              ]]
        return <- pushExpr
    return <- expr

  def override visitExprFor(var expr : smart_ptr<ExprFor>) : ExpressionPtr
    var soa_index = -1
    for index, its in count(), expr.sources
      if (its is ExprVar)
        if (its as ExprVar).name == it_prefix
          soa_index = index
          break
    if soa_index != -1
      let itPrefix = string(expr.iterators[soa_index])
      var inscope newForExpr <- clone_expression(expr)
      var newFor = newForExpr as ExprFor
      newFor.sources |> erase(soa_index)
      newFor.iterators |> erase(soa_index)
      newFor.iteratorsAt |> erase(soa_index)
      newFor.iteratorsAka |> erase(soa_index)
      var names2 <- collect_and_replace_iterator_fields(itPrefix, newFor.body)
      if names2 |> length == 0
        names2 |> push <| fields[0]
      for itn in names2
        if true
          let si = newFor.iterators |> length()
          newFor.iterators |> resize(si + 1)
          newFor.iterators[si] := "{itPrefix}_{itn}"
          newFor.iteratorsAka |> resize(si + 1)
          newFor.iteratorsAka[si] := ""
          newFor.iteratorsAt |> push(expr.iteratorsAt[soa_index])
          var inscope nsrc <- new [[ExprVar()
            at=expr.sources[soa_index].at,
            name := itn
          ]]
          newFor.sources |> emplace(nsrc)
      newFor.iteratorVariables |> clear()

      return <- newForExpr
    return <- expr

struct MakeFuncArg
  argT : TypeDeclPtr
  argField : bool

def should_check_sizes() : bool
  // we aot compile scripts with the same dev aot compiler to reduce amount of possible configurations
  // for aot compiler to know what kind of build it is compiling we pass -sConfig with the value of the $(config) of the game build
  // this way we can have dev builds with debug checks aoted and release builds with no checks
  if is_in_aot()
    let cmdArgs <- get_command_line_arguments()
    for idx in range(length(cmdArgs) - 1)
      if cmdArgs[idx] == "-sConfig"
        return cmdArgs[idx + 1] == "dev"
    return false
  else
    return get_DAGOR_DBGLEVEL() > 0

[structure_macro(name=soa_template)]
class SoaTemplateStructMacro : AstStructureAnnotation

  [unused_argument(group, args)] def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
    if length(st.fields) == 0
      errors := "soa macro expects at least one field"
      return false
    if show_completion && is_in_completion()
      var inscope tdef <- new [[TypeDecl()
          at=st.at,
          baseType = Type tArray,
          alias := "{st.name}_SOA",
          firstType <- new [[TypeDecl() at=st.at,
              baseType = Type tStructure,
              structType = get_ptr(st)
          ]]
      ]]
      compiling_module() |> add_alias(tdef)
      return true
    // soa
    var inscope stype <- clone_structure(get_ptr(st))
    stype.name := "{stype.name}_SOA"

    var t = get_ptr(stype)
    for fld in t.fields
      if true
        var inscope newType <- make_field_type(fld, errors)
        move(fld._type, newType)

    if length(errors) > 0
      return false

    let checkSizes = should_check_sizes()

    // push
    var inscope push_op <- make_named_call(st, stype, "push", "push", /*isConst*/true, checkSizes)
    compiling_module() |> add_generic(push_op)
    // push_clone
    // TODO: add this function if need it
    // var push_clone_op <- make_named_call(st, stype, "push_clone", "push_clone", true)
    // compiling_module() |> add_function(push_clone_op)
    // erase
    var inscope erase_op <- make_erase(st, stype, checkSizes)
    compiling_module() |> add_function(erase_op)
    // erase2
    var inscope erase_op2 <- make_erase2(st, stype, checkSizes)
    compiling_module() |> add_function(erase_op2)
    // emplace
    var inscope emplace_op <- make_named_call(st, stype, "emplace", "emplace", /*isConst*/false, checkSizes)
    compiling_module() |> add_function(emplace_op)
    // clear
    var inscope clear_op <- make_clear(st, stype, checkSizes)
    compiling_module() |> add_function(clear_op)
    // resize
    var inscope resize_op <- make_resize_reserve(st, stype, "resize", checkSizes)
    compiling_module() |> add_function(resize_op)
    // reserve
    var inscope reserve_op <- make_resize_reserve(st, stype, "reserve", checkSizes)
    compiling_module() |> add_function(reserve_op)

    compiling_module() |> add_structure(stype)
    return true

  def make_field_type(fld : FieldDeclaration; var errors : das_string) : TypeDeclPtr
    if length(fld._type.dim) > 0
      errors := "{errors}\nunsupported type {describe(fld._type)}"
      return <- [[TypeDeclPtr]]

    if fld._type.baseType == Type tBool
      return <- typeinfo(ast_typedecl type<ecs::BoolList>)

    if fld._type.baseType == Type tUInt64
      return <- typeinfo(ast_typedecl type<ecs::UInt64List>)

    if fld._type.baseType == Type tFloat
      return <- typeinfo(ast_typedecl type<ecs::FloatList>)

    if fld._type.baseType == Type tInt8
      return <- typeinfo(ast_typedecl type<ecs::Int8List>)

    if fld._type.baseType == Type tInt16
      return <- typeinfo(ast_typedecl type<ecs::Int16List>)

    if fld._type.baseType == Type tInt
      return <- typeinfo(ast_typedecl type<ecs::IntList>)

    if fld._type.baseType == Type tUInt
      return <- typeinfo(ast_typedecl type<ecs::UInt32List>)

    if fld._type.baseType == Type tInt64
      return <- typeinfo(ast_typedecl type<ecs::Int64List>)

    if fld._type.baseType == Type tInt2
      return <- typeinfo(ast_typedecl type<ecs::IPoint2List>)

    if fld._type.baseType == Type tInt3
      return <- typeinfo(ast_typedecl type<ecs::IPoint3List>)

    if fld._type.baseType == Type tString
      return <- typeinfo(ast_typedecl type<ecs::StringList>)

    if fld._type.baseType == Type tFloat2
      return <- typeinfo(ast_typedecl type<ecs::Point2List>)

    if fld._type.baseType == Type tFloat3
      return <- typeinfo(ast_typedecl type<ecs::Point3List>)

    if fld._type.baseType == Type tFloat4
      return <- typeinfo(ast_typedecl type<ecs::Point4List>)

    if fld._type.baseType == Type tUInt8
      return <- typeinfo(ast_typedecl type<ecs::UInt8List>)

    if fld._type.baseType == Type tUInt16
      return <- typeinfo(ast_typedecl type<ecs::UInt16List>)

    if fld._type.baseType == Type tHandle
      if fld._type.annotation.name == "float3x4" && fld._type.annotation._module.name == "math"
        return <- typeinfo(ast_typedecl type<ecs::TMatrixList>)

      if fld._type.annotation.name == "E3DCOLOR" && fld._type.annotation._module.name == "DagorMath"
        return <- typeinfo(ast_typedecl type<ecs::ColorList>)

      if fld._type.annotation.name == "EntityId" && fld._type.annotation._module.name == "ecs"
        return <- typeinfo(ast_typedecl type<ecs::EidList>)

    errors := "{errors}\nunsupported type {describe(fld._type)}"
    return <- [[TypeDeclPtr]]

  def make_any_named_call(st, stype : StructurePtr; var args : array<MakeFuncArg>; fnName, callName : string; check_sizes : bool) : FunctionPtr
    var inscope pushOp <- new [[Function() at=st.at, atDecl=st.at, name := fnName]]
    pushOp.flags |= FunctionFlags generated
    pushOp.result |> move_new() <| new [[TypeDecl() baseType=Type tVoid, at=st.at]]
    var inscope pushBody <- new [[ExprBlock() at=st.at]]
    if check_sizes && length(st.fields) > 1
      for idx, fld in iter_range(st.fields), st.fields
        if idx > 0
          var inscope compareLen1 <- new [[ExprCall() at=st.at, name := "length"]]
          compareLen1.arguments |> emplace_new <| new [[ExprVar() at=st.at, name := st.fields[0].name]]

          var inscope compareLen2 <- new [[ExprCall() at=st.at, name := "length"]]
          compareLen2.arguments |> emplace_new <| new [[ExprVar() at=st.at, name := fld.name]]

          var inscope compalreLens <- new [[ExprOp2()
            at=st.at,
            op := "==",
            left <- compareLen1,
            right <- compareLen2
          ]]

          var inscope pushVerify <- new [[ExprAssert() at=st.at]]
          pushVerify.arguments |> emplace <| compalreLens
          pushVerify.arguments |> emplace_new <| new [[ExprConstString() at=st.at, value := "length(\"{st.fields[0].name}\") != length(\"{fld.name}\")"]]
          pushBody.list |> emplace(pushVerify)

    for fld in st.fields
      if true
        var inscope pushCall <- new [[ExprCall() at=st.at, name := callName]]
        pushCall.arguments |> emplace_new <| new [[ExprVar() at=st.at, name := fld.name]]
        for idx, arg in iter_range(args), args
          if arg.argT == null
            continue
          if arg.argField
            pushCall.arguments |> emplace_new <| new [[ExprField() at=st.at,
                value <- new [[ExprVar() at=st.at, name := "arg{idx}"]],
                name := fld.name,
                fieldFlags = ExprFieldFieldFlags no_promotion
            ]]
          else
            pushCall.arguments |> emplace_new <| new [[ExprVar() at=st.at, name := "arg{idx}"]]
        pushBody.list |> emplace(pushCall)

    for idx, arg in iter_range(args), args
      if arg.argT == null
        continue
      pushOp.arguments |> emplace_new <| new [[Variable()
        name := "arg{idx}",
        at=st.at,
        _type <- arg.argT
      ]]
    for fld in stype.fields
      pushOp.arguments |> emplace_new <| new [[Variable()
        name := fld.name,
        at=fld.at,
        _type <- clone_type(fld._type)
      ]]
    pushOp.body |> move() <| pushBody
    return <- pushOp

  def make_erase(st, stype : StructurePtr; check_sizes : bool)
    var inscope argT <- new [[TypeDecl() baseType=Type tInt, at=st.at]]
    var inscope args <- [{ MakeFuncArg[] [[MakeFuncArg argT <- argT]] }]
    return <- make_any_named_call(st, stype, args, "erase", "erase", check_sizes)

  def make_erase2(st, stype : StructurePtr; check_sizes : bool)
    var inscope argT <- new [[TypeDecl() baseType=Type tInt, at=st.at]]
    var inscope argT2 <- new [[TypeDecl() baseType=Type tInt, at=st.at]]
    var inscope args <- [{ MakeFuncArg[] [[MakeFuncArg argT <- argT]]; [[MakeFuncArg argT <- argT2]] }]
    return <- make_any_named_call(st, stype, args, "erase", "erase", check_sizes)

  def make_clear(st, stype : StructurePtr; check_sizes : bool)
    var inscope argT : TypeDeclPtr
    var inscope args <- [{ MakeFuncArg[] [[MakeFuncArg argT <- argT]] }]
    return <- make_any_named_call(st, stype, args, "clear", "clear", check_sizes)

  def make_resize_reserve(st, stype : StructurePtr; name : string; check_sizes : bool)
    var inscope argT <- new [[TypeDecl() baseType=Type tInt, at=st.at]]
    var inscope args <- [{ MakeFuncArg[] [[MakeFuncArg argT <- argT]] }]
    return <- make_any_named_call(st, stype, args, name, name, check_sizes)

  def make_named_call(st, stype : StructurePtr; fnName, callName : string; isConst : bool; check_sizes : bool) : FunctionPtr
    var inscope argT <- new [[TypeDecl() baseType=Type tStructure, at=st.at, structType=get_ptr(st), flags= isConst ? TypeDeclFlags constant : bitfield(0)]]
    var inscope args <- [{ MakeFuncArg[] [[MakeFuncArg argT <- argT, argField=true]] }]
    return <- make_any_named_call(st, stype, args, fnName, callName, check_sizes)


[macro_interface]
class CollectAndReplaceIteratorFields : AstVisitor
  prefix : string
  names : table<string; bool>
  def CollectAndReplaceIteratorFields(prefix_ : string)
    prefix = prefix_
  def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr
    if expr.value is ExprVar
      if (expr.value as ExprVar).name == prefix
        let ename = string(expr.name)
        names[ename] = true
        return <- new [[ExprVar() at=expr.at, name := "{prefix}_{ename}"]]
    return <- expr


[macro_function]
def collect_and_replace_iterator_fields(prefix : string; blk : ExpressionPtr) : array<string>
  var names : array<string>
  var astVisitor = new CollectAndReplaceIteratorFields(prefix)
  var inscope astVisitorAdapter <- make_visitor(*astVisitor)
  visit(blk, astVisitorAdapter)
  for n in keys(astVisitor.names)
    names |> push(n)
  astVisitorAdapter := null
  unsafe
    delete astVisitor
  return <- names


// [init]
// def gen()
//   let types <- {{ "tBool" => "BoolList"; "tString" => "StringList";
//     "tFloat" => "FloatList";
//     "tFloat2" => "Point2List"; "tFloat3" => "Point3List"; "tFloat4" => "Point4List";
//     "tUInt8" => "UInt8List"; "tUInt16" => "UInt16List"; "tUInt" => "UInt32List"; "tUInt64" => "UInt64List";
//     "tInt8" => "Int8List"; "tInt16" => "Int16List"; "tInt" => "IntList"; "tInt64" => "Int64List";
//     "tInt2" => "IPoint2List"; "tInt3" => "IPoint3List" }}


//   for k, v in keys(types), values(types)
//     print("    if fld._type.baseType == Type {k}
//       return typeinfo(ast_typedecl type<ecs::{v}>)\n")


//   print("    if fld._type.baseType == Type tHandle")

//   let handleTypes <- {{
//     "math::float3x4" => "TMatrixList"; "ecs::EntityId" => "EidList";
//     "DagorMath::E3DCOLOR" => "ColorList"
//   }}

//   for k, v in keys(handleTypes), values(handleTypes)
//     let pair <- split(k, "::")
//     print("      if fld._type.annotation.name == \"{pair[1]}\" && fld._type.annotation._module.name == \"{pair[0]}\"
//         return typeinfo(ast_typedecl type<ecs::{v}>)\n")
