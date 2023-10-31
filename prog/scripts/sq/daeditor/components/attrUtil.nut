from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *
let { regexp, strip, format } = require("string")
let dagorMath = require("dagor.math")
let {tostring_r} = require("%sqstd/string.nut")
let {command} = require("console")
let math = require("math")

let rexFloat = regexp(@"(\+|-)?([0-9]+\.?[0-9]*|\.[0-9]+)([eE](\+|-)?[0-9]+)?")
let rexInt = regexp(@"[\+\-]?[0-9]+")
let tofloat = @(v) v.tofloat()
let tointeger = @(v) v.tointeger()
let isStrInt = @(v) rexInt.match(strip(v))
let isStrFloat = @(v) rexFloat.match(strip(v))
let function isStrBool(text){
  let s = strip(text)
  return (s == "true" || s == "1" || s == "false" || s == "0")
}
let function isValueTextValid(comp_type, text) {
  let simpleTypeFunc = {
    string = @(_) true
    integer = isStrInt
    float = isStrFloat
    bool = isStrBool
  }?[comp_type]

  if (simpleTypeFunc)
    return simpleTypeFunc(text)

  let nFields = {Point2=2, Point3=3, DPoint3=3, Point4=4, TMatrix=3}.map(@(v) [v, isStrFloat]).__update(
        {IPoint2=2, IPoint3=3, E3DCOLOR=4}
        .map(@(v) [v, isStrInt])
      )?[comp_type]

  if (nFields) {
    let fields = text.split(",")
    if (fields.len()!=nFields[0])
      return false
    return fields.reduce(@(a,b) a && nFields[1](b))
  }
  return false
}

let function call_strfunc(str, func){
  local ret = str
  try {
    ret = func.pcall(str)
  }
  catch(e){
    logerr($"can't convert to string {type(str)} = {str}")
  }
  return ret
}

let function safe_cvt_txt(str, func, empty){
  return str!="" ? call_strfunc(str, func) : empty
}

let dagorMathClassFields = {
  Point2 = ["x", "y"],
  Point3 = ["x", "y", "z"],
  DPoint3 = ["x", "y", "z"],
  Point4 = ["x", "y", "z", "w"],
  IPoint2 = ["x", "y"],
  IPoint3 = ["x", "y", "z"]
}


let function convertTextToValForDagorClass(name, fields){
  let classFields = dagorMathClassFields?[name]
  if (fields.len()!=classFields?.len?())
    return null
  let res = dagorMath[name]()
  classFields?.each(@(key, idx) res[key] = fields[idx])
  return res
}

let convertTextToValFuncs = {
  string = @(v) v,
  integer = @(v) safe_cvt_txt(v, "".tointeger, 0),
  float = @(v) safe_cvt_txt(v, "".tofloat, 0.0)
  bool = function(s){
    s = strip(s)
    if (s=="true" || s=="1")
      return true
    if (s=="false" || s=="0")
      return false
    return null
  }
}

let function convertTextToVal(cur_value, comp_type, text) {
  if (convertTextToValFuncs?[comp_type] != null)
    return convertTextToValFuncs[comp_type](text)

  let fields = text.split(",")
  let floatDagorMathTypes = ["Point2", "Point3", "DPoint3", "Point4"]
  if (floatDagorMathTypes.indexof(comp_type) != null)
    return convertTextToValForDagorClass(comp_type, fields.map(pipe(strip, tofloat)))

  let intDagorMathTypes = ["IPoint2", "IPoint3"]
  if (intDagorMathTypes.indexof(comp_type) != null)
    return convertTextToValForDagorClass(comp_type, fields.map(pipe(strip, tointeger)))

  if (comp_type == "E3DCOLOR") {
    if (fields.len()!=4)
      return null
    let f = fields.map(pipe(strip, tointeger, @(v) math.clamp(v, 0, 255)))
    let res = dagorMath.E3DCOLOR()
    foreach (idx, field in ["r","g","b","a"]) {
      res[field] = f[idx]
    }
    return res
  }

  if (comp_type == "TMatrix") {
    let res = cur_value
    res[3] = convertTextToValForDagorClass("Point3", fields.map(pipe(strip, tofloat)))
    return res
  }

  return null
}

