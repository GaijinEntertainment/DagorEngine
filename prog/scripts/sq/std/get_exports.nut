local argv = require_optional("dagor.system")?.argv ?? ::__argv


let function get_arg_value_by_name(name){
  name = $"-{name}"
  foreach (a in argv){
    let l = name.len()
    if (a.slice(0,l) == name){
      if (a.len() > l)
        return a.slice(l+1)
      else
        return a.slice(l)
    }
  }
  return null
}

let function get_all_exports_for_file(fpath){
  let m = require(fpath.replace("\\","/"))
  if (type(m) != "table")
    return null
  return m.keys()
}

let __file__ = __filename__
let function get_all_exports_in_folder(root = "."){
  let {scan_folder} = require("dagor.fs")
  let res = {}
  foreach (file in scan_folder({root, vromfs = false, realfs = true, recursive = true, files_suffix=".nut"})){
    local fpath = file.split("/")
    if (fpath[0]==".")
      fpath = fpath.slice(1)
    let fname = fpath[fpath.len()-1]
    fpath = "/".join(fpath)
    if (fpath in res || fname == __file__)
      continue
    try{
      let fields = get_all_exports_for_file(fpath)
      if (type(fields) != "array")
        continue
      res[fpath] <- fields
    }
    catch(e){
      println($"error in loading {fpath}: {e}")
    }
  }
  foreach (f, fields in res)
    println($"File:'{f}'\n{" ".join(fields)}\n")
  return res
}

if (__name__ == "__main__"){
  let fpath = get_arg_value_by_name("file")
  let dir = get_arg_value_by_name("dir")
  if (dir!=null){
    get_all_exports_in_folder(dir)
  }
  else if (fpath!=null){
    let fields = get_all_exports_for_file(fpath)
    if (type(fields) == "array")
      println(" ".join(["EXPORT FIELDS:"].extend(fields)))
  }
  else
    println($"Usage: csq {__file__} -file:<file> or -dir:<dir>")
}