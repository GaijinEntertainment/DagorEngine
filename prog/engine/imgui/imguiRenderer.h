// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resPtr.h>

struct ImGuiIO;
struct ImGuiPlatformIO;

struct ImDrawData;
struct ImDrawVert;
typedef unsigned short ImDrawIdx;

class DagImGuiRenderer
{
public:
  DagImGuiRenderer();
  void setBackendFlags(ImGuiIO &io);
  void createAndSetFontTexture(ImGuiIO &io);
  void render(ImGuiPlatformIO &platform_io);
  void renderDrawDataToTexture(const ImDrawData *draw_data, BaseTexture *rt);

private:
  void resizeBuffers(const uint32_t vtx_count, const uint32_t idx_count);
  void lockBuffers(ImDrawVert *&vbdata, const uint32_t vtx_count, ImDrawIdx *&ibdata, const uint32_t idx_count);
  void unlockBuffers();
  bool copyDrawData(const ImDrawData *draw_data, ImDrawVert *&vbdata, ImDrawIdx *&ibdata);
  void processDrawDataToRT(const ImDrawData *draw_data, int &global_idx_offset, int &global_vtx_offset);

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