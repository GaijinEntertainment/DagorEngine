options no_aot
require ecs
require ecs.safe
require ecs.common
require ecs.ecs_template
require soundSystem
require soundEvent
require sound_utils.modules.sound_control_common
require DagorSystem
require DagorDebug3D
require DagorConsole
require math.color
require math.base
require strings
require RendInst


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawTemplate)]
def sound_debug_draw_template(evt : Event;
                              eid : EntityId;
                              transform : float3x4)
  add_debug_text_mark(transform[3], "{eid}={getEntityTemplateName(eid)} dist={int(distance(transform[3], get_listener_pos()))}", -1, 0., E3DCOLOR(0xFFFFFFFF))


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=msg_sink)]
def sound_debug_draw_visual_labels(evt : Event)
  query_visual_labels() <| $(labels : array<VisualLabel>#)
    for it in labels
      add_debug_text_mark(it.pos, "radius={it.radius} name={it.name}", -1, 0., E3DCOLOR(0xFFFF00FF))


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=humanSound)]
def human_sound_debug_draw(evt : Event;
                           eid : EntityId;
                           sound_tags : Object const?;
                           sound_event_group : SoundEventGroup const?;
                           human_voice_effect__activeEffects : Object const?;
                           @shared_comp human_voice_effect__allOrderedEffects : StringList const?;
                           sound_control__allowSound : bool = true;
                           sound_control__state : int = default_sound_control_state();
                           human_speech__voiceIdx = -1;
                           squad_member__memberIdx = -1;
                           squad_member__squad = INVALID_ENTITY_ID;
                           human_speech__locale = "";
                           team = -1;
                           transform : float3x4)

  if !is_hearable_sound_state(sound_control__state)
    return

  var text = ""
  if !have_sound(sound_control__state)
    text += "-no snd [allow={sound_control__allowSound}; hearable={is_hearable_sound_state(sound_control__state)}; state={sound_control__state}; templ='{getEntityTemplateName(eid)}']- "

  if human_voice_effect__allOrderedEffects != null && human_voice_effect__activeEffects != null

    var numEffects = 0
    for id in *human_voice_effect__allOrderedEffects
      if (*human_voice_effect__activeEffects)[string(id)] ?? false
        if numEffects == 0
          text += "voice effects: [{id}"
        else
          text += ", {id}"
        ++numEffects
    if numEffects > 0
      text += "] "

    for itFx in *human_voice_effect__activeEffects
      var exist = false
      for id in *human_voice_effect__allOrderedEffects
        exist ||= itFx.key == id
      if !exist
        text += "-'{itFx.key}' is MISSING in human_voice_effect__allOrderedEffects-"

  if sound_tags != null && (is_watched_sound_state(sound_control__state) || length(*sound_tags) > 0)
    text += "sound tags:{*sound_tags} "

  if sound_event_group != null && (is_watched_sound_state(sound_control__state) || get_num_sounds(*sound_event_group) > 0)
    text += "sound group:{get_num_sounds(*sound_event_group)} "

  if !empty(human_speech__locale)
    text += "loc={human_speech__locale} "

  if human_speech__voiceIdx >= 0
    text += "voice=#{human_speech__voiceIdx} "
    text += "member=#{squad_member__memberIdx} "
    text += "team=#{team} "
    text += "squad=#{squad_member__squad} "

  if !empty(text)
    let color = is_watched_sound_state(sound_control__state) ? E3DCOLOR(0xFF0000FF) : E3DCOLOR(0xFF000000)
    let pos = transform[3] + float3(0., 0.5, 0.)
    add_debug_text_mark(pos, text, -1, 0., color)



[console_cmd(name="sndsys.preset_enable")]
def sound_debug_console_sndsys_preset_enable(name : string; enable : bool)
  sound_banks_enable_preset(name, enable)


[console_cmd(name="snd.play")]
def sound_debug_console_sound_play(path : string; duration = 5.)
  var handle = soundEvent::play("", path, get_listener_pos())
  if is_valid_handle_value(handle)
    sound_debug("snd.play '{path}' is3d={is_3d(handle)}; maxRange={get_max_distance(handle)}; isOneshot={is_oneshot(handle)}")
  if is_oneshot(handle)
    abandon(handle)
  else
    abandon(handle, duration)


[console_cmd(name="snd.play_pos")]
def sound_debug_console_sound_play_pos(path : string; x, y, z : float; duration = 5.)
  var handle = soundEvent::play("", path, float3(x, y, z))
  if is_valid_handle_value(handle)
    sound_debug("snd.play '{path}' is3d={is_3d(handle)}; maxRange={get_max_distance(handle)}; isOneshot={is_oneshot(handle)}")
  if is_oneshot(handle)
    abandon(handle)
  else
    abandon(handle, duration)


[console_cmd(name="snd.enum_events")]
def sound_debug_console_enum_events()
  sound_debug_enum_events()


[ecs_template]
struct sound_debug_draw_ri
  sound_debug_draw_ri__name : string


[console_cmd(name="snd.debug_draw_ri")]
def sound_debug_draw_ri_console_cmd(name : string)
  if !empty(name)
    var entEid : EntityId
    query() <| $ [es] (eid : EntityId; sound_debug_draw_ri__name : string)
      if sound_debug_draw_ri__name == name
        entEid = eid
    if !entEid
      let resIdx = get_rigen_extra_res_idx(name)
      if resIdx < 0
        logerr("no such res idx for '{name}'")
        return
      createEntity("sound_debug_draw_ri") <| $(var init : ComponentsInitializer)
        set(init, "sound_debug_draw_ri__name", name)
    else
      destroyEntity(entEid)


[es(tag=sound, on_event=EventSoundDrawDebug)]
def sound_debug_draw_ri(evt : Event;
                        sound_debug_draw_ri__name : string)

  let resIdx = get_rigen_extra_res_idx(sound_debug_draw_ri__name)
  get_ri_gen_extra_instances(resIdx) <| $(handles : array<riex_handle_t>#)
    for handle in handles
      let desc = RendInstDesc(handle)
      let tm = getRIGenMatrix(desc)
      add_debug_text_mark(tm[3], "ri='{sound_debug_draw_ri__name}'  dist={int(distance(get_listener_pos(), tm[3]))}", -1, 0., E3DCOLOR(0xffffffff))
