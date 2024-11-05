module sound_desc_common shared
require ecs
require ecs.safe
require ecs.common
require soundHash

def find_desc_idx(desc_id : string; descs : Array) : int
  var idx : int = 0
  for it in descs
    if ((it as Object)?.id ?? "") == desc_id
      return idx
    ++idx
  return -1

def find_desc_idx_by_hash(desc_id_hash : uint; descs : Array) : int
  var idx : int = 0
  for it in descs
    if sound_hash((it as Object)?.id ?? "") == desc_id_hash
      return idx
    ++idx
  return -1

def get_desc(idx : int; descs : Array)
  return descs?[idx] ?as Object

def find_desc_by_hash(hash : uint; descs : Object) : Object const?
  for it in descs
    if sound_hash(it.key) == hash
      return it.value as Object
  return null
