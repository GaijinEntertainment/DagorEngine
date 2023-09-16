#pragma once

#include "../av_plugin.h"

#include <3d/dag_drv3d.h>
#include <math/dag_Point2.h>

#include <EditorCore/ec_interface.h>
#include <propPanel2/c_control_event_handler.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <shaders/dag_overrideStateId.h>
#include <libTools/util/iesReader.h>
#include "tex_cm.h"


class TexturesPlugin : public IGenEditorPlugin, public ControlEventHandler
{
public:
  TexturesPlugin();
  ~TexturesPlugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "texturesEditor"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset);
  virtual bool end();
  virtual bool reloadAsset(DagorAsset *asset);

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const { return false; }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects();
  virtual void renderTransObjects() {}

  virtual bool supportAssetType(const DagorAsset &asset) const;
  virtual bool supportEditing() const { return false; }

  virtual void fillPropPanel(PropertyContainerControlBase &panel);
  virtual void postFillPropPanel() {}

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif);

  // ControlEventHandler
  virtual void onChange(int pid, PropertyContainerControlBase *panel);
  virtual void onClick(int pid, PropertyContainerControlBase *panel);

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
  int viewAtestVal = 1, viewRgbaMode = ID_COLOR_MODE_RGBA, viewMipLevel = -1;
  int showTexQ = TQL_high;
  float tcZ = 0;
  float texScale = 1.0f;
  shaders::OverrideStateId renderVolImageOverrideId;

  void setIesData();

  bool iesTexture = false;
  IesReader::IesParams iesParams;
  float iesScale = 1;
  bool iesRotated = false;
  float iesOptimalScale = 1;
  bool iesOptimalRotated = false;
  bool iesValid = false;
  float iesWastedArea = 0;
};
