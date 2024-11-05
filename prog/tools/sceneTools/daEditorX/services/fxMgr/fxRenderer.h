// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <generic/dag_span.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>


class BaseEffectObject;

class IGenViewportWnd;


class FxRenderer
{
public:
  void render(IGenViewportWnd &wnd, eastl::function<void()> render_fx_cb);

private:
  void init();
  void initResolution(int width, int height_);

  IPoint2 screenRes = IPoint2::ZERO;
  bool isInited = false;

  PostFxRenderer finalRender;
  UniqueTexHolder colorRt;
};
