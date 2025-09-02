//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_common_interface.h>

#include <math/dag_Point3.h>


class IWaterProjFxService : public IRenderingService
{
public:
  static constexpr unsigned HUID = 0xAF2F6404u; // IWaterProjFxService

  virtual void init() = 0;

  virtual void beforeRender(Stage stage) = 0;
  void renderGeometry(Stage stage) override = 0;

  virtual void render(float waterLevel, float significantWaveHeight) = 0;
};
