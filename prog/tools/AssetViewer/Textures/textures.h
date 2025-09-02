// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <math/dag_Point2.h>

#include <EditorCore/ec_interface.h>
#include <propPanel/c_control_event_handler.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <shaders/dag_overrideStateId.h>
#include <libTools/util/iesReader.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include "tex_cm.h"


class TexturesPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  TexturesPlugin();
  ~TexturesPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "texturesEditor"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override;
  bool end() override;
  void registerMenuAccelerators() override;
  void handleViewportAcceleratorCommand([[maybe_unused]] IGenViewportWnd &wnd, [[maybe_unused]] unsigned id) override;
  bool reloadAsset(DagorAsset *asset) override;

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override { return false; }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override;
  void renderTransObjects() override {}

  bool supportAssetType(const DagorAsset &asset) const override;
  bool supportEditing() const override { return false; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override;

  // ControlEventHandler
  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pid, PropPanel::ContainerPropertyControl *panel) override;

private:
  Point2 viewTile;
  struct CubeData
  {
    VPROG vprog = BAD_VPROG;
    FSHADER fsh = BAD_FSHADER;
    VDECL vdecl = BAD_VDECL;
    Vbuffer *vb = nullptr;
    Ibuffer *ib = nullptr;
  } cubeData;

  BaseTexture *texture = nullptr;
  TEXTUREID texId = BAD_TEXTUREID;

  StdGuiRender::StdGuiShader shader;

  DagorAsset *curAsset = nullptr, *curUhqAsset = nullptr, *curHqAsset = nullptr, *curTqAsset = nullptr;
  E3DCOLOR mColor;
  Color4 cMul = Color4(1, 1, 1, 1), cAdd = Color4(0, 0, 0, 0);
  float cScaleDb = 0;
  int colorModeSVId = -1, aTestSVId = -1;
  bool forceStretch = true, hasAlpha = false, isMoving = false;
  int chanelsRGB = 3;
  bool showMipStripe = false;
  Point2 texMove = Point2(0, 0), priorMousePos = Point2(0, 0);
  int viewAtestVal = 0, viewRgbaMode = ID_COLOR_MODE_RGBA, viewMipLevel = -1;
  int showTexQ = ID_SHOW_TEXQ_HIGH;
  int defShowTexQ = ID_SHOW_TEXQ_HIGH;
  TextureInfo currentTI;
  unsigned texReqLev = 15;
  bool diffTexStatsReady = false;
  float tcZ = 0;
  float texScale = 1.0f;
  shaders::OverrideStateId renderVolImageOverrideId;

  void setIesData();
  int getTextureProps(ManagedTexEntryDesc &out_desc, String &out_type, String &out_dim, String &out_mem_sz);
  bool getDiffTextureStats(String &stat1, String &stat2);

  bool iesTexture = false;
  IesReader::IesParams iesParams;
  float iesScale = 1;
  bool iesRotated = false;
  float iesOptimalScale = 1;
  bool iesOptimalRotated = false;
  bool iesValid = false;
  float iesWastedArea = 0;
};
