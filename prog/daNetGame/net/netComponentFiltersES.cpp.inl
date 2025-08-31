// ATTENTION!
// this file is coupling things to much! Use daScript to add new component filters, @see [register_component_filter] annotation
// shouldDecreaseSize, allowedSizeIncrease = 38

#include <daECS/net/component_replication_filter.h>
#include <daECS/net/connection.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include "ecs/game/generic/team.h"

enum class PossessionResult
{
  NotControlled,
  Controlled,
  Unknown
};
static inline PossessionResult check_controlled(net::ConnectionId controlled_by, net::ConnectionId repl_to)
{
  if (controlled_by == net::INVALID_CONNECTION_ID && repl_to == net::INVALID_CONNECTION_ID)
    return PossessionResult::Unknown;
  return (controlled_by == repl_to) ? PossessionResult::Controlled : PossessionResult::NotControlled;
}

static net::CompReplicationFilter filter_spectated(ecs::EntityId eid, net::ConnectionId, const net::IConnection *conn)
{
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  if (plrEid == ecs::INVALID_ENTITY_ID) // black hole e.g. replay
    return net::CompReplicationFilter::ReplicateNow;

  return ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID) != eid ? net::CompReplicationFilter::SkipNow
                                                                       : net::CompReplicationFilter::ReplicateNow;
}

static net::CompReplicationFilter filter_possessed(ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  PossessionResult possession = check_controlled(cntrl_by, conn->getId());
  if (possession != PossessionResult::Unknown)
    return possession == PossessionResult::NotControlled ? net::CompReplicationFilter::SkipNow
                                                         : net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());

  if (plrEid == ecs::INVALID_ENTITY_ID)
    return net::CompReplicationFilter::ReplicateNow;
  if (ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID) != eid)
    return net::CompReplicationFilter::SkipNow;
  return net::CompReplicationFilter::ReplicateNow;
}

static net::CompReplicationFilter filter_same_player(ecs::EntityId eid, net::ConnectionId, const net::IConnection *conn)
{
  // for player connection never change
  return ecs::EntityId((ecs::entity_id_t)(uintptr_t)conn->getUserPtr()) == eid ? net::CompReplicationFilter::ReplicateForConnection
                                                                               : net::CompReplicationFilter::SkipForConnection;
}

static bool is_friendly_team_or(ecs::EntityId player_eid, ecs::EntityId entity_id, bool default_value)
{
  const int playerTeam = ECS_GET_OR(player_eid, team, (int)TEAM_UNASSIGNED);
  const int *entityTeam = ECS_GET_NULLABLE(int, entity_id, team);
  if (entityTeam)
    return game::is_teams_friendly(playerTeam, *entityTeam);

  // if there is no team in entity itself, check it's possessor (player) team
  const ecs::EntityId entityPlayerEid = ECS_GET_OR(entity_id, possessedByPlr, ecs::INVALID_ENTITY_ID);
  if (entityPlayerEid)
  {
    entityTeam = ECS_GET_NULLABLE(int, entityPlayerEid, team);
    if (entityTeam)
      return game::is_teams_friendly(playerTeam, *entityTeam);
  }
  return default_value;
}

static net::CompReplicationFilter filter_friendly_team(ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  return is_friendly_team_or(plrEid, eid, true /*by default - replicate, be conservative*/) ? net::CompReplicationFilter::ReplicateNow
                                                                                            : net::CompReplicationFilter::SkipNow;
}

static net::CompReplicationFilter filter_possessed_and_spectated(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  PossessionResult possession = check_controlled(cntrl_by, conn->getId());
  if (possession == PossessionResult::Controlled)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());

  if (plrEid == ecs::INVALID_ENTITY_ID)
    return net::CompReplicationFilter::ReplicateNow;
  if (ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID) != eid)
  {
    if (possession == PossessionResult::NotControlled || ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID) != eid)
      return net::CompReplicationFilter::SkipNow;
  }
  return net::CompReplicationFilter::ReplicateNow;
}

static net::CompReplicationFilter filter_possessed_squad_and_spectated_squad(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  if (!plrEid)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId possessed = ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectated = ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID);

  ecs::EntityId squad = ECS_GET_OR(eid, squad_member__squad, ecs::INVALID_ENTITY_ID);
  ecs::EntityId plrSquad = ECS_GET_OR(possessed, squad_member__squad, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectatedSquad = ECS_GET_OR(spectated, squad_member__squad, ecs::INVALID_ENTITY_ID);
  if (squad && ((squad == plrSquad) || (squad == spectatedSquad)))
    return net::CompReplicationFilter::ReplicateNow;

  return filter_possessed_and_spectated(eid, cntrl_by, conn);
}

static net::CompReplicationFilter filter_possessed_spectated_and_attachables(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());

  if (plrEid == ecs::INVALID_ENTITY_ID)
    return net::CompReplicationFilter::ReplicateNow;
  if (ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID) != eid && ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID) != eid)
  {
    ecs::EntityId attachedTo = ECS_GET_OR(eid, animchar_attach__attachedTo, ecs::INVALID_ENTITY_ID);
    if (attachedTo && attachedTo != eid)
      return filter_possessed_spectated_and_attachables(attachedTo, cntrl_by, conn);
    ecs::EntityId gunOwner = ECS_GET_OR(eid, gun__owner, ecs::INVALID_ENTITY_ID);
    if (gunOwner && gunOwner != eid)
      return filter_possessed_spectated_and_attachables(gunOwner, cntrl_by, conn);
    return net::CompReplicationFilter::SkipNow;
  }
  return net::CompReplicationFilter::ReplicateNow;
}

