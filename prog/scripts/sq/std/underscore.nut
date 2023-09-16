/*
     underscore.js inspired functional paradigm extensions for squirrel
     library is self contained - no extra dependecies, no any game or app specific dependencies
     ALL functions in this library do not mutate data
*/

/*******************************************************************************
 ******************** functions checks*******************
 ******************************************************************************/

/**
  make common iteratee function
*/

let isTable = @(v) type(v)=="table"
let isArray = @(v) type(v)=="array"
let isString = @(v) type(v)=="string"
let isFunction = @(v) type(v)=="function"
let function isDataBlock(obj) {
  //prefer this as it can handle any DataBlock binding and implementation
  if (obj?.paramCount!=null && obj?.blockCount != null)
    return true
  return false
}

let callableTypes = ["function","table","instance"]
let recursivetypes =["table","array","class"]

let function isCallable(v) {
  return callableTypes.indexof(type(v)) != null && (v.getfuncinfos() != null)
}

let function mkIteratee(func){
  let infos = func.getfuncinfos()
  let params = infos.parameters.len()-1
  assert(params>0 && params<3)
  if (params == 3)
    return func
  else if (params==2)
    return function(value, index, _list) {return func.pcall(null, value, index)}
  else
    return function(value, _index, _list) {return func.pcall(null, value)}
}

/**
  Check for proper iteratee and so on - under construction
*/
let function funcCheckArgsNum(func, numRequired){
  let infos = func.getfuncinfos()
  local plen = infos.parameters.len() - 1
  let deplen = infos.defparams.len()
  let isVargv = infos.varargs > 0
  if (isVargv)
    plen -= 2
  let mandatoryParams = plen - deplen
  if (mandatoryParams > numRequired)
    return false
  if ((mandatoryParams <= numRequired) && (plen >= numRequired))
    return true
  if (mandatoryParams <= numRequired && isVargv)
    return true
  return false
}


/*******************************************************************************
 ******************** Collections handling (array of tables) *******************
 ******************************************************************************/

/*
Split list into two arrays:
one whose elements all satisfy predicate and one whose elements all do not satisfy predicate.
predicate is transformed through iteratee to facilitate shorthand syntaxes.
*/
let function partition(list, predicate){
  let ok = []
  let not_ok = []
  predicate = mkIteratee(predicate)
  foreach(index, value in list){
    if (predicate(value, index, list))
      ok.append(value)
    else
      not_ok.append(value)
  }
  return [ok, not_ok]
}

/*******************************************************************************
 ****************************** Table handling *********************************
 ******************************************************************************/


/**
 * A convenient version of what is perhaps the most common use-case for map: extracting a list of property values.
 local stooges = [{name: 'moe', age: 40}, {name: 'larry', age: 50}, {name: 'curly', age: 60}]
  _.pluck(stooges, "name")
  => ["moe", "larry", "curly"]
  if entry doesnt have property it skipped in return value
 */
let function pluck(list, propertyName){
  return list.map(function(v){
    if (propertyName not in v)
      throw null
    return v[propertyName]
  })
/*  local res = []
  foreach (v in list) {
    if (propertyName in v)
      res.append(v.propertyName)
  }
  return res
*/
}

/**
 * Returns a copy of the table where the keys have become the values and the
 * values the keys. For this to work, all of your table's values should be
 * unique and string serializable.
 */
let function invert(table) {
  let res = {}
  foreach (key, val in table)
    res[val] <- key
  return res
}

/**
 * Create new table which have all keys from both tables (or just first table,
   if addParams=true), and for each key maps value func(tbl1Value, tbl2Value)
 * If value not exist in one of table it will be pushed to func as defValue
 */
let function tablesCombine(tbl1, tbl2, func=null, defValue = null, addParams = true) {
  let res = {}
  if (func == null)
    func = function (_val1, val2) {return val2}
  foreach(key, value in tbl1)
    res[key] <- func(value, tbl2?[key] ?? defValue)
  if (!addParams)
    return res
  foreach(key, value in tbl2)
    if (!(key in res))
      res[key] <- func(defValue, value)
  return res
}

let function isEqual(val1, val2, customIsEqual={}){
  if (val1 == val2)
    return true
  let valType = type(val1)
  if (valType != type(val2))
    return false

  if (valType in customIsEqual)
    return customIsEqual[valType](val1, val2)

  if (recursivetypes.contains(valType)) {
    if (val1.len() != val2.len())
      return false
    foreach(key, val in val1) {
      if (!(key in val2))
        return false
      if (!isEqual(val, val2[key], customIsEqual))
        return false
    }
    return true
  }

  if (valType == "instance") {
    foreach(classRef, func in customIsEqual)
      if (val1 instanceof classRef && val2 instanceof classRef)
        return func(val1, val2)
    return false
  }

  return false
}
/*
* make list of the same order but with unique values
* equals to python list(set(<list>)), and with optional hash function
* (for example to extract key form list of tables to make unique by that)
*/
let function unique(list, hashfunc=null){
  let values = {}
  let res = []
  hashfunc = hashfunc ?? @(v) v
  foreach (v in list){
    let hash = hashfunc(v)
    if (hash in values)
      continue
    values[hash]<-true
    res.append(v)
  }
  return res
}
/*
foreach (k, v in range(-1, -5, -1))
  print($"{v}  ")
print("\n")
// -1  -2  -3  -4
*/
let function range(m, n=null, step=1) {
  let start = n==null ? 0 : m
  let end = n==null ? m : n
  for (local i=start; (end>start) ? i<end : i>end; i+=step)
    yield i
}

