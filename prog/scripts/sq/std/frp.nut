from "frp" import Computed, Watched, FRP_INITIAL, FRP_DONT_CHECK_NESTED, set_nested_observable_debug, make_all_observables_immutable

let function watchedTable2TableOfWatched(state, fieldsList = null) {
  assert(state instanceof Watched, "state has to be Watched")
  let list = fieldsList ?? state.value
  assert(type(list) == "table", "fieldsList should be provided as table")
  return list.map(@(_, key) Computed(@() state.value[key]))
}

/*
  function receive tick 'Event Stream' observable
  and returns function mkLatestStream(defValue, name=null)
  which return observable and functions to change observable value on tick:
    setValue, mutateValue, modifyValue, setKeyValue and deleteKey
    setValue will just set provided value
    modify(func) requires function as argument will call function with latest value and use returned value as newvalue
    setKeyValue(key, value) implies that observable is a table and it can modify it's keys with provided value
    deleteKey(key) implies that observable is a table and it's keys can be deleted
  that will update it's value only on tick observable change
*/

let function mkLatestByTriggerStream(triggerObservable) {
  return function mkLatestStream(defValue = null, name = null){
    let isTable = type(defValue) == "table"
    local next_value = defValue
    let res = Watched(defValue)
    let function updateFunc(...) {
      let oldValue = res.value
      triggerObservable.unsubscribe(updateFunc)
      res.set(next_value)
      if (next_value == oldValue){
        let resType = type(next_value)
        if (resType == "table" || resType == "array"){
          res.trigger()
        }
      }
    }
    res.whiteListMutatorClosure(updateFunc)
    let function deleteKey(k){
      if (k in next_value)
        delete next_value[k]
      triggerObservable.subscribe(updateFunc)
    }
    let function setKeyVal(k, v){
      next_value[k] <- v
      triggerObservable.subscribe(updateFunc)
    }
    let function setValue(val){
      next_value = val
      triggerObservable.subscribe(updateFunc)
    }
    let function modify(val){
      next_value = val(next_value)
      triggerObservable.subscribe(updateFunc)
    }
    if (name==null) {
      return {state = res, setValue, modify}.__update(isTable ? {setKeyVal, deleteKey } : {})
    }
    return {
      [name] = res, [$"{name}SetValue"] = setValue, [$"{name}Modify"] = modify
    }.__update(isTable ? {[$"{name}SetKeyVal"] = setKeyVal, [$"{name}DeleteKey"] = deleteKey} : {})
  }
}

/*
  function receives triggerObservable and create function that will
  returns new observable of 'Set' {key:key} that will update only at tick change
  and 3 functions:
    - getWatched(key) function will get Watched value that will be update on eid components change
    - updateKey(key, value) - will update Watched value and add new key to Set if it is needed
    - destroyKey(eid) - will remove key from Set
    if mkCombined provided - function will return 'state' observable {key: value}
    this can be needed for some Computed streams
    it is widely  used for osbervables with eid as key and properties as value
    to make them work faster as they update much more often than needed for ui
*/
let TO_DELETE = persist("TO_DELETE", @() freeze({}))
const MK_COMBINED_STATE = true

let function mkTriggerableLatestWatchedSetAndStorage(triggerableObservable) {
  return function mkLatestWatchedSetAndStorage(key=null, mkCombined=false) {
    let observableEidsSet = mkCombined ? null : Watched({})
    let state = mkCombined ? Watched({}) : null
    let storage = {}
    let eidToUpdate = {}
    local update
    update = mkCombined == MK_COMBINED_STATE
      ? function (_){
        state?.mutate(function(v) {
          foreach(eid, val in eidToUpdate){
            if (val == TO_DELETE) {
              if (eid in storage)
                delete storage[eid]
              if (eid in v)
                delete v[eid]
            }
            else
              v[eid] <- val
          }
        })
        eidToUpdate.clear()
        triggerableObservable.unsubscribe(update)
      }
      : function (_){
        observableEidsSet?.mutate(function(v) {
          foreach(eid, val in eidToUpdate){
            if (val == TO_DELETE) {
              if (eid in storage)
                delete storage[eid]
              if (eid in v)
                delete v[eid]
            }
            else
              v[eid] <- eid
          }
        })
        eidToUpdate.clear()
        triggerableObservable.unsubscribe(update)
      }
    let destroyEid = function (eid) {
      if (eid not in storage)
        return
      eidToUpdate[eid] <- TO_DELETE
      triggerableObservable.subscribe(update)
    }
    let updateEidProps = (mkCombined == MK_COMBINED_STATE)
      ? function ( eid, val ) {
          if (eid not in storage || eidToUpdate?[eid] == TO_DELETE) {
            storage[eid] <- Watched(val)
          }
          else {
            storage[eid].update(val)
          }
          eidToUpdate[eid] <- val
          triggerableObservable.subscribe(update)
        }
      : function (eid, val) {
          if (eid not in storage || eidToUpdate?[eid] == TO_DELETE) {
            storage[eid] <- Watched(val)
            eidToUpdate[eid] <- eid
            triggerableObservable.subscribe(update)
          }
          else
            storage[eid].update(val)
        }
    let function getWatchedByEid(eid){
      return storage?[eid]
    }
    if (mkCombined == MK_COMBINED_STATE)
      state?.whiteListMutatorClosure(update)
    else
      observableEidsSet?.whiteListMutatorClosure(update)

    if (key==null) {
      return {
        getWatched = getWatchedByEid
        updateEid = updateEidProps
        destroyEid
      }.__update(mkCombined==MK_COMBINED_STATE ? {state} : {set = observableEidsSet})
    }
    else {
      assert(type(key)=="string", @() $"key should be null or string, but got {type(key)}")
      return {
        [$"{key}GetWatched"] = getWatchedByEid,
        [$"{key}UpdateEid"] = updateEidProps,
        [$"{key}DestroyEid"] = destroyEid,
      }.__update(mkCombined==MK_COMBINED_STATE ? {[$"{key}State"] = state} : {[$"{key}Set"] = observableEidsSet})
    }
  }
}
return {
  mkLatestByTriggerStream
  mkTriggerableLatestWatchedSetAndStorage
  watchedTable2TableOfWatched
  MK_COMBINED_STATE
  Computed
  Watched
  FRP_INITIAL
  FRP_DONT_CHECK_NESTED
  set_nested_observable_debug
  make_all_observables_immutable
}