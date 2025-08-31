//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum TeamType
{
  TEAM_UNASSIGNED = -1, // aka TEAM_INVALID, aka TEAM_UNKNOWN
  TEAM_SPECTATOR = 0,   // aka TEAM_NEUTRAL
  LAST_SHARED_TEAM = TEAM_SPECTATOR,
  FIRST_GAME_TEAM = LAST_SHARED_TEAM + 1 // game specific enumeration starts from here
};

namespace game
{
inline bool is_teams_friendly(int lhs_team, int rhs_team)
{
  if (lhs_team != rhs_team || lhs_team == TEAM_UNASSIGNED || rhs_team == TEAM_UNASSIGNED)
    return false;
  // Note: support for 'team.allHostile' functionality was dropped for perfomance reasons
  // Note2: here we can check that passed team is actually valid (e.g. by checking that it's entity exist),
  // but we would not for perfomance reasons (at least for now)
  return true;
}
}; // namespace game
