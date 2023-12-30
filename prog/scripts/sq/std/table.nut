let NO_VALUE = freeze({})

function setValInTblPath(table, path, value){
  let pathLen = path.len()
  local curTbl = table
  foreach (idx, pathPart in path){
    if (idx == pathLen-1)
      curTbl[pathPart] <- value
    else{
      if (!(pathPart in curTbl))
        curTbl[pathPart] <- {}
      curTbl = curTbl[pathPart]
    }
  }
}

function getValInTblPath(table, path, startIdx=0){
  local curTbl = table
  if (path==null)
    return null
  if (startIdx > 0)
    path = path.slice(startIdx)
  foreach (pathPart in path){
    if (pathPart not in curTbl)
      return NO_VALUE
    curTbl = curTbl[pathPart]
  }
  return curTbl
}

function tryGetValInTblPath(table, path){
  foreach (idx, _ in path) {
    let val = getValInTblPath(table, path, idx)
    if (val != NO_VALUE)
      return val
  }
  return NO_VALUE
}

return {
  tryGetValInTblPath
  setValInTblPath
  getValInTblPath
  NO_VALUE
}