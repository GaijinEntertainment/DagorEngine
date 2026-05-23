// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>

class BaseTexture;
class FlyModeBlackBox;
struct ILandscapePreviewService;
struct IGraphTexGenService;

// Plugin-side camera + service client. All heavy rendering lives host-side
// in ILandscapePreviewService (pluginService/landscape_preview_service.cpp);
// this class owns the free-fly camera state and asks the service to render.
class LandscapePreviewScene final
{
public:
  LandscapePreviewScene();
  ~LandscapePreviewScene();

  struct CameraInput
  {
    bool fwd = false, back = false, left = false, right = false;
    bool worldUp = false, worldDown = false;
    bool turbo = false;
    float mouseDx = 0.0f; // raw pixel delta per frame (FlyModeBlackBox applies angSpdScaleH/V)
    float mouseDy = 0.0f;
  };

  // Acquires the host-side ILandscapePreviewService on first call.
  bool init();

  void actCamera(float dt, const CameraInput &in);

  // Pushes latest heightmap metadata from the graph to the service.
  void updateHeightmapParams(IGraphTexGenService &texgen);

  // Arm a deferred render with the given frame delta (call from updateImgui).
  void setRenderParams(float dt);

  // Execute the deferred render NOW. Call from the plugin's renderObjects().
  void executeRender();

  // Returns the last rendered output texture for ImGui::Image, or nullptr.
  BaseTexture *getOutputTexture();

  // Backing size of the output texture. The panel UV-crops a centered sub-rect
  // of this to match its own aspect.
  IPoint2 getOutputAllocSize() const;

  // Status reporting for the panel.
  bool isServiceAvailable() const;
  const char *getUnavailableReason() const;

  const Point3 &getCameraPos() const { return cameraPos; }
  const Point3 &getCameraForward() const { return cameraForward; }

private:
  ILandscapePreviewService *service = nullptr;
  bool serviceLookedUp = false;

  // Camera state driven by engine FlyModeBlackBox (same class MouseKbdFreeCameraDriver
  // wraps) so the preview camera matches prog/tools/graphEditor/app.cpp bit-for-bit.
  FlyModeBlackBox *flyMode = nullptr;
  float accelTime = 0.0f;
  Point3 cameraPos = Point3(0.0f, 0.0f, 0.0f);
  Point3 cameraForward = Point3(0.0f, 0.0f, 1.0f);
  TMatrix viewTm = TMatrix::IDENT;
};