static net::CompReplicationFilter filter_possessed_vehicle(ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  ecs::EntityId possessed = ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID);
  if (!plrEid)
    return net::CompReplicationFilter::ReplicateNow;

  ecs::EntityId plrVehicle = ECS_GET_OR(possessed, human_anim__vehicleSelected, ecs::INVALID_ENTITY_ID);
  ecs::EntityId vehicle = ECS_GET_OR(eid, turret__owner, ecs::INVALID_ENTITY_ID);
  return eid == plrVehicle || (vehicle && vehicle == plrVehicle) ? net::CompReplicationFilter::ReplicateNow
                                                                 : net::CompReplicationFilter::SkipNow;
}

static net::CompReplicationFilter filter_possessed_and_spectated_vehicle(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  if (!plrEid)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId possessed = ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectated = ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID);

  ecs::EntityId plrVehicle = ECS_GET_OR(possessed, human_anim__vehicleSelected, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectatedVehicle = ECS_GET_OR(spectated, human_anim__vehicleSelected, ecs::INVALID_ENTITY_ID);
  ecs::EntityId vehicle = ECS_GET_OR(eid, turret__owner, ecs::INVALID_ENTITY_ID);
  return eid == plrVehicle || eid == spectatedVehicle || (vehicle && (vehicle == plrVehicle || vehicle == spectatedVehicle))
           ? net::CompReplicationFilter::ReplicateNow
           : net::CompReplicationFilter::SkipNow;
}

static net::CompReplicationFilter filter_possessed_spectated_attachables_and_vehicle(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());

  if (plrEid == ecs::INVALID_ENTITY_ID)
    return net::CompReplicationFilter::ReplicateNow;

  ecs::EntityId possessed = ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectated = ECS_GET_OR(plrEid, specTarget, ecs::INVALID_ENTITY_ID);
  ecs::EntityId plrVehicle = ECS_GET_OR(possessed, human_anim__vehicleSelected, ecs::INVALID_ENTITY_ID);
  ecs::EntityId spectatedVehicle = ECS_GET_OR(spectated, human_anim__vehicleSelected, ecs::INVALID_ENTITY_ID);
  ecs::EntityId turretVehicle = ECS_GET_OR(eid, turret__owner, ecs::INVALID_ENTITY_ID);

  if (eid == possessed || eid == spectated || eid == plrVehicle || eid == spectatedVehicle ||
      (turretVehicle && (turretVehicle == plrVehicle || turretVehicle == spectatedVehicle)))
    return net::CompReplicationFilter::ReplicateNow;

  ecs::EntityId attachedTo = ECS_GET_OR(eid, animchar_attach__attachedTo, ecs::INVALID_ENTITY_ID);
  if (attachedTo && attachedTo != eid)
    return filter_possessed_spectated_and_attachables(attachedTo, cntrl_by, conn);
  ecs::EntityId gunOwner = ECS_GET_OR(eid, gun__owner, ecs::INVALID_ENTITY_ID);
  if (gunOwner && gunOwner != eid)
    return filter_possessed_spectated_and_attachables(gunOwner, cntrl_by, conn);
  return net::CompReplicationFilter::SkipNow;
}

static net::CompReplicationFilter filter_possessed_team_spectated_attachables_and_vehicle(
  ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  if (conn->getId() == cntrl_by)
    return net::CompReplicationFilter::ReplicateNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());
  if (is_friendly_team_or(plrEid, eid, false))
    return net::CompReplicationFilter::ReplicateNow;
  return filter_possessed_spectated_attachables_and_vehicle(eid, cntrl_by, conn);
}

static net::CompReplicationFilter filter_not_possessed(ecs::EntityId eid, net::ConnectionId cntrl_by, const net::IConnection *conn)
{
  PossessionResult possession = check_controlled(cntrl_by, conn->getId());
  if (possession != PossessionResult::Unknown)
    return possession == PossessionResult::NotControlled ? net::CompReplicationFilter::ReplicateNow
                                                         : net::CompReplicationFilter::SkipNow;
  ecs::EntityId plrEid((ecs::entity_id_t)(uintptr_t)conn->getUserPtr());

  if (ECS_GET_OR(plrEid, possessed, ecs::INVALID_ENTITY_ID) == eid)
    return net::CompReplicationFilter::SkipNow;
  return net::CompReplicationFilter::ReplicateNow;
}

namespace bind_dascript
{
extern void das_init_component_replication_filters();
};

void init_component_replication_filters()
{
  net::reset_replicate_component_filters();
  net::register_component_filter("filter_spectated", filter_spectated);
  net::register_component_filter("filter_possessed", filter_possessed);
  net::register_component_filter("filter_not_possessed", filter_not_possessed);
  net::register_component_filter("filter_possessed_and_spectated", filter_possessed_and_spectated);
  net::register_component_filter("filter_same_player", filter_same_player);
  net::register_component_filter("filter_friendly_team", filter_friendly_team);
  net::register_component_filter("filter_possessed_squad_and_spectated_squad", filter_possessed_squad_and_spectated_squad);
  net::register_component_filter("filter_possessed_spectated_and_attachables", filter_possessed_spectated_and_attachables);
  net::register_component_filter("filter_possessed_spectated_attachables_and_vehicle",
    filter_possessed_spectated_attachables_and_vehicle);
  net::register_component_filter("filter_possessed_team_spectated_attachables_and_vehicle",
    filter_possessed_team_spectated_attachables_and_vehicle);
  net::register_component_filter("filter_possessed_vehicle", filter_possessed_vehicle);
  net::register_component_filter("filter_possessed_and_spectated_vehicle", filter_possessed_and_spectated_vehicle);

  bind_dascript::das_init_component_replication_filters();
}
