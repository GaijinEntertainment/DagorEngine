module sound_desc_common shared
require ecs
require ecs.safe
require ecs.common
require soundHash


let WATCHED_SOUND_KEY = "watched"
let OTHER_SOUND_KEY = "other"

def get_watched_sound_key(is_watched = true)
  return is_watched ? WATCHED_SOUND_KEY : OTHER_SOUND_KEY


let WATCHED_SOUND_EVENT_FOLDER = "player"
let OTHER_SOUND_EVENT_FOLDER = "enemy"

def get_watched_sound_event_folder(is_watched = true)
  return is_watched ? WATCHED_SOUND_EVENT_FOLDER : OTHER_SOUND_EVENT_FOLDER


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

def get_desc(idx : int; descs : Array) : Object const?
  return descs?[idx] ?as Object

def find_desc_by_hash(hash : uint; descs : Object) : Object const?
  for it in descs
    if sound_hash(it.key) == hash
      return it.value as Object
  return null
