let abs = @(v) v> 0 ? v.tointeger() : -v.tointeger()
let { logerr } = require("dagor.debug")
//let { log } = require("log.nut")()

let callableTypes = ["function","table","instance"]
let function isCallable(v) {
  return callableTypes.indexof(type(v)) != null && (v.getfuncinfos() != null)
}
/*
+ partial:
  partial(f(x,y,z), 1) == @(y,z) f(1,y,z)
  partial(f(x,y,z), 1, 2) == @(z) f(1,2,z)
  partial(f(x,y,z), 1, 2, 3) == @() f(1,2,3) or f(1,2,3)
*/
let function partial(func, ...){
  assert(isCallable(func), "partial can be applied only to functions as first arguments")
  let infos = func.getfuncinfos()
  let argsnum = infos.parameters.len()-1
  let isvargved = infos.varargs==1
  let pargs = vargv
  let pargslen = pargs.len()
  if ( (pargslen == argsnum) && !isvargved) {
    return function(){
      return func.acall([null].extend(pargs))
    }
  }
  if ( (pargslen <= argsnum) || isvargved) {
    return function(...){
      return func.acall([null].extend(pargs).extend(vargv))
    }
  }
  assert(false, @() $"function '{infos.name}' cannot be partial with more arguments({pargslen}) that it accepts({argsnum})")
  return func
}

/*
 kwarg function:
  foo(x,y,z)
  kwarg(foo)==@(p) (foo(p?.x, p?.y, p?.z))
  foo(x,y,z=2)
  kwarg(foo)==@(p) (foo(p?.x, p?.y, p?.z ?? 2))
*/
let allowedKwargTypes = { table = true, ["class"] = true, instance = true }
let KWARG_NON_STRICT = persist("KWARG_NON_STRICT", @() freeze({}))
let function kwarg(func){
  assert(isCallable(func), "kwarg can be applied only to functions as first arguments")
  let infos = func.getfuncinfos()
  let funcName = infos.name
  let funcargs = infos.parameters.slice(1)
  let defargs = infos.defparams
  let argsnum = funcargs.len()
  let kfuncargs = {}
  let mandatoryparams = []
  let defparamsStartFrom = argsnum-defargs.len()
  foreach (idx, arg in funcargs) {
    if (idx >= defparamsStartFrom) {
      kfuncargs[arg] <- defargs[idx-defparamsStartFrom]
    }
    else{
      kfuncargs[arg] <-null
      mandatoryparams.append(arg)
    }
  }
  return function kwarged(params = kfuncargs, strict_mode = null) {
    if (type(params) not in allowedKwargTypes)
      assert(false, @() $"param of function can be only hashable (table, class, instance), found:'{type(params)}'")
    let nonManP = mandatoryparams.filter(@(p) p not in params)
    if (nonManP.len() > 0)
      assert(false, "not all mandatory parameters provided: {0}".subst(nonManP.len()==1 ? $"'{nonManP[0]}'" : nonManP.reduce(@(a,b) $"{a},'{b}'")))
    if (strict_mode != KWARG_NON_STRICT) {
      foreach (k, _ in params){
        if (k not in kfuncargs)
          logerr($"unknown argument in function {funcName} call: '{k}'")
        // FIXME: Need to use assert after fix all strict args condition
        //assert(k in kfuncargs, @() $"unknown argument in function {funcName} call: '{k}'")
      }
    }
    let posarguments = funcargs.map(@(kv) kv in params ? params[kv] : kfuncargs[kv])
    posarguments.insert(0, this)
    return func.acall(posarguments)
  }
}
/*
 kwpartial
  local function foo(a,b,c){(a+b)*c}
  kwpartial(foo, {b=3})(1,5) == (1+3)*5
  kwpartial(foo, {b=3}, 2)(5) == (2+3)*5
*/
let function kwpartial(func, partparams, ...){
  assert(isCallable(func), "partial can be applied only to functions as first arguments")
  assert(["table", "class","instance"].indexof(type(partparams))!=null, "kwpartial second argument of function can be only hashable (table, class, instance)")
  let infos = func.getfuncinfos()
  let funcargs = infos.parameters.slice(1)
//  local defargs = infos.defparams
  let argsnum = funcargs.len()
  let posfuncargs = {}
  let partvargs = vargv
  foreach (p, v in partparams){
    let posidx = funcargs.indexof(p)
    if (posidx == null)
      continue
    posfuncargs[posidx] <- v
  }
  return function(...){
    let curargs = partvargs.extend(vargv)
    assert(curargs.len()+posfuncargs.len()>=argsnum, @() $"not enough arguments provided for function '{infos?.name}' to call")
    let finalargs = []
    local provArgIdx = 0
    for (local i=0; i<argsnum; i++) {
      if (i in posfuncargs) {
        finalargs.append(posfuncargs[i])
      }
      else {
        finalargs.append(curargs[provArgIdx])
        provArgIdx++
      }
    }
    return func.acall([this].extend(finalargs))
  }
}

