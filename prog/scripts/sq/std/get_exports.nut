from "modules" import get_native_module_names
from "dagor.system" import get_arg_value_by_name, get_all_arg_values_by_name
from "dagor.fs" import scan_folder
from "json.nut" import saveJson


function get_all_exports_for_file(fpath){
  return require(fpath)
}
function getClassOfInstanceName(v){
  if (type(v)=="instance" && typeof(v)!="instance")
    return typeof(v)
  return v?.__name__ ?? v?.__name ?? v?.name ?? v?.getName()
}

function isNativeClassOrInstance(v){
  let tt = type(v)
  return (tt=="instance" || tt=="class") && "__getTable" in v && "__setTable" in v
}

local make_module_description
make_module_description = function(module, frozen=null){
  let t = type(module)
  if (isNativeClassOrInstance(module))
    return {"type":t, frozenable=true, name=getClassOfInstanceName(module)}
  if (t != "table" && t !="array")
    return {"type":t}
  if (module.is_frozen() || frozen) {
    return {
      "type":t,
      frozen = true
      content = module.$map(function(v) {
        let tt = type(v)
        if (tt == "table" || tt == "array")
          return {frozen=true, "type":tt, content = v.$map(@(i) make_module_description(i, true))}
        if (tt=="class" || tt=="instance" )
          return {frozen=true, "type":tt, name=getClassOfInstanceName(v)}
        return {frozen=true, "type":tt}
      })
    }
  }
  return {
    "type":t,
    frozen = false
    content = module.$map(function(v) {
      let tt = type(v)
      if (isNativeClassOrInstance(v))
        return {"type":tt, frozenable=true, name=getClassOfInstanceName(v)}
      if (tt =="array" || tt=="table"){
        frozen = frozen || v.is_frozen()
        return {frozen, "type":tt}.__update( tt =="array" || tt=="table" ? {content = v.$map(@(i) make_module_description(i, frozen))} : {}, tt=="instance" ? {name=getClassOfInstanceName(v)} : {})
      }
      return {"type":tt}
    })
  }
}

let __file__ = __filename__
function get_all_exports_in_folder(root = ".", res=null){
  res = res ?? {}
  foreach (file in scan_folder({root, vromfs = false, realfs = true, recursive = true, files_suffix=".nut"})){
    local fpath = file.split("/")
    if (fpath[0]==".")
      fpath = fpath.slice(1)
    //let fname = fpath[fpath.len()-1]
    fpath = "/".join(fpath)
    if (fpath in res || __file__.indexof(fpath)!=null)
      continue
    try{
      let info = get_all_exports_for_file(fpath)
      res[fpath] <- make_module_description(info)
    }
    catch(e){
      println($"error in loading {fpath}: {e}")
    }
  }
  return res
}

if (__name__ == "__main__"){
  const defOut = "modules_info.json"
  let res = {}
  let fpaths = get_all_arg_values_by_name("file") ?? []
  let outFname = get_arg_value_by_name("out")
  let replacePath = get_arg_value_by_name("replace")
  let dirs = get_all_arg_values_by_name("dir") ?? []
  if (dirs.len() + fpaths.len() != 0 ) {
    foreach(dir in dirs)
      get_all_exports_in_folder(dir, res)
    foreach(fpath in fpaths)
      res[fpath] <- make_module_description(get_all_exports_for_file(fpath))
    if (outFname==null) {
      foreach (f, module_info in res) {
        let info = type(module_info) != "table"
          ? ""
          : "  ".join(module_info.reduce(@(r, _, k) r.append($"{k}"), []))
        println($"File:'{f}'\n{info}\n")
      }
    }
//    foreach(nm in get_native_module_names())
//      res[nm] <- make_module_description(require(nm))
    if (outFname) {
      if (replacePath) {
        assert (replacePath.split(";").len() == 2, @() $"{replacePath} is incorrect format, should be '<replace_prefix>';<replace_with>, like 'ui/;ui/' ")
        let [replaceFrom, replaceTo] = replacePath.split(";")
        let newres = {}
        foreach (k, v in res) {
          println(k)
          if (k.startswith(replaceFrom)){
            newres["".concat("%", replaceTo, k.slice(replaceFrom.len()))] <- v
          }
          else
            newres[k] <- v
        }
        res.clear()
        res.__update(newres)
      }
      saveJson(outFname, res)
    }
  }
  else
    println($"Usage: csq --set-sqconfig:<path to .sqconfig> {__file__} -file:<file> or -dir:<dir> [-out:{defOut}] [-replace:replace_path_prefix;with_this_mount_name]")
}