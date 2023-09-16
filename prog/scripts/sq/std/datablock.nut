let { isFunction, isDataBlock } = require("underscore.nut")
// Recursive translator to DataBlock data.
// sometimes more conviniet to store, search and use data in DataBlock.
// It saves order of items in tables as an array,
// and block can easily be found by header as in table.

let function fillBlock(id, block, data, arrayKey = "array") {
  if (type(data) == "array") {
    let newBl = id == arrayKey? block.addNewBlock(id) : block.addBlock(id)
    foreach (v in data)
      fillBlock(v?.label ?? arrayKey, newBl, v)
  }
  else if (type(data) == "table") {
    let newBl = id == arrayKey? block.addNewBlock(id) : block.addBlock(id)
    foreach (key, val in data)
      fillBlock(key, newBl, val)
  }
  else {
    if (id == arrayKey)
      block[id] <- data
    else
      block[id] = data
  }
}

// callback(blockValue[, blockName[, index]])
let function eachBlock(db, callback, thisArg = null) {
  if (db == null)
    return

  assert(isDataBlock(db))
  assert(isFunction(callback))
  let numArgs = callback.getfuncinfos().parameters.len() - 1
  assert(numArgs >= 1 && numArgs <= 3)

  let l = db.blockCount()
  for (local i = 0; i < l; i++) {
    let b = db.getBlock(i)
    if (numArgs == 1)
      callback.call(thisArg, b)
    else if (numArgs == 2)
      callback.call(thisArg, b, b.getBlockName())
    else
      callback.call(thisArg, b, b.getBlockName(), i)
  }
}

// callback(paramValue[, paramName[, index]])
let function eachParam(db, callback, thisArg = null) {
  if (db == null)
    return

  assert(isDataBlock(db))
  assert(isFunction(callback))
  let numArgs = callback.getfuncinfos().parameters.len() - 1
  assert(numArgs >= 1 && numArgs <= 3)

  let l = db.paramCount()
  for (local i = 0; i < l; i++)
    if (numArgs == 2)
      callback.call(thisArg, db.getParamValue(i), db.getParamName(i))
    else if (numArgs == 1)
      callback.call(thisArg, db.getParamValue(i))
    else
      callback.call(thisArg, db.getParamValue(i), db.getParamName(i), i)
}

local function copyParamsToTable(db, table = null) {
  table = table ?? {}
  eachParam(db, @(v, n) table[n] <- v)
  return table
}

let function blk2SquirrelObjNoArrays(blk){
  let res = {}
  for (local i=0; i<blk.paramCount(); i++){
    let paramName = blk.getParamName(i)
    let paramValue = blk.getParamValue(i)
    if (paramName not in res)
      res[paramName] <- paramValue
  }
  for (local i=0; i<blk.blockCount(); i++){
    let block = blk.getBlock(i)
    let blockName = block.getBlockName()
    if (blockName not in res)
      res[blockName] <- blk2SquirrelObjNoArrays(block)
  }
  return res
}


let function blk2SquirrelObj(blk){
  let res = {}
  for (local i=0; i<blk.blockCount(); i++){
    let block = blk.getBlock(i)
    let blockName = block.getBlockName()
    if (blockName not in res)
      res[blockName] <- []
   res[blockName].append(blk2SquirrelObj(block))
  }
  for (local i=0; i<blk.paramCount(); i++){
    let paramName = blk.getParamName(i)
    let paramValue = blk.getParamValue(i)
    if (paramName not in res)
      res[paramName] <- []
    res[paramName].append(paramValue)
  }
  return res
}

let function normalizeConvertedBlk(obj){
  let t = type(obj)
  if (t == "array" && obj.len()==1) {
    return normalizeConvertedBlk(obj[0])
  }
  else if (t == "table" || t=="array") {
    return obj.map(normalizeConvertedBlk)
  }
  return obj
}

