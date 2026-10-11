// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstCollision.h>

namespace rendinstdestr
{

#if ENABLE_APEX
bool remove_apex_destructed_ri(const rendinst::RendInstDesc &desc, int apex_destr_id);
#endif

namespace netsync
{

void call_restorable_rendinst_cb(const rendinst::RendInstDesc &desc, rendinstdestr::RestorableRendinstState state);
void add_restorable(const rendinst::RendInstDesc &desc, const rendinst::RendInstDesc &offsetedDesc,
  const rendinst::RendInstBufferData &riBuffer, destructables::id_t destrId, float at_time, const rendinst::CollisionInfo *coll_info,
  rendinst::riex_handle_t createdDestroyedRiexHandle, int apex_destructible_id, const TMatrix &mainTm, const Point3 &pos,
  const Point3 &impulse);
void update_restorables(float dt);

void init_sync(bool apply_pending, bool register_ri_callbacks);
void start_session();
void end_session();
void clear_sync();
void perform_delayed_destruction();

} // namespace netsync

} // namespace rendinstdestr
