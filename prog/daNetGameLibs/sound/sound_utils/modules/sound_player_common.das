module sound_player_common shared

require ecs
require ecs.safe
require ecs.common
require soundHash
require soundSystem
require soundEvent
require DagorSystem
require strings


def raise_sound_tag(tag : string; var tags : Object&)
  if !empty(tag)
    set(tags, tag, true)

def clear_sound_tag(tag : string; var tags : Object&)
  if !empty(tag)
    erase(tags, tag)

def set_sound_tag(tag : string; enable : bool; var tags : Object&)
  if enable
    raise_sound_tag(tag, tags)
  else
    clear_sound_tag(tag, tags)

def read_sound_tag(tag : string; tags : Object) : bool
  return tags[tag] ?? false


def get_desc(descs : Object; desc_str : string) : Object const?
  return !empty(desc_str) ? (descs[desc_str] ?as Object) : null


def test_tags(opt : Object; all_tags : Object) : bool
  for it in opt
    let tag = get_bool(it.value)
    if tag != null && *tag != read_sound_tag(it.key, all_tags)
      return false
  return true


def get_option_path_string(obj : Object const; is_watched : bool; var result : string&; watched_key = "hero"; unwatched_key = "enemy") : bool
  if is_watched && has(obj, watched_key)
    result = obj[watched_key] ?? "error"
    return true
  elif !is_watched && has(obj, unwatched_key)
    result = obj[unwatched_key] ?? "error"
    return true
  elif has(obj, "path")
    result = obj.path ?? "error"
    return true
  return false


def get_option_path(path : Object; path_id : int; is_watched : bool; var result : string&; watched_key = "hero"; unwatched_key = "enemy") : bool
  if !has(path, "options")
    if path_id == 0
      return get_option_path_string(path, is_watched, result, watched_key, unwatched_key)
    return false
  let opts = *(path["options"]) as Array
  if opts != null
    if path_id >= 0 && path_id < length(*opts)
      let opt = (*opts)[path_id] as Object
      if opt != null
        return get_option_path_string(*opt, is_watched, result, watched_key, unwatched_key)
  return false


def get_option_path(path : Object; tags : Object; is_watched : bool; var result : string&; watched_key = "hero"; unwatched_key = "enemy") : bool
  if !has(path, "options")
    if test_tags(path, tags)
      return get_option_path_string(path, is_watched, result, watched_key, unwatched_key)
    return false
  let opts = *(path["options"]) as Array
  if opts != null
    for it in *opts
      let opt = it as Object
      if opt != null && test_tags(*opt, tags)
        return get_option_path_string(*opt, is_watched, result, watched_key, unwatched_key)
  return false


def get_num_option_paths(path : Object) : int
  if !has(path, "options")
    return 1
  let opts = *(path["options"]) as Array
  if opts != null
    return length(*opts)
  return 0


def get_option_path(path : Object const?; tags : Object; is_watched : bool; var result : string&) : bool
  return path != null && get_option_path(*path, tags, is_watched, result)


def get_option_object(opts : Object; tags : Object) : Object const?
  for it in opts
    let opt = it.value as Object
    if opt != null && test_tags(*opt, tags)
      return opt
  return null


def get_path_by_name(paths : Object; path_name : string)
  let path = paths[path_name] ?as Object
  return path != null ? path : get_ecs_object(paths, "default")

def get_path_by_desc_nullable(desc : Object; paths : Object)
  return get_path_by_name(paths, desc.path ?? "")

def is_valid(obj : Object)
  return has(obj, "name") || has(obj, "hero") || has(obj, "enemy") || has(obj, "path")

def is_valid(obj : Object; all_tags : Object)
  return is_valid(obj) && test_tags(obj, all_tags)

def play_name_path_impl(name : string; path : string; id : uint; pos : float3; var group : SoundEventGroup&; delay = 0.)
  let handle = soundEvent::delayed_play(name, path, pos, delay)
  reject_sound(group, id)
  add_sound(group, id, handle)
  return handle

def play_name_path_impl(name : string; path : string; pos : float3; is_abandon : bool; delay = 0.)
  var handle = soundEvent::delayed_play(name, path, pos, delay)
  if is_abandon
    abandon(handle)
  return handle

def play_name_path(name : string;
                   path : string;
                   id : uint;
                   pos : float3;
                   var group : SoundEventGroup&;
                   delay = 0.)
  reject_sound(group, id)
  return play_name_path_impl(name, path, id, pos, group, delay)

def play_name_path(name : string;
                   path : string;
                   pos : float3;
                   is_abandon : bool = true;
                   delay = 0.)
  if !empty(name) || !empty(path)
    return play_name_path_impl(name, path, pos, is_abandon, delay)
  return invalid_sound_event_handle()


def get_name(desc : Object; is_watched_sound : bool; def_ = "")
  return is_watched_sound ? (desc.hero ?? desc.name ?? def_) : (desc.enemy ?? desc.name ?? def_)


