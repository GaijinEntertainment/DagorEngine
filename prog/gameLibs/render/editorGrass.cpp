// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/editorGrass.h>
#include <shaders/dag_rendInstRes.h>
#include <math/random/dag_random.h>
#include <math/dag_frustum.h>
#include <3d/dag_render.h>


EditorGrass::EditorGrass(const DataBlock &level_grass_blk, const DataBlock &params_blk) : RandomGrass(level_grass_blk, params_blk)
{
  useSorting = false;
  vboForMaxElements = true;

  landMask = new LandMask(level_grass_blk, GRID_SIZE, true);
};

EditorGrass::~EditorGrass() { del_it(landMask); }

void EditorGrass::beforeRender(const Point3 &center_pos, IRandomGrassRenderHelper &render_helper, const Point3 &view_dir,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const Frustum &frustum, bool force_update, Occlusion *occlusion)
{
  landMask->setWorldSize(cellSize * GRID_SIZE, cellSize);
  landMask->beforeRender(center_pos, render_helper, force_update);

  RandomGrass::beforeRender(center_pos, render_helper, *landMask, occlusion);
  RandomGrass::generateGPUGrass(*landMask, frustum, view_dir, view_tm, proj_tm, center_pos);
}

void EditorGrass::renderDepth() { RandomGrass::renderDepth(*landMask); }

void EditorGrass::renderTrans() { RandomGrass::renderTrans(*landMask); }

void EditorGrass::renderOpaque(bool optimization_prepass_needed)
{
  if (optimization_prepass_needed)
    RandomGrass::renderDepthOptPrepass(*landMask);
  RandomGrass::renderOpaque(*landMask);
}

void EditorGrass::resetLayersVB()
{
  RandomGrass::resetLayersVB();

  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
  {
    GrassLayer &layer = *layers[layerIdx];
    setLayerDensity(layerIdx, layer.info.density);
  }
}


int EditorGrass::getGrassLayersCount() { return layers.size(); }

GrassLayerInfo *EditorGrass::getGrassLayer(int layer_i)
{
  if (layer_i < 0 || layer_i >= layers.size())
    return NULL;

  return &layers[layer_i]->info;
}

int EditorGrass::addDefaultLayer()
{
  GrassLayerInfo layer_info;

  layer_info.density = 1.f;
  layer_info.resName = "grass_scatter_a_3";
  layer_info.bitMask = 1;

  layer_info.crossBlend = 0.f;
  layer_info.radiusMul = 1.f;
  layer_info.windMul = 1.f;

  layer_info.minScale = Point2(0.4f, 0.2f);
  layer_info.maxScale = Point2(1.f, 1.f);

  layer_info.colors[CHANNEL_RED].start = Color4(0.f, 0.f, 0.f, 1.f);
  layer_info.colors[CHANNEL_RED].end = Color4(1.f, 1.f, 1.f, 1.f);

  layer_info.colors[CHANNEL_GREEN].start = Color4(0.f, 0.f, 0.f, 1.f);
  layer_info.colors[CHANNEL_GREEN].end = Color4(0.5f, 0.5f, 0.5f, 1.f);

  layer_info.colors[CHANNEL_BLUE].start = Color4(0.f, 0.f, 0.f, 1.f);
  layer_info.colors[CHANNEL_BLUE].end = Color4(1.f, 1.f, 1.f, 1.f);

  return addLayer(layer_info);
}

bool EditorGrass::removeLayer(int layer_i)
{
  resetLayer(*layers[layer_i]);
  safe_erase_items(layers, layer_i, 1);

  return true;
}


void EditorGrass::removeAllLayers()
{
  resetLayers();
  for (unsigned int layerNo = 0; layerNo < layers.size(); layerNo++)
  {
    GrassLayer *layer = layers[layerNo];
    ::release_game_resource((GameResource *)layer->resource);

    delete layer;
  }
  clear_and_shrink(layers);
}

void EditorGrass::reload(const DataBlock &grass_blk, const DataBlock &params_blk)
{
  removeAllLayers();
  loadParams(grass_blk, params_blk);
}


void EditorGrass::setLayerDensity(int layer_i, float new_density)
{
  GrassLayer *layer = layers[layer_i];
  layer->info.density = new_density;
  const unsigned int lodCount = layer->resource->lods.size();

  for (unsigned int lodIdx = 0; lodIdx < lodCount; ++lodIdx)
  {
    GrassLod &lod = layer->lods[lodIdx];

    const float lodDensityFactor = lerp(1.f, minDensity, float(lodIdx) / float(max(1U, lodCount - 1)));
    lod.density = layer->info.density * lodDensityFactor;

    unsigned int numInstancesInCell = (unsigned int)(baseNumInstances * lod.density / (GRID_SIZE * GRID_SIZE) + 0.5f);

    const ShaderMesh::RElem &elem = lod.mesh->getAllElems()[0];
    numInstancesInCell = min(numInstancesInCell, (unsigned int)MAX_VERTICES_IN_CELL / elem.numv);
    lod.numInstancesInCell = numInstancesInCell;

    if (numInstancesInCell == 0)
      return;

    lod.singleVb = (lod.numInstancesInCell >= INSTANCES_IN_SINGLE_VB);
    lod.numv = lod.numInstancesInCell * elem.numv * (lod.singleVb ? 1 : VB_GRID_SIZE * VB_GRID_SIZE);
    lod.numf = lod.numInstancesInCell * elem.numf;
  }
}

bool EditorGrass::changeLayerResource(int layer_i, const char *resName)
{
  GrassLayer &layer = *layers[layer_i];

  // clear layer
  ::release_game_resource((GameResource *)layer.resource);

  GameResource *res = loadLayerResource(resName);
  layer.resource = (RenderableInstanceLodsResource *)res;

  if (!layer.resource || !layer.resource->isSubOf(RenderableInstanceLodsResourceCID))
    return false;

  resetLayer(layer);

  clear_and_shrink(layer.lods);

  // set new res
  setLayerRes(&layer);
  replaceLayerVdecl(layer);

  // fill layer
  int rndSeed = 20100311;

  layer.info.resetLayerVB = false;
  int lodRndSeed = _rnd(rndSeed);
  for (unsigned int lodIdx = 0; lodIdx < layer.lods.size(); ++lodIdx)
  {
    fillLayerLod(layer, lodIdx, lodRndSeed);
    setLayerDensity(layer_i, layer.info.density);
  }

  maxLodCount = 0;
  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    maxLodCount = max<int>(maxLodCount, layers[layerIdx]->lods.size());

  return true;
}

void EditorGrass::updateLayerVbo(int layerNo)
{
  GrassLayer &layer = *layers[layerNo];

  for (unsigned int lodIdx = 0; lodIdx < layer.lods.size(); ++lodIdx)
  {
    fillLodVB(layer, lodIdx);
  }
}