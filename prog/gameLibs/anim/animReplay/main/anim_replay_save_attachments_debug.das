options no_aot
require ecs
require ecs.common


def get_initial_equip_from_slots(var initial_equip : Object; slots : Object)
  for slot in slots
    assume slotObj = *get_ecs_object(slot.value)
    let equipEid = *get_Eid(slotObj, "item")
    if equipEid == INVALID_ENTITY_ID
      continue
    let equipTemplate = getEntityTemplateName(equipEid)
    let equipName = split_template(equipTemplate)[0]
    set(initial_equip, equipName, slot.key)

[es(on_appear, after=anim_replay_gather_animchars_for_save)]
def anim_replay_save_animchar_attachments(evt : Event; var anim_replay__savedAnimchars : Array)
  for animcharChildComp in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    let animcharEid = *get_Eid(animcharData, "eid")
    if get_ecs_object(animcharData, "additionalTemplates") == null
      using() <| $(var additionalTemplates : Object)
        set(animcharData, "additionalTemplates", additionalTemplates)
    assume additionalTemplates = *getRW_ecs_object(animcharData, "additionalTemplates")
    //todo: It looks like game specific
    query(animcharEid) <| $ [es] (human_equipment__slots : Object;
                                  human_weap__weapTemplates : Object)
      using <| $(var equipment_components : Object)
        using <| $(var initialEquip : Object)
          get_initial_equip_from_slots(initialEquip, human_equipment__slots)
          set(equipment_components, "human_equipment__initialEquip", initialEquip)
        set(additionalTemplates, "anim_replay_human_attaches", equipment_components)
      using <| $(var weapon_components : Object)
        set(weapon_components, "human_weap__weapTemplates", human_weap__weapTemplates)
        set(additionalTemplates, "base_human_weap", weapon_components)