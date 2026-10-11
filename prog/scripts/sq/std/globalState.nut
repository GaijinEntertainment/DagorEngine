import "dagor.math" as math
from "modules" import on_module_unload
from "nestdb" import ndbRead, ndbWrite, ndbDelete, ndbExists
from "eventbus" import eventbus_send_foreign, eventbus_subscribe
from "dagor.debug" import logerr
from "dagor.memtrace" import is_quirrel_object_larger_than, set_huge_alloc_threshold
from "dagor.time" import get_time_msec
from "frp" import this_subscriber_call_may_take_up_to_usec, get_slow_subscriber_threshold_usec
from "iostream" import blob
from "types" import String

let serialazableClasses = {}.__update(math)

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!So that there is no record in nestdb on shutdown!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!eventbus event app.shutdown is required!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

let { Watched } = require("frp.nut")
const EVT_NEW_DATA = "GLOBAL_PERMANENT_STATE.newDataAvailable"
let registered = {}


function readNewData(name){
  if (name in registered) {
    let {key, watched} = registered[name]
    watched.set(ndbRead(key))
  }
}

function globalWatched(name, ctor=null): table {
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
  function update(value) {
    ndbWrite(key, value)
    res.set(value)
    eventbus_send_foreign(EVT_NEW_DATA, name)
  }
  res.whiteListMutatorClosure(readNewData)
  res.whiteListMutatorClosure(update)
  return {
    [name] = res,
    [$"{name}Update"] = update
  }
}

eventbus_subscribe(EVT_NEW_DATA, readNewData)


let usedKeysForPersist = {}
local uniqueKey = null
function setUniqueNestKey(key) {
  if (uniqueKey != null) { //constraint: two UI trees share one VM
    assert(uniqueKey == key, @() $"setUniqueNestKey({key}) after {uniqueKey}")
    return
  }
  assert(!ndbExists(key), $"key {key} is not unique")
  assert(key instanceof String, $"setUniqueNestKey failed: {key} is not string")
  assert(usedKeysForPersist.len() == 0, @() $"setUniqueNestKey({key}) after the storage of {", ".join(usedKeysForPersist.keys())}")
  uniqueKey = key
  ndbWrite(key, true)
}

local isExiting = false
let mkPersistOnHardReloadKey = @(key) uniqueKey==null
  ? $"PERSIST_ON_RELOAD_DATA__{key}"
  : $"PERSIST_ON_RELOAD_DATA__{uniqueKey}__{key}"

eventbus_subscribe("app.shutdown", @(_) isExiting = true)

local isPlainDataCheckOn = false
function setHardPersistPlainDataCheck(is_on) {
  isPlainDataCheckOn = is_on
}

const plainTypes = { ["null"] = true, integer = true, float = true, bool = true, string = true }
function findNotPlainPath(v, path) {
  let t = type(v)
  if (t != "table" && t != "array")
    return t in plainTypes || v instanceof blob ? null : $"{path} ({t})"
  foreach (k, item in v) {
    let res = findNotPlainPath(k, $"{path} key") ?? findNotPlainPath(item, $"{path}.{k}")
    if (res != null)
      return res
  }
  return null
}

let persistOnHardReloadData = persist("PERSIST_ON_RELOAD_DATA", @() {})
let _big_datas = persist("_big_datas", @() {})

const size_threshold_to_store_as_big_data = 100 << 10
on_module_unload(function(is_closing) {
  if (isExiting) {
    print("App exiting, not writing PersistentOnReload data")
  }
  else if (is_closing) {
    print("Scripts unloading for hard reload, writing PersistentOnReload data")
    foreach (key, val in persistOnHardReloadData) {
      let prevAllocThreshold = set_huge_alloc_threshold(66560 << 10)
      try {
        local is_big_data = false
        local info = ""
        if (key in _big_datas) {
          info = "manually set to store as big data"
          is_big_data = true
        }
        else if (is_quirrel_object_larger_than(val, size_threshold_to_store_as_big_data)) {
          is_big_data = true
          info = $"store as big data as size is bigger than {size_threshold_to_store_as_big_data}"
        }
        if (is_big_data) {
          println($"store compressed version in hard reload storage for: {key}, {info}")
        }

        local data
        if (is_big_data) {
          data = blob()
          data.writeobject(val, serialazableClasses)
        }
        else {
          let notPlainPath = isPlainDataCheckOn ? findNotPlainPath(val, key) : null
          if (notPlainPath != null)
            logerr($"ERROR: on hard reload storage save: {notPlainPath} is not plain data")
          data = val
        }

        ndbWrite(mkPersistOnHardReloadKey(key), {data, is_big_data = !!is_big_data })
      }
      catch(e){
        println($"ERROR: on hard reload storage {key} = {type(val)}")
        println(e)
        logerr($"ERROR: on hard reload storage save: {key}")
      }
      set_huge_alloc_threshold(prevAllocThreshold)
    }
  }
  else {
    println("Scripts unloading for hot reload")
  }
  if (uniqueKey != null) {
    if (ndbExists(uniqueKey))
      ndbDelete(uniqueKey)
    uniqueKey = null
  }
})

function loadHardPersistValue(key, def, big_immutable_data) {
  assert(key not in usedKeysForPersist, @() $"super persistent {key} already registered")
  if (big_immutable_data != null)
    _big_datas[key] <- big_immutable_data
  let ndbKey = mkPersistOnHardReloadKey(key)
  usedKeysForPersist[key] <- null
  local val = def
  let isInNdb = ndbExists(ndbKey)
  if (key in persistOnHardReloadData) {
    val = persistOnHardReloadData[key]
    if (isInNdb)
      ndbDelete(ndbKey)
  }
  else if (isInNdb) { //on hard reload
    let prevAllocThreshold = set_huge_alloc_threshold(66560 << 10)
    let stored = ndbRead(ndbKey)
    try {
      let {is_big_data=null, data=null} = stored
      if (is_big_data) {
        val = data?.readobject(serialazableClasses)
        if (big_immutable_data) {
          let t3 = get_time_msec()
          deduplicate_object(val)
          let t4 = get_time_msec()
          print($">>> Shrinking {key}, time = {t4-t3} ms")
        }
      }
      else
        val = data
    }
    catch(e) {
      println(e)
      logerr($"ERROR: on hard reload storage load: {key}")
    }
    set_huge_alloc_threshold(prevAllocThreshold)
    ndbDelete(ndbKey)
  }
  persistOnHardReloadData[key] <- val
  return val
}

function hardPersistWatched(key, def=null, big_immutable_data = null) {
  let res = Watched(loadHardPersistValue(key, def, big_immutable_data))
  if (!big_immutable_data)
    res.subscribe(@(v) persistOnHardReloadData[key] <- v)
  else
    res.subscribe(function(v) {
      this_subscriber_call_may_take_up_to_usec(10 * get_slow_subscriber_threshold_usec())
      persistOnHardReloadData[key] <- v
    })
  return res
}

function hardPersist(key: string, def: table|array|null = null, big_immutable_data = null): table|array {
  return loadHardPersistValue(key, def ?? {}, big_immutable_data)
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!So that there is no record in nestdb on shutdown!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!eventbus event app.shutdown is required!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
return freeze({
  globalWatched
  hardPersistWatched
  hardPersist
  setUniqueNestKey
  setHardPersistPlainDataCheck
})