let function normalizeAndFlattenConvertedBlk(obj){
  let t = type(obj)
  if (t == "array" && obj.len()==1) {
    let el = obj[0]
    if (type(el)=="table" && el.len()==1){
      foreach(v in el){
        return (type(v)=="array") // -unconditional-terminated-loop
          ? v.map(normalizeAndFlattenConvertedBlk)
          : el.map(normalizeAndFlattenConvertedBlk)
      }
    }
    else
      return normalizeAndFlattenConvertedBlk(el)
  }
  else if (t == "table" || t=="array") {
    return obj.map(normalizeAndFlattenConvertedBlk)
  }
  return obj
}

let convertBlkFlat = @(blk) normalizeAndFlattenConvertedBlk(blk2SquirrelObj(blk))
let convertBlk = @(blk) normalizeConvertedBlk(blk2SquirrelObj(blk))

let function getParamsListByName(blk, name){
  let res = []
  for (local j = 0; j < blk.paramCount(); j++) {
    if (blk.getParamName(j)!=name)
      continue
    res.append(blk.getParamValue(j))
  }
  return res
}


let function getBlkByPathArray(path, blk, defaultValue = null) {
  local currentBlk = blk
  foreach (p in path) {
    if (!isDataBlock(currentBlk))
      return defaultValue
    currentBlk = currentBlk?[p]
  }
  return currentBlk ?? defaultValue
}

local function getBlkValueByPath(blk, path, defVal=null) {
  if (!blk || !path)
    return defVal

  let nodes = path.split("/")
  let key = nodes.len() ? nodes.pop() : null
  if (!key || !key.len())
    return defVal

  blk = getBlkByPathArray(nodes, blk, defVal)
  if (blk == defVal || !isDataBlock(blk))
    return defVal
  local val = blk?[key]
  val = (val!=null && (defVal == null || type(val) == type(defVal))) ? val : defVal
  return val
}

local function setFuncBlkByArrayPath(blk, path, func){
  assert(isFunction(func))
  if (type(path) != "array")
    path = [path]
  assert(path.len()>0)
  let valForSet = path[path.len()-1]
  assert(type(valForSet)=="string")

  local got = blk
  foreach (p in path.slice(-1)){
    assert(type(p) == "string")
    if (got?[p] != null)
      got = got[p]
    else {
      got.addBlock(p)
      got = got[p]
    }
  }
  got[valForSet] <- func(got?[valForSet])
  return got
}

return {
  isDataBlock
  fillBlock
  eachBlock
  eachParam
  copyParamsToTable
  getBlkByPathArray
  getBlkValueByPath
  setFuncBlkByArrayPath

/*
   blk is different from squirrelObject\Json. there is no arrays
   But each block or parameter can be unique or can be used multiple times
   this can be translated to squirrel object several ways:
     0) each paramerter and block treated as unique
     1) by treating EACH object\parameter as an array (that can be one element or multiple)
       Example: name{v:i=2; v:i=3, c:t="a"} -> {name = [{v = [2,3], c=["a"]}]}
     2a) by treating each block\param as array if it has more than one element and otherwise as unique type
       Example: name{v:i=2; v:i=3, c:t="a"} -> {name = {v = [2,3], c="a"}}
     2b) optionally we can treat block that has objects of one name as an array
        some{val:i=1, val:i=2} -> some = [1,2] (instead of some = {val = [1,2]} as in 2a)
     3) [Not implemented] with optional schema (separate from blk or built-in) where blk has extended types or path annotations
*/
  blk2SquirrelObjNoArrays //Convert Blk with 0) way - no arrays at all
  blk2SquirrelObj //Convert Blk with 1) way - each value is an array
  normalizeConvertedBlk
  normalizeAndFlattenConvertedBlk
  convertBlk //Convert Blk 2a) way - each more-than-one-element-of-the-name object is an array, others are not
  convertBlkFlat //Convert Blk 2b) way  - each more-than-one-element-of-the-name object is an array and if object has one element that is an array - use its value, others are not

  getParamsListByName//get all params from block by name
}