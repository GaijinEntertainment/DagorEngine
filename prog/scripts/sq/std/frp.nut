from "frp" import Computed, Watched

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

let KeyAndVal = class{
  val = null
  key = null
  constructor(k, v){
    this.val = v
    this.key = k
  }
}
let Modifier = class{
  mod = null
  constructor(v){
    this.mod = v
  }
}
let DeleteKey = class{
  key = null
  constructor(k){
    this.key = k
  }
}

let function mkLatestByTriggerStream(triggerObservable) {
  return function mkLatestStream(defValue = null, name = null){
    let isTable = type(defValue) == "table"
    local valsToSet = []
    let res = Watched(defValue)
    let function updateFunc(_) {
      local newResVal = res.value
      foreach (newVal in valsToSet) {
        let klass = newVal?.getclass()
        if (klass == KeyAndVal) {
          newResVal[newVal.key] <- newVal.val
        }
        else if (klass == DeleteKey){
          let key = newVal.key
          if (key in newResVal)
            delete newResVal[key]
        }
        else if (klass == Modifier) {
          newResVal = newVal.mod(newResVal)
        }
        else
          newResVal = newVal
      }
      triggerObservable.unsubscribe(updateFunc)
      valsToSet.clear()
      let resType = type(newResVal)
      let needToTrigger = newResVal == res.value && (resType == "table" || resType == "array")
      res(newResVal)
      if (needToTrigger)
        res.trigger()
    }
    res.whiteListMutatorClosure(updateFunc)
    let function deleteKey(k){
      valsToSet.append(DeleteKey(k))
      triggerObservable.subscribe(updateFunc)
    }
    let function setKeyVal(k, v){
      valsToSet.append(KeyAndVal(k, v))
      triggerObservable.subscribe(updateFunc)
    }
    let function setValue(val){
      valsToSet.clear()
      valsToSet.append(val)
      triggerObservable.subscribe(updateFunc)
    }
    let function modify(val){
      valsToSet.append(Modifier(val))
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
        state.mutate(function(v) {
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
        observableEidsSet.mutate(function(v) {
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
      state.whiteListMutatorClosure(update)
    else
      observableEidsSet.whiteListMutatorClosure(update)

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
}