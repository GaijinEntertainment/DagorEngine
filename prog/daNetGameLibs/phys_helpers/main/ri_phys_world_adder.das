require ecs
require Dacoll
require DagorMath

[es(no_order)]
def ri_phys_world_adder(info : UpdateStageInfoAct; var ri_phys_world_adder__nextAt : float&; ri_phys_world_adder__timer : float;
                        ri_phys_world_adder__boxExtends : float3; transform : float3x4)
  if info.curTime <= ri_phys_world_adder__nextAt
    return
  ri_phys_world_adder__nextAt = info.curTime + ri_phys_world_adder__timer
  let box = BBox3(transform[3] - ri_phys_world_adder__boxExtends * 0.5, transform[3] + ri_phys_world_adder__boxExtends * 0.5)
  update_ri_cache_in_volume_to_phys_world(box)


