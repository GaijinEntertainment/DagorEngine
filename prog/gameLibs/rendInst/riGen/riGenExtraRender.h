#pragma once

#include "riGenExtra.h"

#include <3d/dag_ringDynBuf.h>
#include <3d/dag_resPtr.h>


namespace rendinst
{

struct VbExtraCtx
{
  RingDynamicSB *vb = nullptr;
  uint32_t gen = 0;
  int lastSwitchFrameNo = 0;
};
extern VbExtraCtx vbExtraCtx[RIEX_RENDERING_CONTEXTS];
extern UniqueBufHolder perDrawData;

extern float riExtraLodDistSqMul;
extern float riExtraCullDistSqMul;

} // namespace rendinst
