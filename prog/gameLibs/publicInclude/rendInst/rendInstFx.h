//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>

class AcesEffect;
class DataBlock;
class TMatrix;

namespace rifx
{
namespace composite
{
int loadCompositeEntityTemplate(const DataBlock *blk);
void spawnEntitiy(rendinst::riex_handle_t ri_idx, unsigned short int template_id, const TMatrix &tm, bool restorable);
void setEntityApproved(rendinst::riex_handle_t ri_idx, bool approved);
} // namespace composite

} // namespace rifx