let map_type_to_str = {
  float =  function(v){
    let tf = v.tostring()
    return (tf.indexof(".") != null || tf.indexof("e") != null) ? tf : $"{tf}.0"
  },
  ["null"] = @(_) "null",
}
let map_class_to_str = {
  [dagorMath.Point2] = @(v) format("%.4f, %.4f", v.x, v.y),
  [dagorMath.Point3] = @(v) format("%.4f, %.4f, %.4f", v.x, v.y, v.z),
  [dagorMath.DPoint3] = @(v) format("%.4f, %.4f, %.4f", v.x, v.y, v.z),
  [dagorMath.Point4] = @(v) format("%.4f, %.4f, %.4f, %.4f", v.x, v.y, v.z, v.w),
  [dagorMath.IPoint2] = @(v) format("%d, %d", v.x, v.y),
  [dagorMath.IPoint3] = @(v) format("%d, %d, %d", v.x, v.y, v.z),
  [dagorMath.E3DCOLOR] = @(v) format("%d, %d, %d, %d", v.r, v.g, v.b, v.a),
  [dagorMath.TMatrix] = function(v){
    let pos = v[3]
    return format("%.2f, %.2f, %.2f", pos.x, pos.y, pos.z)
  },
}

let function instance_to_str(v, max_cvstr_len, compValToString_){
  let function objToStr(o){
    local s = format("[%d]={", o.len())
    foreach (val in o) {
      let nexts = "{0}{{1}},".subst(s, compValToString_(val, max_cvstr_len))
      if (max_cvstr_len > 0 && nexts.len() > max_cvstr_len) {
        s = $"{s}..."
        break
      }
      else
        s = nexts
    }
    s = $"{s}\}"
    return s
  }

  let function arrayToStr(o){
    local s = ""
    foreach (fieldName, fieldVal in o.getAll()) {
      if (s.len()>0)
        s = $"{s}|"
      let nexts = $"{s}{fieldName} = {tostring_r(fieldVal)}"
      if (max_cvstr_len > 0 && nexts.len() > max_cvstr_len) {
        s = $"{s}..."
        break
      }
      else
        s = nexts
    }
    return s
  }

  local res =  map_class_to_str?[v?.getclass()]?(v)
  if (res == null) {
    if (v instanceof CompObject)
      res = objToStr(v)
    else if (v instanceof CompArray)
      res = arrayToStr(v)
    else
      res = ""
  }
  return res
}

let function compValToString(v, max_cvstr_len = 80){
  let compValToString_ = callee()
  return type(v) == "instance"
    ? instance_to_str(v, max_cvstr_len, compValToString_)
    : (map_type_to_str?[type(v)]?(v) ?? v.tostring())
}

let function isCompReadOnly(eid, comp_name){
  local object = _dbg_get_comp_val_inspect(eid, comp_name)
  return object?.isReadOnly() ?? false
}

let function getValFromObj(eid, comp_name, path=null){
  local object = _dbg_get_comp_val_inspect(eid, comp_name)
  object = object?.getAll() ?? object
  local res = object
  foreach (key in (path ?? [])) {
    if (key in res) {
      res = res[key]
    }
    else
      break
  }
  return res
}

let function setValToObj(eid, comp_name, path, val){
  let comp = _dbg_get_comp_val_inspect(eid, comp_name)
  if (comp?.isReadOnly() ?? false)
    return
  let object = comp.getAll()
  local res = object
  let lastkey = path?[path.len()-1]
  if (lastkey == null)
    return
  foreach (idx, key in path) {
    if (idx < path.len()-1) {
      if (!(res?[key] != null || key in object || object?.indexof(key)!=null))
        return
      res = res[key]
    }
    else {
      res[lastkey] = val
      obsolete_dbg_set_comp_val(eid, comp_name, object)
      break
    }
  }
}

let function updateComp(eid, comp_name){
  command?($"ecs.update_component {eid} {comp_name}")
}

let exports = {
  isCompReadOnly
  getValFromObj
  setValToObj
  isValueTextValid
  convertTextToVal
  compValToString
  updateComp

  ecsTypeToSquirrelType = {
    //[TYPE_NULL] = null
    [TYPE_STRING] = "string",
    [TYPE_INT] = "integer",
    [TYPE_FLOAT] = "float",
    [TYPE_POINT2] = "Point2",
    [TYPE_POINT3] = "Point3",
    [TYPE_DPOINT3] = "DPoint3",
    [TYPE_POINT4] = "Point4",
    [TYPE_IPOINT2] = "IPoint2",
    [TYPE_IPOINT3] = "IPoint3",
    [TYPE_BOOL] = "bool",
    [TYPE_COLOR] = "E3DCOLOR",
    [TYPE_MATRIX] = "TMatrix",
    [TYPE_EID] = "integer",
  }
}

return exports
