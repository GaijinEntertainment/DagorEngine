// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <memory/dag_framemem.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <util/dag_convar.h>
#include "private_worldRenderer.h"
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <render/deferredRenderer.h>
#include <rendInst/rendInstGen.h>
#include <EASTL/vector.h>
#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_computeShaders.h>
#include <gui/dag_visualLog.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>

CONSOLE_BOOL_VAL("debug", collision_density, false);
CONSOLE_BOOL_VAL("debug", phys_density, false);
CONSOLE_INT_VAL("debug", collision_density_threshold, 1000, 10, 10000);

enum
{
  MAX_COUNT = 128 << 10
};
static int lastThreshold = -1;

static ShaderVariableInfo commonBufferReg{"debug_buffer_reg_no"};

void WorldRenderer::renderDebugCollisionDensity()
{
  if (!collision_density.get() && !phys_density.get())
    return;

  if (!debugCollisionSB || lastThreshold != collision_density_threshold.get() || collision_density.pullValueChange() ||
      phys_density.pullValueChange())
  {
    lastThreshold = collision_density_threshold.get();
    closeDebugCollisionDensity();
    initDebugCollisionDensity();
    fillDebugCollisionDensity(lastThreshold, Point3(2, 2, 2), collision_density.get(), phys_density.get());
  }
  if (debugVoxelsCount)
  {
    d3d::setvsrc(0, 0, 0);
    drawFacesCountElem->setStates(0, true);
    d3d::set_buffer(STAGE_VS, 15, debugCollisionSB.get());
    d3d::draw(PRIM_TRILIST, 0, 12 * debugVoxelsCount);
    d3d::set_buffer(STAGE_VS, 15, 0);
  }
}

enum
{
  FILL_DEBUG_SIZE = 128,
  FILL_DEBUG_SIZE_Y = 128
};

