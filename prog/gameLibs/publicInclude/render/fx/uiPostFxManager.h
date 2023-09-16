//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resId.h>

class IPoint2;
class IBBox2;

class UiPostFxManager
{
public:
  static bool isBloomEnabled();
  static TEXTUREID getUiBlurTexId();

  static bool isBlurRequested();
  static void resetBlurRequest();

  static void setBlurUpdateEnabled(bool val);

  // this method will update result of getUiBlurTexId()
  // it is about 3~4 times slower, than using highly optimized blur during bloom (as we downsample scene 4 times anyway for bloom)
  // but this allow to make 'UI panels' (one over another). Which should be fast enough for menu UI.
  // Note! it will invalide getUiBlurTexId() result outside requested area (well, not completely invalidate, but there will be 'seam')
  // So call it only in strict order of layers
  static void updateFinalBlurred(const IPoint2 &lt, const IPoint2 &end, int max_mip);  // end is rb +1, i.e. one pixel after. rb-lt ==
                                                                                       // wd
  static void updateFinalBlurred(const IBBox2 *begin, const IBBox2 *end, int max_mip); // end is rb +1, i.e. one pixel after. rb-lt ==
                                                                                       // wd
};
