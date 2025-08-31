// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/dag_vr.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_sleepPrecise.h>

#include "vrEmulatorInputHandler.h"

class PostFxRenderer;

class VREmulatorDevice : public VRDevice
{
  friend struct VRDevice;

public:
  VREmulatorDevice(const char *profile);
  ~VREmulatorDevice() override;

  virtual RenderingAPI getRenderingAPI() const override;
  virtual AdapterID getAdapterId() const override;
  virtual const char *getRequiredDeviceExtensions() const override;
  virtual const char *getRequiredInstanceExtensions() const override;
  virtual eastl::pair<uint64_t, uint64_t> getRequiredGraphicsAPIRange() const override;

  virtual void getViewResolution(int &width, int &height) const override;
  virtual float getViewAspect() const override;

  virtual bool isHDR() const override;

  virtual bool isActive() const override;

  virtual bool setRenderingDeviceImpl() override;

  virtual void setZoom(float zoom) override;

  virtual void setUpSpace() override;

  virtual bool isReadyForRender() override;

  virtual void tick(const SessionCallback &cb) override;

  virtual bool prepareFrame(FrameData &frameData, float zNear, float zFar) override;
  virtual void beginFrame(FrameData &frameData) override;

  virtual bool hasQuitRequest() const override;

  virtual VrInput *getInputHandler() override { return &inputHandler; }

  virtual bool hasScreenMask() override;
  virtual void retrieveScreenMaskTriangles(const TMatrix4 &projection, eastl::vector<Point4> &visibilityMaskVertices,
    eastl::vector<uint16_t> &visibilityMaskIndices, int view_index) override;
  virtual void beforeSoftDeviceReset() override;
  virtual void afterSoftDeviceReset() override;

  virtual bool isInStateTransition() const override;

  virtual bool isEmulated() const override { return true; }

  const Point3 &GetPosition() const { return position; }
  const TMatrix &GetOrientation() const { return orientation; }

protected:
  virtual bool isInitialized() const override;

  virtual void beginRender(FrameData &frameData) override;
  virtual void endRender(FrameData &frameData) override;

private:
  float zoom = 1;

  bool isHmdOn;
  bool callFirstHmdOn;
  bool isHmdWorn;

  Point3 position;
  TMatrix orientation;

  UniqueTex renderTargets[2];

  eastl::unique_ptr<PostFxRenderer> renderer;
  shaders::UniqueOverrideStateId scissor;

  VrEmulatorInputHandler inputHandler;

  int fpsLimit;
  PreciseSleepContext preciseSleepContext = {};
};
