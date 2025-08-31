require ecs
require ecs.safe
require ecs.common
require app // get_sync_time
require soundEvent
require soundHash
require soundSystem
require sound_utils_net.modules.sound_utils_net_events
require sound_utils.modules.sound_utils_events

let MAX_NET_DELAY = 5.

// -------------------------------------------------------------
def has_net_sound_desc(hash : uint; descs : Object) : bool
  for it in descs
    if sound_hash(it.key) == hash
      return true
  return false

def get_net_sound_desc(hash : uint; descs : Object) : Object const?
  for it in descs
    if sound_hash(it.key) == hash
      return it.value as Object
  for it in descs
    sound_debug("  {it.key}({sound_hash(it.key)})")
  sound_debug("net_sound: hash '{hash}' not found in net_sound.descs")
  return null

def play_impl(hash : uint; var pos : float3; param : float; descs : Object; is_watched : bool; var sound_event_group : SoundEventGroup?)
  if !sound_banks_is_master_preset_loaded()
    return
  let objRef = get_net_sound_desc(hash, descs)
  if objRef != null
    assume obj = *objRef
    let nameRef = is_watched ? (has(obj, "hero") ? obj["hero"] : obj["path"]) : (has(obj, "enemy") ? obj["enemy"] : obj["path"])
    if nameRef != null
      let name = (*nameRef) ?? ""
      let isFar = obj.isFar ?? false
      let delayedAbandon = obj.delayedAbandon ?? false
      if obj.bringToListener ?? false // FMOD may treat event with snapshot as 3d;
        pos = get_listener_pos() // should bring any 2d event to listener to avoid ignoring it by distance when faraway from camera
      var handle = isFar ? play_far(name, "", pos) : play(name, "", pos)
      if has(obj, "var")
        set_var(handle, obj["var"] ?? "", param)
      if has(obj, "groupId") && sound_event_group != null
        if has(obj, "releaseId")
          release_sound(*sound_event_group, sound_hash(obj.releaseId ?? ""))
        add_sound(*sound_event_group, sound_hash(obj.groupId ?? ""), handle)
        if delayedAbandon
          abandon(handle, param)
      else
        abandon(handle, delayedAbandon ? param : 0.)
// -------------------------------------------------------------

[es(tag=sound)]
def play_net_sound_pos_impl_cmd(cmd : CmdPlayNetSoundPos; eid : EntityId; @shared_comp net_sound__descs : Object; var sound_event_group : SoundEventGroup?)
  if get_sync_time() < cmd.time + MAX_NET_DELAY
    play_impl(cmd.hash, cmd.pos, cmd.param, net_sound__descs, has(eid, "hero"), sound_event_group)

[es(tag=sound)]
def play_net_sound_impl_cmd(cmd : CmdPlayNetSound; eid : EntityId; @shared_comp net_sound__descs : Object; var sound_event_group : SoundEventGroup?; transform : float3x4)
  if get_sync_time() < cmd.time + MAX_NET_DELAY
    play_impl(cmd.hash, transform[3], cmd.param, net_sound__descs, has(eid, "hero"), sound_event_group)

[es(tag=sound)]
def play_net_sound_impl_client(cmd : CmdPlaySound; eid : EntityId; @shared_comp net_sound__descs : Object; var sound_event_group : SoundEventGroup?)
  play_impl(cmd.hash, cmd.pos, cmd.param, net_sound__descs, has(eid, "hero"), sound_event_group)

[es(tag=sound)]
def play_net_sound_impl_client_simple(cmd : CmdPlaySoundSimple; eid : EntityId; @shared_comp net_sound__descs : Object; var sound_event_group : SoundEventGroup?; transform : float3x4)
  play_impl(cmd.hash, transform[3], 0., net_sound__descs, has(eid, "hero"), sound_event_group)


// -------------------------------------------------------------

[es(tag=sound)]
def play_scene_net_sound_impl(cmd : CmdPlaySceneNetSound; eid : EntityId; var sound_event_group : SoundEventGroup?; transform : float3x4)
  find_query() <| $ [es(REQUIRE=sceneNetSounds)] (@shared_comp net_sound__descs : Object)
    if has_net_sound_desc(cmd.hash, net_sound__descs)
      play_impl(cmd.hash, transform[3], 0., net_sound__descs, has(eid, "hero"), sound_event_group)
      return true
    return false
