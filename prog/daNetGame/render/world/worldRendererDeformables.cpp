// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include "private_worldRenderer.h"
void WorldRenderer::setDeformsOrigin(const Point3 &p) { gpuDeformObjectsManager.setOrigin(p); }
void WorldRenderer::createDeforms() { gpuDeformObjectsManager.updateDeforms(); }
void WorldRenderer::closeDeforms() { gpuDeformObjectsManager.close(); }