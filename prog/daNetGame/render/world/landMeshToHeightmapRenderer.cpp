// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "global_vars.h"
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/dag_cur_view.h>
#include <render/world/wrDispatcher.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/lmeshRenderer.h>
#include <fftWater/fftWater.h>
#include "landMeshToHeightmapRenderer.h"

void render_landmesh_to_heightmap(Texture *heightmap_tex,
  float heightmap_size,
  const Point2 &center_pos,
  const BBox2 *box,
  const float water_level,
  Point4 &world_to_heightmap)
{
  if (!heightmap_tex)
    return;
  int prevBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  {
    SCOPE_VIEW_PROJ_MATRIX;

    auto lmeshMgr = WRDispatcher::getLandMeshManager();
    auto lmeshRenderer = WRDispatcher::getLandMeshRenderer();
    G_ASSERT(lmeshMgr && lmeshRenderer);

    BBox3 levelBox = lmeshMgr->getBBox();
    levelBox.lim[0].x = lmeshMgr->getCellOrigin().x * lmeshMgr->getLandCellSize();
    levelBox.lim[0].z = lmeshMgr->getCellOrigin().y * lmeshMgr->getLandCellSize();
    levelBox.lim[1].x =
      (lmeshMgr->getNumCellsX() + lmeshMgr->getCellOrigin().x) * lmeshMgr->getLandCellSize() - lmeshMgr->getGridCellSize();
    levelBox.lim[1].z =
      (lmeshMgr->getNumCellsY() + lmeshMgr->getCellOrigin().y) * lmeshMgr->getLandCellSize() - lmeshMgr->getGridCellSize();

    Point3 origin =
      heightmap_size < 0 ? levelBox.center() : Point3(floor(center_pos.x / 16 + 0.5) * 16, 0, floor(center_pos.y / 16 + 0.5) * 16);

    heightmap_size = heightmap_size < 0 ? 32768 : heightmap_size;
    if (box)
      levelBox = BBox3(Point3::xVy(box->lim[0], levelBox[0].y), Point3::xVy(box->lim[1], levelBox[1].y));
    else
      levelBox = levelBox.getIntersection(
        BBox3(origin - Point3(heightmap_size, 3000, heightmap_size), origin + Point3(heightmap_size, 3000, heightmap_size)));

    TMatrix4 viewMatrix = ::matrix_look_at_lh(Point3(levelBox.center().x, levelBox[1].y + 1, levelBox.center().z),
      Point3(levelBox.center().x, 0.f, levelBox.center().z), Point3(0.f, 0.f, 1.f));

    TMatrix4 projectionMatrix = matrix_ortho_lh(levelBox.width().x, -levelBox.width().z, 0, levelBox.width().y + 2);
    TMatrix4_vec4 globTm = viewMatrix * projectionMatrix;

    world_to_heightmap.set(1.f / levelBox.width().x, 1.f / levelBox.width().z, -levelBox[0].x / levelBox.width().x,
      -levelBox[0].z / levelBox.width().z);

    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
    shaders::UniqueOverrideStateId depthClipState = shaders::overrides::create(state);

    d3d::settm(TM_VIEW, &viewMatrix);
    d3d::settm(TM_PROJ, &projectionMatrix);
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

    SCOPE_RENDER_TARGET;

    d3d::set_render_target(nullptr, 0);
    d3d::set_depth(heightmap_tex, DepthAccess::RW);
    d3d::clearview(CLEAR_ZBUFFER, 0, 0.f, 0);

    lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_HEIGHTMAP);
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

    float oldInvGeomDist = lmeshRenderer->getInvGeomLodDist();
    lmeshRenderer->setInvGeomLodDist(0.5 / heightmap_size);

    lmeshRenderer->prepare(*lmeshMgr, Point3::xVz(origin, 0), 0, water_level);

    lmeshRenderer->setRenderInBBox(levelBox);
    shaders::overrides::set(depthClipState);
    lmeshRenderer->render(reinterpret_cast<mat44f_cref &>(globTm), projectionMatrix, Frustum{globTm}, *lmeshMgr,
      LandMeshRenderer::RENDER_ONE_SHADER, ::grs_cur_view.pos);
    shaders::overrides::reset();
    lmeshRenderer->setRenderInBBox(BBox3());

    lmeshRenderer->setInvGeomLodDist(oldInvGeomDist);
  }

  // Restore.
  ShaderGlobal::setBlock(prevBlockId, ShaderGlobal::LAYER_FRAME);
}
