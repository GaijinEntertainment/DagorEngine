//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_baseInterfaces.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_rendEdObject.h>
#include <libTools/util/iLogWriter.h>
#include <util/dag_simpleString.h>

// external classes forward declaration
class DataBlock;
class IGenSave;
class StaticGeometryContainer;
class SH3Lighting;
class SHUnifiedLighting;
class GeomObject;
class RenderableInstanceLodsResource;
class TMatrix;
namespace mkbindump
{
class BinDumpSaveCB;
};

using mkbindump::BinDumpSaveCB;


template <class T>
class BezierSplineInt;

class IPluginObject : public RenderableEditableObject
{
public:
  enum PluginType
  {
    PLUGIN_TYPE_ENVI,
    PLUGIN_TYPE_CAMERA,
    PLUGIN_TYPE_NAVIGATION,
    PLUGIN_TYPE_FLARE,
    PLUGIN_TYPE_GL,
    PLUGIN_TYPE_PREFAB,
    PLUGIN_TYPE_SOUND,
    PLUGIN_TYPE_PATH_POINT,
    PLUGIN_TYPE_NAVIGATION_POINT,
  };

  IPluginObject() : RenderableEditableObject() {}
  IPluginObject(const IPluginObject *fo) : RenderableEditableObject(fo) {}

  virtual PluginType getPluginType() const = 0;
  virtual void renderColor(Tab<DynRenderBuffer *> &render_buf, const E3DCOLOR &c){};

  virtual IPluginObject *virtualNew() = 0;
};

//
// common interfaces
//

class IRenderOnCubeTex
{
public:
  static constexpr unsigned HUID = 0x43625504u; // IRenderOnCubeTex
  virtual void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit) = 0;
};
