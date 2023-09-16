from "modules" import on_module_unload
from "nestdb" import ndbRead, ndbWrite, ndbDelete, ndbExists
import "eventbus"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!So that there is no record in nestdb on shutdown!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!eventbus event app.shutdown is required!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//let {logerr} = require("%sqstd/log.nut")()
let { Watched } = require("frp")
const EVT_NEW_DATA = "GLOBAL_PERMANENT_STATE.newDataAvailable"
let registered = {}


let function readNewData(name){
  if (name in registered) {
    let {key, watched} = registered[name]
    watched(ndbRead(key))
  }
//  else
//    println($"requested data for unknown subscriber '{name}'")
// it spamming too much, but without info about VM logs are useless
}

let function globalWatched(name, ctor=null) {
  assert(name not in registered, $"Global persistent state duplicate registration: {name}")
  let key = ["GLOBAL_PERSIST_STATE", name]
  local val
  if (ndbExists(key)) {
    val = ndbRead(key)
  }
  else {
    val = ctor?()
    ndbWrite(key, val)
  }
  let res = Watched(val)
  registered[name] <- {key, watched=res}
  let function update(value) {
    ndbWrite(key, value)
    res(value)
    eventbus.send_foreign(EVT_NEW_DATA, name)
  }
  res.whiteListMutatorClosure(readNewData)
  res.whiteListMutatorClosure(update)
  return {
    [name] = res,
    [$"{name}Update"] = update
  }
}

eventbus.subscribe(EVT_NEW_DATA, readNewData)


local uniqueKey = null
let function setUniqueNestKey(key) {
  assert(!ndbExists(key), $"key {key} is not unique")
  assert(type(key)=="string", $"setUniqueNestKey failed: {key} is not string")
  uniqueKey = key
  ndbWrite(key, true)
}

local isExiting = false
let mkPersistOnHardReloadKey = @(key) uniqueKey==null
  ? $"PERSIST_ON_RELOAD_DATA__{key}"
  : $"PERSIST_ON_RELOAD_DATA__{uniqueKey}__{key}"

eventbus.subscribe("app.shutdown", @(_) isExiting = true)

let persistOnHardReloadData = persist("PERSIST_ON_RELOAD_DATA", @() {})
let usedKeysForPersist = {}

on_module_unload(function(is_closing) {
  if (isExiting) {
    print("App exiting, not writing PersistentOnReload data")
  }
  else if (is_closing) {
    print("Scripts unloading for hard reload, writing PersistentOnReload data")
    foreach (key, val in persistOnHardReloadData)
      ndbWrite(mkPersistOnHardReloadKey(key), val)
  }
  else {
    print("Scripts unloading for hot reload")
  }
  if (uniqueKey != null) {
    if (ndbExists(uniqueKey))
      ndbDelete(uniqueKey)
    uniqueKey = null
  }
})

let function hardPersistWatched(key, def=null) {
  assert(key not in usedKeysForPersist, @() $"super persistent {key} already registered")
  let ndbKey = mkPersistOnHardReloadKey(key)
  usedKeysForPersist[key] <- null
  local val
  let isInNdb = ndbExists(ndbKey)
  if (key in persistOnHardReloadData) {
    val = persistOnHardReloadData[key]
    if (isInNdb)
      ndbDelete(ndbKey)
  }
  else if (isInNdb) { //on hard reload
    val = ndbRead(ndbKey)
    ndbDelete(ndbKey)
  }
  else {
    val = def
  }
  persistOnHardReloadData[key] <- val
  let res = Watched(val)
  res.subscribe(@(v) persistOnHardReloadData[key] <- v)
  return res
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!So that there is no record in nestdb on shutdown!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!eventbus event app.shutdown is required!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
return {
  globalWatched
  hardPersistWatched
  setUniqueNestKey
}