require DagorMath
require ecs

[es(tag=render, after=charctrl_update_es, before=animchar_es, before=after_camera_sync)]
def charctrl_update_camera_transform_es(evt : UpdateStageInfoAct; phys_global__tick_residual : float)
  query() <| $ [es] (var transform : float3x4&;
                     phys__pos : float3;
                     phys__rot : float4;
                     phys__vel : float3;
                     phys__wvel : float3;
                     charctrl__capsule_dir : float3;
                     charctrl__capsule_ofs : float2;
                     charctrl__view_ht : float&)
    var pos : float3 = phys__pos
    var rot : float4 = phys__rot
    let dt = phys_global__tick_residual
    if dt > 0.f
      let orientationInc = phys__wvel * dt
      let quatInc = quat(orientationInc, length(orientationInc))
      let orient = DagorMath::quat(phys__rot)
      rot = float4(normalize(orient * quatInc))
      pos += phys__vel * dt

    let view_ht = (charctrl__view_ht - charctrl__capsule_ofs.y) * (1.0f - charctrl__capsule_ofs.x)
    pos += quat_mul_vec(quat_from_euler(euler_from_quat(rot).x, 0.f, 0.f), charctrl__capsule_dir) * view_ht
    pos.y += charctrl__capsule_ofs.y
    make_tm(DagorMath::quat(rot), pos, transform)
