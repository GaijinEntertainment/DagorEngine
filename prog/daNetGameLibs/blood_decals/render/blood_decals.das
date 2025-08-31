require ecs
require danetlibs.blood_decals.render.blood_decals_common
require ProjectiveDecals
require danetlibs.renderer.includes.pufd_events


[es(tag=gameClient, no_order)]
def blood_decals_manager_add_decal(evt : CmdAddBloodDecalInternal;
                                   var blood_decals_manager : RingBufferDecals&)
  blood_decals_manager |> addDecal(evt.tm, evt.rad, evt.tex_id, evt.mx_id, evt.params)


[es(tag=gameClient, on_event=CmdClearBloodDecals)]
def blood_decals_manager_clear(evt : Event;
                               var blood_decals_manager : RingBufferDecals&)
  blood_decals_manager |> clear()

[es(tag=gameClient, no_order)]
def blood_decal_manager_update(evt : ParallelUpdateFrameDelayed;
                               eid : EntityId;
                               blood_decal_manager__fromPos : float3;
                               blood_decal_manager__targets : EidList;
                               blood_decal_manager__decalPerUpdate : int;
                               blood_decal_manager__offset : float3;
                               var blood_decal_manager__currentIndex : int&)
  let lastTargetIndex = length(blood_decal_manager__targets)
  let endIndex = min(lastTargetIndex, blood_decal_manager__currentIndex + blood_decal_manager__decalPerUpdate)
  for i in range(blood_decal_manager__currentIndex, endIndex)
    query(blood_decal_manager__targets[i]) <| $ [es] (transform : float3x4)
      let pos = transform[3] + blood_decal_manager__offset
      blood_decals_add_decal(blood_decal_manager__fromPos, pos)
  blood_decal_manager__currentIndex = endIndex
  if endIndex == lastTargetIndex
    destroyEntity(eid)
