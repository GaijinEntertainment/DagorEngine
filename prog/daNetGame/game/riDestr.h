// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class TMatrix;
class TMatrix4;
namespace net
{
class IConnection;
}

class AcesEffect;

namespace ridestr
{
void init(bool have_render);
void update(float dt, const TMatrix4 &glob_tm);
void send_initial_ridestr(net::IConnection &conn);
void shutdown();
}; // namespace ridestr
void ri_destr_start_effect(
  int type, const TMatrix &, const TMatrix &fx_tm, int pool_idx, bool is_player, AcesEffect **locked_fx, const char *effect_template);