// pipe:
//  pipe(f,g) =  @(x) f(g(x))
//it can be replaced with oneliner pipe = @(...) @(x) vargv.reduce(@(a,b) b(a), x)
let function pipe(...){
  let args = vargv.filter(isCallable)
  assert(args.len() == vargv.len() && args.len()>0, "pipe should be called with functions")
  let finfos = args[0].getfuncinfos()
  let numarg = (finfos.native ? abs(finfos.paramscheck) : finfos.parameters.len()) - 1
  let isvargved = finfos.native ? finfos.paramscheck < -2 : finfos.varargs==1
  assert(numarg==1 || (numarg==0 && isvargved), "pipe cannot be applied to multiargument function or function with no argument")
  return @(x) args.reduce(@(a,b) b(a), x)
}
// compose (reverse to pipe):
//  compose(f,g) =  @(x) g(f(x))
//it can be replaced with oneliner compose = @(...) @(x) vargv.reverse().reduce(@(a,b) b(a), x)
let function compose(...){
  let args = vargv.filter(isCallable)
  assert(args.len() == vargv.len() && args.len()>0, "compose should be called with functions")
  args.reverse()
  let finfos = args[0].getfuncinfos()
  let numarg = (finfos.native ? abs(finfos.paramscheck) : finfos.parameters.len()) - 1
  let isvargved = finfos.native ? finfos.paramscheck < -2 : finfos.varargs==1
  assert(numarg==1 || (numarg==0 && isvargved), "compose cannot be applied to multiargument function or function with no argument")
  return @(x) args.reduce(@(a,b) b(a), x)
}
/*
  tryCatch(tryer function, catcher function) return function that will operate on input safely
*/
let function tryCatch(tryer, catcher){
  return function(...) {
    try{
      return tryer.pacall([null].extend(vargv))
    }
    catch(e)
      return catcher(e)
   }
}

/*
 (un)curry:
  cf = curry(f) == @(x) @(y) @(z) f(x,y,z)
  f(x,y,z) = cf(x)(y)(z)
  cf(x) == @(y) @(z) f(x,y,z)
  local get = curry(function(property, object){ return object?[property] })
  local map = curry(function(fn, value){ return value.map(fn) })

  local objects = [{ id = 1 }, { id = 2 }, { id = 3 }]
  local getIDs = map(get("id"))

  log(objects.map(get("id"))) //= [1, 2, 3]
  log(objects.map(@(v) v?.id)) //= [1, 2, 3]
  log(getIDs(objects)) //= [1, 2, 3]

also our curry is (un)curry - so
local sum = curry(@(a,b,c) a+b+c)
sum(1)(2)(3) == sum(1)(2,3) == sum(1,2,3) == sum(1,2)(3)
unfortunately returning function are now use vargv, instead of rest of parameters (the same issue goes to partial)
*/
let function curry(fn) {
  let finfos = fn.getfuncinfos()
  assert(!finfos.native || finfos.paramscheck >= 0, "Cannot curry native function with varargs")
  let arity = (finfos.native ? finfos.paramscheck : finfos.parameters.len())-1

  return function f1(...) {
    let args = vargv
    if (args.len() >= arity) {
      return fn.acall([this].extend(args))
    } else {
      let fone = f1
      return function(...) {
        let moreArgs = vargv
        let newArgs = clone args
        newArgs.extend(moreArgs)
        return fone.acall([this].extend(newArgs))
      }
    }
  }
}

/**
* memoize(function, [hashFunction])
  Memoizes a given function by caching the computed result. Useful for speeding up slow-running computations.
  If passed an optional hashFunction, it will be used to compute the hash key for storing the result, based on the arguments to the original function.
  The default hashFunction uses all arguments to the memoized function as the keys array.
  heavily optimized for most common cases - memoize by first argument, or by any amount

  NOTES:
  memoize works 20% faster with try catch instead of if to check if there is value in cache
  but 5 times slower on cache population.
  we also need to check if cache is overcrowded and clean it on
  and it would be better to not clean external cache
*/
let NullKey = persist("NullKey", @() {})
let Leaf = persist("Leaf", @() {})
let NO_VALUE = persist("NO_VALUE", @() {})

let function setValInCacheVargved(path, value, cache) {
  local curTbl = cache
  foreach (p in path){
    local pathPart = p ?? NullKey
    if (pathPart not in curTbl)
      curTbl[pathPart] <- {}
    curTbl = curTbl[pathPart]
  }
  curTbl[Leaf] <- value
  return value
}

