//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_common_interface.h>

class Point3;

typedef uint32_t cable_handle_t;

class ICableService
{
public:
  static constexpr unsigned HUID = 0xA55B0B19u; // ICableService

  virtual cable_handle_t addCable(const Point3 &start, const Point3 &end, float radius, float sag) = 0;
  virtual void removeCable(cable_handle_t id) = 0;

  virtual void beforeRender(float pixel_scale) = 0;
  virtual void renderGeometry() = 0;

  virtual void exportCables(BinDumpSaveCB &cwr) = 0;
};

ICableService *queryCableService();
