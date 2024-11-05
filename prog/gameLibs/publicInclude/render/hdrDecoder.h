//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_tex3d.h>

#include <shaders/dag_postFxRenderer.h>
#include <screenShotSystem/IHDRDecoder.h>
#include <EASTL/unique_ptr.h>

class DataBlock;

class HDRDecoder : public IHDRDecoder
{
  bool locked = false;
  UniqueTex sdrTex;
  UniqueTexHolder copyTex;
  eastl::unique_ptr<PostFxRenderer> decodeHDRRenderer;

public:
  HDRDecoder(const DataBlock &videoCfg);
  ~HDRDecoder();
  void Decode() override;
  char *LockResult(int &stride, int &w, int &h) override;
  bool IsActive() override;
};