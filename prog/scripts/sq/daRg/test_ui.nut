from "debug" import getstackinfos
let {exit, get_arg_value_by_name} = require("dagor.system")
let {tostring_r} = require("%sqstd/string.nut")

let knownProps = ["size","rendObj","watch","behavior","halign","valign","flow","pos","hplace","vplace","padding", "margin", "eventHandlers", "hotkeys"].reduce(function(res, a) {res[a] <- a; return res}, {})
function checkIsUiComponent(table) {
  if (table.len()==0)
    return true
  foreach(k in knownProps)
    if (k in table)
      return true
  println("not darg ui table, error in component:")
  println(tostring_r(table))
  return false
}

function testUi(entry){
  if (entry==null)
    return true
  if (type(entry)=="function")
    entry=entry()
  let t = type(entry)
  if (t=="table" || t=="class") {
    if ("children" not in entry)
      return checkIsUiComponent(entry)
    if ("array"==type(entry.children)) {
      foreach (child in entry.children) {
        if (!testUi(child))
          return false
      }
      return true
    }
    return testUi(entry.children)
  }
  return false
}

function test(entryPoint = null){
  entryPoint = entryPoint ?? get_arg_value_by_name("ui")
  if (entryPoint==null) {
    println($"Usage: csq {__FILE__} -ui:<path_to_darg_ui.nut>")
    exit(0)
  }
  println($"starting test for '{entryPoint}'...")
  if (!testUi(require(entryPoint))){
    println("failed to run")
    exit(1)
  }
  println("all ok")
}

if (__name__ == "__main__") {
  test()
}

return {testUi, test, checkIsUiComponent}