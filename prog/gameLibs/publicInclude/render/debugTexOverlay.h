//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <util/dag_string.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resId.h>

class PostFxRenderer;

//
// custom texture overlay (show_tex console command)
//
class DebugTexOverlay
{
  static constexpr int MAX_TEXTURE_CNT = 4;
  static constexpr int PARAM_PER_TEXTURE = 8; // {texture_name, width, height, offset_x, offset_y, swizzle, range0, range1}
  static constexpr int SEPARATOR_CNT = MAX_TEXTURE_CNT - 1;

public:
  static constexpr int MAX_CONSOLE_ARGS_CNT = 1 + PARAM_PER_TEXTURE * MAX_TEXTURE_CNT + SEPARATOR_CNT;

  DebugTexOverlay();
  DebugTexOverlay(const Point2 &target_size);
  ~DebugTexOverlay();

  String processConsoleCmd(const char *argv[], int argc);
  void setTargetSize(const Point2 &target_size);
  void hideTex();
  void render();

private:
  class TextureWrapper
  {
  public:
    TextureWrapper();
    ~TextureWrapper();
    void reset();
    String initFromConsoleCmd(const char *argv[], int argc, const Point2 &target_size);
    void render(const Point2 &targetSize, const PostFxRenderer &renderer);

  private:
    enum
    {
      Zeroes = 4096,
      Ones = 8192,
      Alpha = 16384
    };

    void setTexEx(const Point2 &target_size_, TEXTUREID texId, const Point2 &offset, const Point2 &size, const carray<Point4, 4> &swz,
      const carray<Point2, 4> &mod, int mip, int face, int num_slices = 0, int filter = 0);
    void fixAspectRatio(const Point2 &targetSize);

    TEXTUREID texId;
    bool needToReleaseTex = false;
    int mip = 0;
    int face = 0;
    int numSlices = 0;
    int dataFilter = 0;
    Point2 offset = Point2(0, 0);
    Point2 size = Point2(0, 0);
    carray<Point4, 4> swizzling;
    carray<Point2, 4> modifiers;
  };

  void init();

  Tab<TextureWrapper> textures;
  Point2 targetSize;
  PostFxRenderer *renderer;
  shaders::UniqueOverrideStateId scissor;
};
