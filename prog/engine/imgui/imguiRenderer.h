// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resPtr.h>

struct ImGuiIO;
struct ImDrawData;

class DagImGuiRenderer
{
public:
  DagImGuiRenderer();
  void setBackendFlags(ImGuiIO &io);
  void createAndSetFontTexture(ImGuiIO &io);
  void render(ImDrawData *draw_data);

private:
  DynamicShaderHelper renderer;
  shaders::UniqueOverrideStateId overrideStateId;
  VDECL vDecl = BAD_VDECL;
  int vStride = -1;
  UniqueTex fontTex;
  UniqueBuf vb;
  UniqueBuf ib;
  uint32_t vbSize = 0;
  uint32_t ibSize = 0;
};