def play_desc_path(desc : Object;
                   path : string;
                   is_watched_sound : bool;
                   id : uint;
                   pos : float3;
                   var group : SoundEventGroup&)
  reject_sound(group, id)
  if !empty(path) && is_valid(desc)
    let name = get_name(desc, is_watched_sound)
    if !empty(name)
      return play_name_path_impl(name, path, id, pos, group, desc.delay ?? 0.)
  return invalid_sound_event_handle()

def play_desc_path(desc : Object;
                   path : string;
                   is_watched_sound : bool;
                   pos : float3;
                   is_abandon : bool = true)
  if !empty(path) && is_valid(desc)
    let name = get_name(desc, is_watched_sound)
    if !empty(name)
      return play_name_path_impl(name, path, pos, is_abandon, desc.delay ?? 0.)
  return invalid_sound_event_handle()


def should_apply_watched_prefix(path : string) : bool
  return path |> starts_with("/")

def private apply_watched_prefix_name_path(var path : string&; is_watched_sound : bool) : string
  if should_apply_watched_prefix(path)
    let name = path
    path = is_watched_sound ? "player" : "enemy"
    return name
  return ""

def apply_watched_prefix(path : string; is_watched_sound : bool) : string
  if should_apply_watched_prefix(path)
    return is_watched_sound ? "player{path}" : "enemy{path}"
  return path


def make_final_path_str(paths, tags : Object; is_watched : bool; name = "") : string
  return build_string() <| $(var str)
    var path = ""
    if get_option_path(paths, tags, is_watched, path)
      if should_apply_watched_prefix(path)
        str |> write(is_watched ? "player" : "enemy")
      str |> write(path)
      if !empty(name)
        str |> write("/")
        str |> write(name)


def make_final_path_str(paths : Object; path_id : int; is_watched : bool; name = "") : string
  return build_string() <| $(var str)
    var path = ""
    if get_option_path(paths, path_id, is_watched, path)
      if should_apply_watched_prefix(path)
        str |> write(is_watched ? "player" : "enemy")
      str |> write(path)
      if !empty(name)
        str |> write("/")
        str |> write(name)


def play_path(path_obj : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              is_abandon : bool = true;
              delay = 0.;
              prefix = "")
  var path = ""
  if get_option_path(path_obj, tags, is_watched_sound, path)
    let name = apply_watched_prefix_name_path(path, is_watched_sound)
    path = "{prefix}{path}"
    return play_name_path(name, path, pos, is_abandon, delay)
  return invalid_sound_event_handle()


def play_path(path_obj : Object;
              name : string;
              is_watched_sound : bool;
              pos : float3;
              is_abandon : bool = true;
              delay = 0.)
  var path = ""
  if get_option_path_string(path_obj, is_watched_sound, path)
    return play_name_path(name, path, pos, is_abandon, delay)
  return invalid_sound_event_handle()


def play_path(path_obj : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              id : uint;
              var group : SoundEventGroup&;
              delay = 0.;
              prefix = "")
  reject_sound(group, id)
  var path = ""
  if get_option_path(path_obj, tags, is_watched_sound, path)
    let name = apply_watched_prefix_name_path(path, is_watched_sound)
    path = "{prefix}{path}"
    return play_name_path(name, path, id, pos, group, delay)
  return invalid_sound_event_handle()


def play_path(path_id : string;
              paths : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              is_abandon : bool = true;
              prefix = "")
  let path = get_desc(paths, path_id)
  if path != null
    return play_path(*path, tags, is_watched_sound, pos, is_abandon, 0., prefix)
  sound_debug("no path '{path_id}' exists")
  return invalid_sound_event_handle()


def play_path(path_id : string;
              paths : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              id : uint;
              var group : SoundEventGroup&;
              prefix = "")
  reject_sound(group, id)
  let path = get_desc(paths, path_id)
  if path != null
    return play_path(*path, tags, is_watched_sound, pos, id, group, 0., prefix)
  sound_debug("no path '{path_id}' exists")
  return invalid_sound_event_handle()


def play(desc_id : string;
         path_obj : Object;
         descs : Object;
         tags : Object;
         is_watched_sound : bool;
         pos : float3;
         is_abandon : bool = true)
  let desc = get_desc(descs, desc_id)
  if desc != null
    if is_valid(*desc, tags)
      var path = ""
      if get_option_path(path_obj, tags, is_watched_sound, path)
        let name = get_name(*desc, is_watched_sound, desc_id)
        if !empty(name)
          return play_name_path(name, apply_watched_prefix(path, is_watched_sound), pos, is_abandon, desc?.delay ?? 0.)
    return invalid_sound_event_handle()
  sound_debug("no desc '{desc_id}' exists")
  return invalid_sound_event_handle()


