//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <EASTL/unique_ptr.h>

class BackBufferHolder
{
private:
  TextureIDPair srgbFrame;
  bool readable;

  void init();
  void close();

  static eastl::unique_ptr<BackBufferHolder> holder;
  BackBufferHolder();

public:
  static void create(); // allocates holder
  static void update();
  static void destroy(); // destroys holder
  static void d3dReset(bool);

  static bool isReadable() { return holder && holder->readable; }
  static const TextureIDPair getTex(bool force_copy = false);
  static void releaseTex();

  ~BackBufferHolder();
};