let function getValInCacheVargved(path, cache) {
  local curTbl = cache
  foreach (p in path) {
    let key = p ?? NullKey
    if (key in curTbl)
      curTbl = curTbl[p ?? NullKey]
    else
      return NO_VALUE
  }
  return (Leaf in curTbl) ? curTbl[Leaf] : NO_VALUE
}

let function setValInCache(path, value, cache) {
  local curTbl = cache
  let n = path.len()-1
  foreach (idx, p in path){
    local pathPart = p ?? NullKey
    if (idx == n) {
      curTbl[pathPart] <- value
      return value
    }
    if (pathPart not in curTbl)
      curTbl[pathPart] <- {}
    curTbl = curTbl[pathPart]
  }
  return value
}

let function getValInCache(path, cache) {
  local curTbl = cache
  foreach (p in path) {
    let key = p ?? NullKey
    if (key in curTbl)
      curTbl = curTbl[p ?? NullKey]
    else
      return NO_VALUE
  }
  return curTbl
}

const DEF_MAX_CACHE_ENTRIES = 10000
let function memoize(func, hashfunc = null, cacheExternal=null, maxCacheNum=DEF_MAX_CACHE_ENTRIES) {
  let cache = cacheExternal ?? {}
  local simpleCache = null
  local simpleCacheUsed = false
  let {parameters=null, varargs=0, defparams=null} = func.getfuncinfos()
  let isVarargved = (varargs > 0) || ((defparams?.len() ?? 0) > 0)
  let parametersNum = (parameters?.len() ?? 0)-1
  let isOneParam = (parametersNum == 1) && !isVarargved
  let isNoParams = (parametersNum == 0) && !isVarargved
  local cacheValues = 0
  if (type(hashfunc)=="function")
    return function memoizedfuncHash(...){
      let args = [null].extend(vargv)
      let hashKey = hashfunc.acall(args) ?? NullKey
      if (hashKey in cache)
        return cache[hashKey]
      cacheValues+=1
      if (cacheValues > maxCacheNum)
        cache.clear()
      let res = func.acall(args)
      cache[hashKey] <- res
      return res
//      try { return cache[hashKey] }
//      catch(e) { return cache[hashKey] <- func.acall(args) }
    }
  else if (isOneParam) {
    return function memoizedfuncOne(v){
      let k = v ?? NullKey
      if (k in cache)
        return cache[k]
      cacheValues+=1
      if (cacheValues > maxCacheNum)
        cache.clear()
      let res = func(v)
      cache[k] <- res
      return res
//      try { return cache[v ?? NullKey] }
//      catch(e) { return cache[v ?? NullKey] <- func(v) }
    }
  }
  else if (hashfunc==1) {
    return function memoizedfunc1(...){
      let key = vargv[0] ?? NullKey
      if (key in cache)
        return cache[key]
      if (vargv.len()>0) {
        cacheValues+=1
        if (cacheValues > maxCacheNum)
          cache.clear()
        let res = func.acall([null].extend(vargv))
        cache[key] <- res
        return res
      }
      if (simpleCacheUsed)
        return simpleCache
      simpleCache = func.acall([null].extend(vargv))
      simpleCacheUsed = true
      return simpleCache
//      try { return cache[vargv[0] ?? NullKey] }
//      catch(e) {
//        if (vargv.len()>0)
//          return cache[vargv[0] ?? NullKey] <- func.acall([null].extend(vargv))
//        if (simpleCacheUsed)
//          return simpleCache
//        simpleCache = func.acall([null].extend(vargv))
//        simpleCacheUsed = true
//        return simpleCache
//      }
    }
  }
  else if (isNoParams)
    return function memoizedfuncNo() {
      if (simpleCacheUsed)
        return simpleCache
      simpleCache = func()
      simpleCacheUsed = true
      return simpleCache
    }
  else if (hashfunc==0) {
    return function memoizedfunc0(...){
      if (simpleCacheUsed)
        return simpleCache
      simpleCache = func.acall([null].extend(vargv))
      simpleCacheUsed = true
      return simpleCache
    }
  }
  else if (type(hashfunc)=="integer") {
    if (isVarargved) {
      return function memoizedfuncIntV(...){
        let path = vargv.slice(0, hashfunc)
        let cached = getValInCacheVargved(path, cache)
        if (cached != NO_VALUE)
          return cached
        cacheValues+=1
        if (cacheValues > maxCacheNum)
          cache.clear()
        return setValInCacheVargved(path, func.acall([null].extend(vargv)), cache)
//        try { return getValInCacheVargved(path, cache) }
//        catch(e) { return setValInCacheVargved(path, func.acall([null].extend(vargv)), cache) }
      }
    }
    return function memoizedfuncInt(...){
      let path = vargv.slice(0, hashfunc)
      let cached = getValInCache(path, cache)
      if (cached != NO_VALUE)
        return cached
      cacheValues+=1
      if (cacheValues > maxCacheNum)
        cache.clear()
      return setValInCache(path, func.acall([null].extend(vargv)), cache)
//      try { return getValInCache(path, cache) }
//      catch(e) { return setValInCache(path, func.acall([null].extend(vargv)), cache) }
    }
  }
  assert(hashfunc == null, "hash function should be null, function or integer of arguments of function")
  if (isVarargved) {
    return function memoizedfuncV(...){
      let cached = getValInCacheVargved(vargv, cache)
      if (cached != NO_VALUE)
        return cached
      cacheValues+=1
      if (cacheValues > maxCacheNum)
        cache.clear()
      return setValInCacheVargved(vargv, func.acall([null].extend(vargv)), cache)
//      try { return getValInCacheVargved(vargv, cache) }
//      catch(e) { return setValInCacheVargved(vargv, func.acall([null].extend(vargv)), cache) }
    }
  }
  return function memoizedfunc(...){
    let cached = getValInCache(vargv, cache)
    if (cached != NO_VALUE)
      return cached
    cacheValues+=1
    if (cacheValues > maxCacheNum)
      cache.clear()
    return setValInCache(vargv, func.acall([null].extend(vargv)), cache)
//    try { return getValInCache(vargv, cache) }
//    catch(e) { return setValInCache(vargv, func.acall([null].extend(vargv)), cache) }
  }
}
//the same function as in underscore.js
//Creates a version of the function that can only be called one time.
//Repeated calls to the modified function will have no effect, returning the value from the original call.
//Useful for initialization functions, instead of having to set a boolean flag and then check it later.
let function once(func){
  local result
  local called = false
  let function memoizedfunc(...){
    if (called)
      return result
    let res = func.acall([null].extend(vargv))
    result = res
    called = true
    return res
  }
  return memoizedfunc
}

