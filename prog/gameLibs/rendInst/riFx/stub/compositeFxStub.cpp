// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstFxDetail.h>

namespace rifx
{
namespace composite
{

int loadCompositeEntityTemplate(const DataBlock *) { return -1; }
void spawnEntitiy(rendinst::riex_handle_t, unsigned short int, const TMatrix &, bool) {}
void setEntityApproved(rendinst::riex_handle_t, bool) {}

namespace detail
{
int getTypeByName(const char *) { return -1; }
void startEffect(int, const TMatrix &, AcesEffect **) {}
void stopEffect(AcesEffect *) {}
} // namespace detail
} // namespace composite
} // namespace rifx
