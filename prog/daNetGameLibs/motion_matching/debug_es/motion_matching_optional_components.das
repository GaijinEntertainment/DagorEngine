require ecs

[es(tag=(render, dev), on_appear)]
def mm_add_optional_subtemplate(evt : Event; eid : EntityId; motion_matching__optionalSubtemplate : string)
  var optionalSubtemplatesManager = getSingletonEntity("motion_matching_optional_subtemplates_manager")
  if !optionalSubtemplatesManager
    optionalSubtemplatesManager = createEntitySync("motion_matching_optional_subtemplates_manager")
  query(optionalSubtemplatesManager) <| $ [es] (motion_matching__optionalSubtemplatesEnabled : bool)
    if motion_matching__optionalSubtemplatesEnabled
      let newTemplate : string = add_sub_template_name(eid, motion_matching__optionalSubtemplate)
      if !empty(newTemplate)
        reCreateEntityFrom(eid, newTemplate)

[es(tag=(render, dev), track=motion_matching__optionalSubtemplatesEnabled)]
def mm_track_optional_subtemplates(evt : Event; motion_matching__optionalSubtemplatesEnabled : bool)
  query() <| $ [es] (eid : EntityId; motion_matching__optionalSubtemplate : string)
    var newTemplate : string
    if motion_matching__optionalSubtemplatesEnabled
      newTemplate = add_sub_template_name(eid, motion_matching__optionalSubtemplate)
    else
      newTemplate = remove_sub_template_name(eid, motion_matching__optionalSubtemplate)
    if !empty(newTemplate)
      reCreateEntityFrom(eid, newTemplate)
