//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint2.h>

struct ToroidalHelper
{
  IPoint2 curOrigin, mainOrigin;
  unsigned texSize;
  ToroidalHelper() : texSize(1), curOrigin(IPoint2::ZERO), mainOrigin(IPoint2::ZERO) { resetOrigin(); }
  ToroidalHelper(const IPoint2 &origin, const IPoint2 &main, unsigned tsz) : texSize(tsz), curOrigin(origin), mainOrigin(main) {}

  void resetOrigin() { curOrigin = IPoint2(-1000000, 100000); }
};

struct ToroidalQuadRegion
{
  IPoint2 lt, wd, texelsFrom;
  ToroidalQuadRegion() {}
  ToroidalQuadRegion(const IPoint2 &lt_, const IPoint2 &wd_, const IPoint2 &tf_) : lt(lt_), wd(wd_), texelsFrom(tf_) {}
};
