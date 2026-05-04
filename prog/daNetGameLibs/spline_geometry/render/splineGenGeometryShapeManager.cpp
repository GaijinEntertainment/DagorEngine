// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometryShapeManager.h"
#include "splineGenGeometryShaderVar.h"
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"
#include "3d/dag_lockSbuffer.h"

SplineGenGeometryShapeManager::SplineGenGeometryShapeManager() { nameToIndex["default"] = defaultShapeData; }

void SplineGenGeometryShapeManager::createAndFillBuffer()
{
  if (!shapeBufElemCount)
  {
    shapeBuf.close();
    return;
  }

  shapeBuf = dag::create_sbuffer(sizeof(SplineGenShapeData), shapeBufElemCount,
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, 0, "shapeBuf");

  if (auto bufferData = lock_sbuffer<SplineGenShapeData>(shapeBuf.getBuf(), 0, shapeBufElemCount, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    bufferData.updateDataRange(0, shapeData.data(), shapeData.size());
  }
}

void SplineGenGeometryShapeManager::addShape(ecs::List<Point4> &shape, const eastl::string &name)
{
  if (shape.size() < 3)
  {
    logerr("SplineGenGeometryShapeManager::addShape: shape '%s' must have at least 3 points.", name.c_str());
    return;
  }

  if (nameToIndex.find(name) != nameToIndex.end())
  {
    logerr("SplineGenGeometryShape::addShape: shape with name '%s' already exists. Accidentally defined it twice?", name.c_str());
    return;
  }

  int ofs = shapeBufElemCount;
  int shapePointNr = shape.size();
  int segmentNr = shapePointNr - 1;
  shapeBufElemCount += segmentNr;

  for (int i = 0; i < shape.size() - 1; i++)
  {
    SplineGenShapeData sd;
    sd.firstPos = shape[i];
    sd.lastPos = shape[i + 1];
    shapeData.push_back(sd);
  }

  nameToIndex[name] = IPoint2(ofs, segmentNr);
}

IPoint2 SplineGenGeometryShapeManager::getShapeData(const eastl::string &name) const
{
  auto it = nameToIndex.find(name);
  if (it != nameToIndex.end())
    return it->second;
  return defaultShapeData;
}

void SplineGenGeometryShapeManager::bindShapes() const { ShaderGlobal::set_buffer(var::spline_gen_shape_data_buffer, shapeBuf); }
