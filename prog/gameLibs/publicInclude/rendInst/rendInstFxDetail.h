//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class AcesEffect;
class TMatrix;

namespace rifx
{
namespace composite
{
namespace detail
{
int getTypeByName(const char *name);
void startEffect(int type, const TMatrix &fx_tm, AcesEffect **locked_fx = nullptr);
void stopEffect(AcesEffect *fx_handle);
} // namespace detail
} // namespace composite

} // namespace rifx