//not recursive isEqual, for simple lists or tables
let function isEqualSimple(list1, list2, compareFunc=null) {
  compareFunc = compareFunc ?? @(a,b) a!=b
  if (list1 == list2)
    return true
  if (type(list1) != type(list2) || list1.len() != list2.len())
    return false
  foreach (key, val in list1) {
    if (key not in list2 || compareFunc(val, list2[key]))
      return false
  }
  return true
}

//create from one-dimentional array two-dimentional array by slice it to rows with fixed amount of columns
let function arrayByRows(arr, columns) {
  let res = []
  for(local i = 0; i < arr.len(); i += columns)
    res.append(arr.slice(i, i + columns))
  return res
}

/*
**Chunk a single array into multiple arrays, each containing count or fewer items.
*/

let function chunk(list, count) {
  if (count == null || count < 1) return []
  let result = []
  local i = 0
  let length = list.len()
  while (i < length) {
    let n = i + count
    result.append(list.slice(i, n))
    i = n
  }
  return result
}

/**
 * Given a array, and an iteratee function that returns a key for each
 * element in the array (or a property name), returns an object with an index
 * of each item.
 */
let function indexBy(list, iteratee) {
  let res = {}
  if (isString(iteratee)){
    foreach (val in list)
      res[val[iteratee]] <- val
  }
  else if (isFunction(iteratee)){
    foreach (idx, val in list)
      res[iteratee(val, idx, list)] <- val
  }

  return res
}

let function deep_clone(val) {
  if (!recursivetypes.contains(type(val)))
    return val
  return val.map(deep_clone)
}


//Updates (mutates) target arrays and tables recursively with source
/*
 * - if types of target and source doesn't match, target will be overwritten by source
 * - primitive types and arrays at target will be fully overwritten by source
 * - values of target table with same keys from source table will be overritten
 * - new key value pairs from source table will be added to target table
 * - it's impossible to delete key from target table, only overwrite with null value
 */
let function deep_update(target, source) {
  if ((recursivetypes.indexof(type(source)) == null)) {
    target = source
    return target
  }
  if (type(target)!=type(source) || isArray(source)){
    target = deep_clone(source)
    return target
  }
  foreach(k, v in source){
    if (!(k in target)){
      target[k] <- deep_clone(v)
    }
    else if (!recursivetypes.contains(type(v))){
      target[k] = v
    }
    else {
      target[k]=deep_update(target[k], v)
    }
  }
  return target
}

//Creates new value from target and source, by merges (mutates) target arrays and tables recursively with source
let function deep_merge(target, source) {
  let ret = deep_clone(target)
  return deep_update(ret, source)
}
//
let function flatten(list, depth = -1, level=0){
  if (!isArray(list))
    return list
  let res = []
  foreach (i in list){
    if (!isArray(i) || level==depth)
      res.append(i)
    else {
      res.extend(flatten(i, depth, level))
    }
  }
  return res
}


/*
 do_in_scope(object, function(on_enter_object_value))
 * like python 'with' statement

examples:
```
let {set_huge_alloc_threshold} = require("dagor.memtrace")
local class ChangeAllocThres{
  _prev_limit = null
  _limit = null
  constructor(new_limit){
    this._limit = new_limit
  }
  __enter__ = @() this._prev_limit = set_huge_alloc_threshold(this._limit)
  __exit__ = @() set_huge_alloc_threshold(this._prev_limit)
}

local a = do_in_scope(ChangeAllocThres(8<<10), @(...) array(10000000, {foo=10}))
```
*/

let function do_in_scope(obj, doFn){
  assert(
    type(obj)=="instance" &&  "__enter__" in obj && "__exit__" in obj,
    "to support 'do_in_scope' object passed as first argument should implement '__enter__' and '__exit__' methods"
  )
  assert(type(doFn) == "function", "function should be passed as second argument")

  let on = obj.__enter__()
  local err
  local res
  try{
    res = doFn(on)
  }
  catch(e){
    println($"Catch error while doing action {e}")
    err = e
  }
  obj.__exit__()
  if (err!=null)
    throw(err)
  return res
}

let function insertGap(list, gap){
  let res = []
  let len = list.len()
  foreach (idx, l in list){
    res.append(l)
    if (idx==len-1)
      break
    res.append(gap)
  }
  return res
}

return {
  invert
  tablesCombine
  isEqual
  isEqualSimple
  funcCheckArgsNum
  partition
  pluck
  range
  do_in_scope
  unique
  arrayByRows
  chunk
  isTable
  isArray
  isString
  isFunction
  isCallable
  isDataBlock
  indexBy
  deep_clone
  deep_update
  deep_merge
  flatten
  insertGap
}
