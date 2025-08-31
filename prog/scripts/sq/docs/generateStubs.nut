import "io" as io
from "dagor.fs" import file_exists, mkdir
from "dagor.system" import argv, get_arg_value_by_name
from "modules" import get_native_module_names
from "%sqstd/moduleInfo.nut" import mkStubStr
let log = require("%sqstd/log.nut")().log
/*
  allow generate stubs for native modules
  TODO:
    we need return type of function or it is mostly useless even for stubs
    class are not generated yet
*/
function saveFile(file_path, data){
  assert(type(data) == "string", "data should be string")
  let file = io.file(file_path, "wt+")
  file.writestring(data)
  file.close()
  return true
}

function generateStubs(stubsDir="", verbose=false){
  stubsDir = stubsDir ?? ""
  foreach(nm in get_native_module_names()) {
    local path = stubsDir=="" ? nm : $"{stubsDir}/{nm}"
    let manualModuleInfoPath = $"mans/{nm}.info.nut"
    let manModuleInfo = require_optional(manualModuleInfoPath)
    log($"generating stub for '{nm}' in path '{path}'")
    if (!file_exists(stubsDir))
      mkdir(stubsDir)
    saveFile(path, $"return {mkStubStr(require(nm), null, 0, verbose, manModuleInfo)}")
  }
  log("all done\n")
}

if (__name__ == "__main__") {
  let verbose = argv.contains("-v") || argv.contains("--verbose")
  if (argv.contains("-build")) {
    let dir = get_arg_value_by_name("dir") ?? "../stubs"
    generateStubs(dir, verbose)
  }
  else{
    log("to generate stubs to ../stubs dir add '-build' in arguments\n","-v for verbosity")
  }
}
