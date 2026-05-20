module collimator_moa_public shared
require ecs


def collimator_moa_is_moa_scope(gun_mod_eid : EntityId) : bool
  if !gun_mod_eid || isLoadingEntity(gun_mod_eid)
    return false
  var res = false
  query(gun_mod_eid) <| $ [es(REQUIRE=gunmod__collimator_moa_img_template)]
    res = true
  return res


def collimator_moa_set_gun_mod_eid(gun_mod_eid : EntityId)
  query() <| $ [es] (var collimator_moa_render__gun_mod_eid : EntityId&;
                     var collimator_moa_render__relem_update_required : bool&)
    if collimator_moa_render__gun_mod_eid == gun_mod_eid
      return
    collimator_moa_render__gun_mod_eid = gun_mod_eid
    collimator_moa_render__relem_update_required = true
