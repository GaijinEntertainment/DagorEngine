from "%darg/ui_imports.nut" import *
from "dagor.fs" import scan_folder, dir_exists
from "dagor.system" import argv, get_arg_value_by_name

let show_extensions=[".ui.nut", ".ddsx", ".png", ".jpg",".svg",".tga", ".nut", ".ivf", ".ogm",".ogg",".avif",".hlsl"/*".txt"*/]

let vrom_file_tree = function(){
  let vrom_files = dir_exists("prog_samples") ? [] : scan_folder({root=".", vromfs = true, realfs = false, recursive = true}).map(@(v) $"./{v}")
  let res = {}
  function set_ftree_value(path, value) {
    if (path in res){
      if (res[path].findindex(@(v) v.name==value.name) == null)
        res[path].append(value)
    }
    else
      res[path] <- [value]
  }
  foreach (f in vrom_files){
    let path = f.split("/")
    let pathLen = path.len()
    foreach (idx, name in path){
      set_ftree_value("/".join(path.slice(0, idx)), {name, isDir=idx < pathLen-1})
    }
  }
  return res
}()

function listdir(path_or_options=null){
  if (type(path_or_options) != "table") {
    path_or_options = {path = path_or_options}
  }
  local path = path_or_options?.path
  let {recursive = false} = path_or_options
  path = path ?? "."
  let sorter = @(a,b) a.name<=>b.name
  let files = scan_folder({root=path, vromfs = false, realfs = true, recursive})
    .map(function(v) {
      let p = v.split("/")
      let name = p[p.len()-1]
      foreach (n in show_extensions)
        if (name.endswith(n))
          return {name, isDir=false}
      throw null
    })
  let vroms_files = vrom_file_tree?[path].filter(@(v) !v.isDir) ?? []
  files.extend(vroms_files).sort(sorter)
  let dirs = scan_folder({root=path, vromfs = false, realfs = true, recursive, directories_only=true})
    .map(function(v) {
      let p = v.split("/")
      let name = p[p.len()-1]
      if (name == "CVS")
        throw null
      return {name, isDir=true}
    })
  let vroms_dirs = vrom_file_tree?[path].filter(@(v) v.isDir) ?? []
  dirs.extend(vroms_dirs).sort(sorter)
  return dirs.extend(files)
}

function getStartPathImpl(fullPath) {
  local path = fullPath.replace("\\", "/")
  path = (path.indexof(".") == 0) || path.indexof(":") == 1 ? path : $"./{path}"
  local file = path.split("/").top()

  if (file.indexof(".") == null || file.indexof(".") == file.len() - 1) //folder
    return { startPath = path }
  return {
    startPath = path.slice(0, -file.len()-1)
    startFile = path
  }
}

function getStartPath(){
  let fullPath = get_arg_value_by_name("path")
  if (fullPath)
    return getStartPathImpl(fullPath)

  foreach(arg in argv)
    if (arg.indexof(":\\") == 1)
      return getStartPathImpl(arg)
  return null
}

let { startPath = ".", startFile = null } = getStartPath()

return {
  startPath
  startFile
  listdir
}