//the same function as in underscore.js
//Creates a version of the function that can be called no more than count times.
//The result of the last function call is memoized and returned when count has been reached.
let function before(count, func){
  local called = 0
  local res
  return function beforeTimes(...){
    if (called >= count)
      return res
    called++
    res = func.acall([null].extend(vargv))
    return res
  }
}

//the same function as in underscore.js
//Creates a version of the function that will only be run after being called count times.
//Useful for grouping asynchronous responses, where you want to be sure that all the async calls have finished, before proceeding.
let function after(count, func){
  local called = 0
  return function beforeTimes(...){
    if (called < count) {
      called++
      return
    }
    return func.acall([null].extend(vargv))
  }
}

//generic breakable reduce. By breakable reduce most of other main functional can be implemented
// like (reduce, each, findindex, findvalue, filter, map).Each and reduce can be faster
// it can be done other way, with special type

/*
local BreakValue = class{
  result = null
  constructor(val){
    result = val
  }
}

local MemoNotInited = class{}
local function breakable_reduce(obj, func, memo=MemoNotInited()) {
  local firstInited = !(memo instanceof MemoNotInited)
  local argsnum = func.getfuncinfos().parameters.len()-1
  local nfunc
  if (argsnum==2)
    nfunc = @(prevval, curval, idx, obj_ref) func(prevval, curval)
  else if (argsnum==3)
    nfunc = @(prevval, curval, idx, obj_ref) func(prevval, curval, idx)
  else if (argsnum==4)
    nfunc = func
  else
    assert(false, "function in breakable reduce should have arguments func(prevval, curval, [idx, [obj_ref]])")
  foreach (idx, curval in obj) {
    if (!firstInited){
      memo = curval
      firstInited = true
      continue
    }
    local res = nfunc(memo, curval, idx, obj)
    if (res instanceof BreakValue)
      return res.result
    if (res == BreakValue)
      return memo
    else
      memo = res
  }
  return !firstInited ? null : memo
}
*/
let combine = @(...) @() vargv.each(@(v) v.call(null))


//creates function that will do map 'set' ({key:key}) to table with provided function
//function result will be cached for each key, until key disappears, than cache result for this key would be cleaned

let function mkMemoizedMapSet(func){
  let cache = {}
  let funcParams = func.getfuncinfos().parameters.len()-1
  return function memoizedMapSet(set){
    foreach (k, v in set){
      if (k in cache)
        continue
      cache[k] <- funcParams == 1 ? func(k) : func(k, v)
    }
    let toDelete = []
    foreach(k, _ in cache) {
      if (k not in set)
        toDelete.append(k)
    }
    foreach(k in toDelete)
      delete cache[k]
    return cache
  }
}


return {
  partial
  pipe
  compose
  kwarg
  KWARG_NON_STRICT
  kwpartial
  curry
  memoize
  isCallable
  once
  before
  after
//  reduce = breakable_reduce
//  BreakValue
  combine
  tryCatch
  mkMemoizedMapSet
}