void WorldRenderer::fillDebugCollisionDensity(int threshold, const Point3 &cell, bool raster_collision, bool raster_phys)
{
  struct RiCollision
  {
    uint32_t firstFace = 0, faceCount = 0;
  };
  uint32_t numFaces = 0;
  eastl::vector<RiCollision> collisionInfo(rendinst::getRiGenExtraResCount());
  for (int i = 0, ei = collisionInfo.size(); i < ei; ++i)
  {
    CollisionResource *collRes = rendinst::getRIGenExtraCollRes(i);
    if (!collRes)
      continue;
    RiCollision &c = collisionInfo[i];
    dag::ConstSpan<CollisionNode> nodes = collRes->getAllNodes();
    c.firstFace = numFaces;
    c.faceCount = 0;
    for (const CollisionNode &node : nodes)
      if (!node.indices.empty() && (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX))
      {
        if ((raster_collision && node.checkBehaviorFlags(CollisionNode::TRACEABLE)) ||
            (raster_phys && node.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE)))
          c.faceCount += node.indices.size() / 3;
      }
    numFaces += c.faceCount;
  }
  if (!numFaces)
    return;
  //== end of calc num verts
  struct Triangle
  {
    Point4 v[3];
  };

  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> facesBoxes(d3d::create_sbuffer(sizeof(Triangle), numFaces,
    SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_CPU_ACCESS_WRITE, 0, "tempBuffer"));

  Triangle *tris = 0;
  if (!facesBoxes->lock(0, sizeof(Triangle) * numFaces, (void **)&tris, VBLOCK_WRITEONLY) || !tris)
    return;

  for (int i = 0, ei = collisionInfo.size(); i < ei; ++i)
  {
    if (!collisionInfo[i].faceCount)
      continue;
    CollisionResource *collRes = rendinst::getRIGenExtraCollRes(i);
    dag::ConstSpan<CollisionNode> nodes = collRes->getAllNodes();
    for (const CollisionNode &node : nodes)
      if (!node.indices.empty() && (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX))
      {
        if ((raster_collision && node.checkBehaviorFlags(CollisionNode::TRACEABLE)) ||
            (raster_phys && node.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE)))
        {
          for (int f = 0; f < node.indices.size() / 3; ++f, ++tris)
          {
            tris->v[0] = Point4::xyz0(node.vertices.data()[node.indices[f * 3 + 0]]);
            tris->v[1] = Point4::xyz0(node.vertices.data()[node.indices[f * 3 + 1]]);
            tris->v[2] = Point4::xyz0(node.vertices.data()[node.indices[f * 3 + 2]]);
          }
        }
      }
  }
  facesBoxes->unlock();
  //== end of fill vb/ib

  eastl::unique_ptr<ComputeShaderElement> debug_count_tri_boxes_cs(new_compute_shader("debug_count_tri_boxes_cs"));
  eastl::unique_ptr<ComputeShaderElement> debug_count_tri_cs(new_compute_shader("debug_count_tri_cs"));
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> tempFacesBufCount(
    d3d::buffers::create_ua_sr_structured(sizeof(uint32_t), FILL_DEBUG_SIZE * FILL_DEBUG_SIZE * FILL_DEBUG_SIZE_Y, "tempBuffer"));

  const int bufferReg = commonBufferReg.get_int();

  BBox3 worldBox = getWorldBBox3();
  const IPoint3 res(FILL_DEBUG_SIZE, FILL_DEBUG_SIZE_Y, FILL_DEBUG_SIZE);
  const IPoint3 widthCells = (ipoint3(ceil(div(worldBox.width(), cell))) + res - IPoint3(1, 1, 1)) / FILL_DEBUG_SIZE;
  Point3 fillCell = cell * FILL_DEBUG_SIZE;
  rendinst::riex_collidable_t handles;
  ShaderGlobal::set_color4(get_shader_variable_id("debug_count_tri_cell", true), cell.x, cell.y, cell.z, threshold);
  d3d::zero_rwbufi(debugCollisionCounterSB.get());
  for (int z = 0; z < widthCells.z; z++)
    for (int x = 0; x < widthCells.x; x++)
    {
      Point3 lt = worldBox[0] + mul(Point3(x, 0, z), fillCell);
      BBox3 box(lt, Point3::xVz(lt + fillCell, worldBox[1].y));
      handles.clear();
      rendinst::gatherRIGenExtraCollidable(handles, box, true /*read_lock*/);
      if (!handles.size())
        continue;
      d3d::zero_rwbufi(tempFacesBufCount.get());
      // d3d::set_rwbuffer(STAGE_VS, 4, tempFacesBufCount.get(), false);
      ShaderGlobal::set_color4(get_shader_variable_id("debug_count_tri_start", true), lt.x, lt.y, lt.z, 0);

      d3d::set_rwbuffer(STAGE_CS, 0, tempFacesBufCount.get());
      d3d::set_buffer(STAGE_CS, bufferReg, facesBoxes.get());
      for (auto h : handles)
      {
        const uint32_t type = rendinst::handle_to_ri_type(h);
        if (type >= collisionInfo.size() || collisionInfo[type].faceCount == 0)
          continue;
        // TMatrix nodeTm = coll.tm * node->tm;// I assume it all collapsed anyway
        mat43f instTm = rendinst::getRIGenExtra43(h);
        d3d::set_cs_const(6, (float *)&instTm, 3);
        uint32_t v[4] = {collisionInfo[type].firstFace, collisionInfo[type].faceCount, 0, 0};
        d3d::set_cs_const(9, (float *)&v, 1);
        debug_count_tri_boxes_cs->dispatch((collisionInfo[type].faceCount + 15) / 16, 1, 1);
      }
      // d3d::set_rwbuffer(STAGE_VS, 4, 0, false);

      d3d::set_rwbuffer(STAGE_CS, 0, debugCollisionSB.get());
      d3d::set_rwbuffer(STAGE_CS, 1, debugCollisionCounterSB.get());
      d3d::set_buffer(STAGE_CS, bufferReg, tempFacesBufCount.get());
      debug_count_tri_cs->dispatch(FILL_DEBUG_SIZE / 4, FILL_DEBUG_SIZE / 4, FILL_DEBUG_SIZE_Y / 4);
      d3d::set_rwbuffer(STAGE_CS, 0, 0);
      d3d::set_rwbuffer(STAGE_CS, 1, 0);
      d3d::set_buffer(STAGE_CS, bufferReg, 0);
    }
  uint32_t *debugCollisionCount = nullptr;
  if (debugCollisionCounterSB->lock32(0, sizeof(uint32_t), &debugCollisionCount, VBLOCK_READONLY) && debugCollisionCount)
  {
    debugVoxelsCount = min<int>(MAX_COUNT, *debugCollisionCount);
    debug("debugCollisionCount = %d", *debugCollisionCount);
    visuallog::logmsg(String(0, "There are %d places of size %@ with triangles more than %d", debugVoxelsCount, cell, threshold));
  }
  else
  {
    logerr("Couldn't lock debugCollisionCounterSB");
  }
}

void WorldRenderer::initDebugCollisionDensity()
{
  debugCollisionSB = dag::buffers::create_ua_sr_structured(sizeof(vec4f), MAX_COUNT, "debugFacesTriCount");
  const unsigned int cntBufFlags = SBCF_UA_SR_STRUCTURED | SBCF_INDEX32;
  debugCollisionCounterSB = dag::create_sbuffer(sizeof(uint32_t), 1, cntBufFlags, 0, "debugFacesTriCountNum");
  debugVoxelsCount = 0;
  drawFacesCountMat.reset(new_shader_material_by_name_optional("debug_draw_faces_count"));
  drawFacesCountElem = drawFacesCountMat->make_elem();
}

void WorldRenderer::closeDebugCollisionDensity()
{
  debugCollisionSB.close();
  debugCollisionCounterSB.close();
  debugVoxelsCount = 0;
  drawFacesCountMat.reset();
  drawFacesCountElem = 0;
}
