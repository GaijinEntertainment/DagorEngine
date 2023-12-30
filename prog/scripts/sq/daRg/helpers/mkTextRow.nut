/*
  this function takes text (for example from localization)
  and replace some tokens with darg components, and all left text with mkText functions, returning list of components
  example:
    let text = "hello <<user>>, current time: <<time>>!"
    let mkText = @(text) {rendObj = ROBJ_TEXT, text=text}
    let curUser = Watched("Bob")
    let replaceTable = {
      ["<<user>>"] = {text=curUser.value, rendObj = ROBJ_TEXT, color = Color(255,200,200)},
      ["<<time>>"] = {text=curTime.value, rendObj = ROBJ_TEXT, color = Color(200,255,200)}
    }

    let greeting = @(){
      children = mkTextRow(text, mkText, replaceTable)
      watch = [curUser, curTime]
      flow = FLOW_HORIZONTAL
      gap = hdpx(5)
    }
    result will be text that will be automatically update text with time and username, and time can be disaplayed with clocks widget
*/

function mkTextRow(fullText, mkText, replaceTable) {
  local res = [fullText]
  foreach(key, comp in replaceTable) {
    let curList = res
    res = []
    foreach(text in curList) {
      if (type(text) != "string") {
        res.append(text)
        continue
      }
      local nextIdx = 0
      local idx = text.indexof(key)
      while (idx != null) {
        if (idx > nextIdx)
          res.append(text.slice(nextIdx, idx))
        if (type(comp) == "array")
          res.extend(comp)
        else
          res.append(comp)
        nextIdx = idx + key.len()
        idx = text.indexof(key, nextIdx)
      }
      if (nextIdx < text.len())
        res.append(text.slice(nextIdx))
    }
  }
  return res.map(@(t) type(t) == "string" ? mkText(t) : t)
}

return mkTextRow