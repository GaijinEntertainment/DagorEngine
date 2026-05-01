module anim_replay_create_animchars shared
options no_aot
options no_global_variables = false

require ecs
require DagorSystem
require %gameLibs/anim/animReplay/imports/anim_replay_events

var private animchars_to_create = 0

def private create_replay_animchar(animchar_data : Object)
  var animcharTemplate = "replay_animchar"
  let reduceCounter <- @ <| (eid : EntityId)
    animchars_to_create--
    if animchars_to_create == 0
      broadcastEvent(AnimReplayAnimcharsCreated())
  assume additionalTemplates = *get_ecs_object(animchar_data, "additionalTemplates")
  for templ in additionalTemplates
    animcharTemplate = "{animcharTemplate}+{templ.key}"
  let animcharEid = createEntity(animcharTemplate, reduceCounter) <| $(var init : ComponentsInitializer)
    set(init, "animchar__res", *get_ecs_string(animchar_data, "animcharRes"))
    for templ in additionalTemplates
      for component in *get_ecs_object(templ.value)
        set(init, component.key, component.value)
  return animcharEid

def schedule_animchars_creation(var animchars_data : Array)
  if animchars_to_create != 0
    logerr("Some animchars are already scheduled for creation ({animchars_to_create})")
  animchars_to_create = length(animchars_data)
  for animchar in animchars_data
    assume animcharData = *getRW_ecs_object(animchar)
    set(animcharData, "eid", create_replay_animchar(animcharData))
