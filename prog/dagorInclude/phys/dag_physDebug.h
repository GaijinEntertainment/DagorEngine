//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <util/dag_bitFlagsMask.h>

struct Frustum;
class Point3;

namespace physdbg
{
enum class RenderFlag : uint32_t
{
  BODIES = 1 << 0,
  CONSTRAINTS = 1 << 1,
  CONSTRAINT_LIMITS = 1 << 2,
  CONSTRAINT_REFSYS = 1 << 3,
  BODY_BBOX = 1 << 4,
  BODY_CENTER = 1 << 5,
  CONTACT_POINTS = 1 << 6,

  USE_ZTEST = 1 << 30
};
using RenderFlags = BitFlagsMask<RenderFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(RenderFlag);

inline RenderFlags max_dbg_flags()
{
  return RenderFlag::BODIES | RenderFlag::CONSTRAINTS | RenderFlag::CONSTRAINT_LIMITS | RenderFlag::CONSTRAINT_REFSYS |
         RenderFlag::BODY_BBOX | RenderFlag::BODY_CENTER | RenderFlag::CONTACT_POINTS;
}

template <typename PW>
extern void init();
template <typename PW>
extern void term();

template <>
void init<PhysWorld>();
template <>
void term<PhysWorld>();

void renderWorld(PhysWorld *pw, RenderFlags rflg, const Point3 *camPos = nullptr, float render_dist = 1000.0f,
  const Frustum *f = nullptr);
void renderOneBody(PhysWorld *pw, const PhysBody *pb, RenderFlags rflg = max_dbg_flags(), unsigned col = 0xFFFFFF);
void renderOneBody(PhysWorld *pw, const PhysBody *pb, const TMatrix &tm, RenderFlags rflg = max_dbg_flags(), unsigned col = 0xFFFFFF);

void setBufferedForcedDraw(PhysWorld *pw, bool forced);
bool isInDebugMode(PhysWorld *pw);
} // namespace physdbg
