from "frp" import Watched
let {debounce} = require("%sqstd/timers.nut")
let {saveJson, loadJson} = require("%sqstd/json.nut")

function WatchedJsonFile(fileName){
  let load = @() loadJson(fileName)
  let fileContent = Watched(load() ?? {})
  local value = fileContent.get()
  function saveImmediate(){
    saveJson(fileName, fileContent.get())
  }
  let save = debounce(saveImmediate, 0.05)

  function update(object){
    fileContent.mutate(@(v) v.__update(object))
    value = fileContent.get()
    save()
  }
  function deleteKey(key){
    fileContent.mutate(@(v) v.$rawdelete(key))
    value = fileContent.get()
    save()
  }
  return {save, update, load, fileContent, value, saveImmediate, deleteKey}
}


return {WatchedJsonFile}