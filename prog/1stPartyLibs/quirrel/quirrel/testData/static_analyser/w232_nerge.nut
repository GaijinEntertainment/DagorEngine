

let codeSet = {}

let curcode = 10
local haveFound=false


foreach (codes in codeSet){
  foreach (code in codes){
    if (curcode==code) {
      haveFound = true
      break
    }
  }
  if (haveFound)
    break
}