def play(desc_id : string;
         path_obj : Object;
         descs : Object;
         tags : Object;
         is_watched_sound : bool;
         pos : float3;
         id : uint;
         var group : SoundEventGroup&)
  reject_sound(group, id)
  let desc = get_desc(descs, desc_id)
  if desc != null
    if is_valid(*desc, tags)
      var path = ""
      if get_option_path(path_obj, tags, is_watched_sound, path)
        let name = get_name(*desc, is_watched_sound, desc_id)
        if !empty(name)
          return play_name_path(name, apply_watched_prefix(path, is_watched_sound), id, pos, group, desc?.delay ?? 0.)
    return invalid_sound_event_handle()
  sound_debug("no desc '{desc_id}' exists")
  return invalid_sound_event_handle()


def play_desc(desc_obj : Object;
              path_obj : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              id : uint;
              var group : SoundEventGroup&)
  reject_sound(group, id)
  if is_valid(desc_obj, tags)
    var path = ""
    if get_option_path(path_obj, tags, is_watched_sound, path)
      let name = get_name(desc_obj, is_watched_sound)
      if !empty(name)
        return play_name_path(name, apply_watched_prefix(path, is_watched_sound), id, pos, group, desc_obj.delay ?? 0.)
  return invalid_sound_event_handle()


def play_desc(desc_obj : Object;
              path_obj : Object;
              tags : Object;
              is_watched_sound : bool;
              pos : float3;
              is_abandon : bool = true)
  if is_valid(desc_obj, tags)
    var path = ""
    if get_option_path(path_obj, tags, is_watched_sound, path)
      let name = get_name(desc_obj, is_watched_sound)
      if !empty(name)
        return play_name_path(name, apply_watched_prefix(path, is_watched_sound), pos, is_abandon, desc_obj.delay ?? 0.)
  return invalid_sound_event_handle()


def play_parameterized(var sound : SoundEvent&; name : string; param : float; var max_param : float&; param_name : string; pos : float3;
                       paths, tags : Object; is_watched_sound : bool;
                       cur_time : float; var play_time : float&; max_time : float = 0.5; abandon_time : float = 4.)
  if param > 0.
    if !is_playing(sound) || cur_time > play_time + max_time
      abandon(sound)
      sound |> reset(play_path(name, paths, tags, is_watched_sound, pos, false))
      play_time = cur_time
      max_param = param
    max_param = max(max_param, param)
    set_var(sound, param_name, max_param)
  elif play_time >= 0. && cur_time > play_time + abandon_time
    abandon(sound)
    play_time = -1.
  set_pos(sound, pos)


def play_parameterized(var sound : SoundEvent&; param : float; var max_param : float&; param_name : string; pos : float3;
                       path, tags : Object; is_watched_sound : bool;
                       cur_time : float; var play_time : float&; max_time : float = 0.5; abandon_time : float = 4.)
  if param > 0.
    if !is_playing(sound) || cur_time > play_time + max_time
      abandon(sound)
      sound |> reset(play_path(path, tags, is_watched_sound, pos, false))
      play_time = cur_time
      max_param = param
    max_param = max(max_param, param)
    set_var(sound, param_name, max_param)
  elif play_time >= 0. && cur_time > play_time + abandon_time
    abandon(sound)
    play_time = -1.
  set_pos(sound, pos)


def private play_or_release_impl(var event : SoundEvent&; name : string; pos : float3; should_play : bool; is_abandon : bool)
  if event.enabled != should_play
    event.enabled = should_play
    event.abandonOnReset = is_abandon
    if should_play
      event |> reset(play(name, pos))
    elif is_abandon
      event |> abandon()
    else
      event |> release()
  elif should_play
    event |> set_pos(pos)
  return event.enabled

def private play_or_release_impl(var event : SoundEvent&; name : string; should_play : bool; is_abandon : bool)
  if event.enabled != should_play
    event.enabled = should_play
    event.abandonOnReset = is_abandon
    if should_play
      event |> reset(play(name))
    elif is_abandon
      event |> abandon()
    else
      event |> release()
  return event.enabled


// for use from update()
// event.enabled is a user state(tag)
// event.enabled SHOULD NOT depend from sound state

def play_or_release(var event : SoundEvent&; name : string; pos : float3; should_play : bool)
  return play_or_release_impl(event, name, pos, should_play, false)

def play_or_release(var event : SoundEvent&; name : string; should_play : bool)
  return play_or_release_impl(event, name, should_play, false)

def play_or_abandon(var event : SoundEvent&; name : string; pos : float3; should_play : bool)
  return play_or_release_impl(event, name, pos, should_play, true)

def play_or_abandon(var event : SoundEvent&; name : string; should_play : bool)
  return play_or_release_impl(event, name, should_play, true)

def release_play_or(var event : SoundEvent&)
  if is_valid_handle_value(event)
    event |> release()
  event.enabled = false

def abandon_play_or(var event : SoundEvent&)
  if is_valid_handle_value(event)
    event |> abandon()
  event.enabled = false
