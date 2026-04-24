// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityManager.h>

struct SplineGenShapeData;

// Responsible for managing (storing / binding) shape data for SplineGenGeometry entities.
// Shapes basically define the cross-section of the generated spline geometry.
// Since they are the cross-section, each point of the shape is a Point4, where (x,y) is the 2D position of the point in the
// cross-section, and Z is unused, and W is the UV's V coordinate along the shape. Shapes are stored in a vector as well as in a
// structured buffer for shader access. Shape offset (into the buffer) and segment count is stored in a hash map with the shape name as
// key for easy retrieval. Buffer does not shrink. Shapes are only queried when the SplineGenGeometryRepository is created, after that
// none of the data changes.
class SplineGenGeometryShapeManager
{
public:
  void addShape(ecs::List<Point4> &shape, const eastl::string &name);
  IPoint2 getShapeData(const eastl::string &name) const;
  void bindShapes() const;
  void createAndFillBuffer();
  SplineGenGeometryShapeManager();

private:
  int shapeBufElemCount = 0;
  UniqueBuf shapeBuf;

  eastl::vector<SplineGenShapeData> shapeData;
  ska::flat_hash_map<eastl::string, IPoint2> nameToIndex;
  IPoint2 defaultShapeData = {-1, 0};
};