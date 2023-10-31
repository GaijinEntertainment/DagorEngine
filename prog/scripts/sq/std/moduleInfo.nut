let log = require("%sqstd/log.nut")().log
let math = require("math")

let typesByTypechecks ={
  [0x00000001] = "null",
  [0x00000002] = "integer",
  [0x00000004] = "float",
  [0x00000008] = "bool",
  [0x00000010] = "string",
  [0x00000020] = "table",
  [0x00000040] = "array",
  [0x00000080] = "userdata",
  [0x00000100] = "closure",
  [0x00000200] = "nativeclosure",
  [0x00000400] = "generator",
  [0x00000800] = "userpointer",
  [0x00001000] = "thread",
  [0x00002000] = "funcproto", //internal type
  [0x00004000] = "class",
  [0x00008000] = "instance",
  [0x00010000] = "weakref",
  [0x00020000] = "outer", //internal type
}
let function mkAssertStr(x, argname, verbose=false){
  if (verbose)
    log("type", x, "argname", argname)
  if (x < 0 || argname == null)
    return ""
  let types = (typesByTypechecks.filter(@(_, bits) (x & bits)!=0)).values()
  let typestr = "[{0}]".subst(", ".join(types.map(@(v) $"\"{v}\"")))
  let infostr = " ".join(types.map(@(v) $"'{v}'"))

  return $"  assert({typestr}.contains(type({argname})), @() $\"type of argument should be one of: {infostr}\")"
}

let function typeCheckArrToStringCheck(mask, arguments, verbose = false) {
  return "\n  ".join(mask.map(@(x, i) mkAssertStr(x, arguments?[i], verbose)).filter(@(v) v!=""))
}

let valuesByTypechecks ={
  [0x00000001] = "null",
  [0x00000002] = "0",
  [0x00000004] = "0.0",
  [0x00000008] = "true",
  [0x00000010] = "\"\"",
  [0x00000020] = "{}",
  [0x00000040] = "[]",
  [0x00000080] = "userdata",
  [0x00000100] = "@(...) null",
  [0x00000200] = "nativeclosure",
  [0x00000400] = "generator",
  [0x00000800] = "userpointer",
  [0x00001000] = "thread",
  [0x00002000] = "funcproto", //internal type
  [0x00004000] = "class{}",
  [0x00008000] = "class{}()",
  [0x00010000] = "weakref",
  [0x00020000] = "outer", //internal type
}
let function typeBitsToStringFirst(x) {
  if (x==null || x < 0)
    return "null"
  return (valuesByTypechecks.filter(@(_, bits) (x & bits)!=0)).values()?[0] ?? "null"
}

let def_params_names = ["a", "b", "c", "d", "e"].extend(array(10).map(@(_, i) $"var_{i+5}"))

const INDENT_SYM = "  "

let function mkFunStubStr(func, name=null, indent = 0, verbose=false, manualModInfo=null){
  let infos = func.getfuncinfos()
  let indentStr = "".join(array(indent, INDENT_SYM))
  local retValueStr = manualModInfo?[name].returnStr
  if (retValueStr)
    retValueStr = $"\n{indentStr}{INDENT_SYM}return {retValueStr}\n{indentStr}"
  else
    retValueStr = " "
  let argumentsNames = manualModInfo?[name].arguments ?? def_params_names
  let {paramscheck, typecheck} = infos
  name = name ?? infos.name
  name = name!=null ? $" {name}" : ""
  if (typecheck!=null)
    typecheck.remove(0)
  let actParams = paramscheck == 0
    ? paramscheck
    : paramscheck > 0
      ? paramscheck-1
      : ((-paramscheck)-1)
  let varargs = paramscheck > 0
    ? ""
    : paramscheck < -1
      ? ", ..."
      : "..."
  local args = array(math.max(actParams, typecheck?.len() ?? 0)).map(@(_, i) argumentsNames[i])
  let defined_args = args.map(function(arg, i){
    let isOptional = paramscheck < 0 && i >= actParams && varargs != ""
    return isOptional
      ? $"{arg} = {typeBitsToStringFirst(typecheck[i])}"
      : arg
  })
  let args_string = "{0}{1}".subst(", ".join(defined_args), varargs)
  if (verbose)
    log(name, args, infos)
  let typechecks = typecheck!=null ? typeCheckArrToStringCheck(typecheck, args, verbose) : ""
  return name == " constructor"
    ? $"constructor({args_string})\{\}"
    : typechecks == ""
      ? $"function{name}({args_string}) \{{retValueStr}\}"
      : retValueStr != " "
        ? $"function{name}({args_string}) \{\n{indentStr}{typechecks}{retValueStr}\}"
        : $"function{name}({args_string}) \{\n{indentStr}{typechecks}\n{indentStr}\}"
}

let function mkStubStr(val, name=null, indent=0, verbose = false, manualModInfo=null){
  let typ = type(val)
  let indentStr = "".join(array(indent, INDENT_SYM))
  let mkStubSt = callee()
  if (["string", "float", "integer", "boolean"].contains(typ))
    return name == null ? val.tostring() : $"{indentStr}{name} = {val.tostring()}"
  if (typ=="function")
    return  $"{indentStr}{mkFunStubStr(val, name, indent, verbose, manualModInfo)}"
  if (typ=="table"){
    let res = [name!=null ? $"{indentStr}{name} = \{" : $"{indentStr}\{"]
    foreach(k, v in val){
      res.append(mkStubSt(v, k, indent+1, verbose, manualModInfo))
    }
    res.append($"{indentStr}\}")
    return "\n".join(res)
  }
  if (typ=="class"){
    let res = [name==null ? $"{indentStr}class\{" : $"{indentStr}{name} = class\{"]
    foreach(k, v in val){
      res.append(mkStubSt(v, k, indent+1))
    }
    res.append($"{indentStr}\}")
    return "\n".join(res)
  }
  if (typ == "array") {
    if (name=="argv")//hack for bad dagor.system api
      return $"{indentStr}argv = []"
    return name == null
      ? $"{indentStr}[{", ".join(val.map(@(j) mkStubSt(j, null, 0, verbose, manualModInfo)))}]"
      : $"{indentStr}{name} = [{", ".join(val.map(@(j) mkStubSt(j, null, 0, verbose, manualModInfo)))}]"
  }
  return name == null ? $"\"{typ}\"" : $"{indentStr}{name} = \"{typ}\""
}

let mkModuleStub = @(nm) mkStubStr(require(nm), nm)


return {
  mkModuleStub
  mkStubStr
}
