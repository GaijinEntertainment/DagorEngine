require ecs
require net
require DngNet
require DagorRandom
require DngActor
require DngCamera
require outer_space.events_outer_space
require outer_space.es.camera_common

[es(tag=gameClient, on_appear)]
def finalize_possesion_client(evt : Event; eid, possessedByPlr : EntityId; var replication : NetObject?)
  let possessedEid = eid
  query(possessedByPlr) <| $ [es(REQUIRE=player)] (var possessed : EntityId&; is_local : bool)
    if !is_local
      return

    print("player_possess_client {int(uint(possessedByPlr))}: {int(uint(possessedEid))}")

    possessed = possessedEid
    var actor : BasePhysActor? = get_phys_actor(possessedEid)

    if replication != null // basically same as has_network()
      if actor != null
        *actor |> base_phys_actor_setRoleAndTickrateType(NetRole ROLE_LOCALLY_CONTROLLED_SHADOW, PhysTickRateType Normal)
      let sconn : IConnection const? = get_server_conn()
      if sconn != null
        *replication |> net_object_setControlledBy(connection_getId(*sconn))

    if actor != null
      let newActorTemplName : string = "{getEntityFutureTemplateName(possessedEid)}+{actor.physTypeStr}_hero_components_local+observed_local"
      let plrEid = possessedByPlr
      let finalizeReCreation <- @ <| (actor_eid : EntityId)
        possess_entity_camera(actor_eid, /*cam_idx*/0, plrEid, /*cam_names*/INVALID_ENTITY_ID)
      possessedEid |> reCreateEntityFrom(newActorTemplName, finalizeReCreation) <| $(var init : ComponentsInitializer)
        set(init, "watchedByPlr", plrEid)
    else
      possess_entity_camera(eid, /*cam_idx*/0, possessedByPlr, /*cam_names*/INVALID_ENTITY_ID)
