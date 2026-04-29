// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "burntGroundNodes.h"
#include "burntGround.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <math/dag_hlsl_floatx.h>
#include <3d/dag_lockSbuffer.h>

#include <burnt_ground/shaders/burnt_ground.hlsli>
#include <memory/dag_framemem.h>
#include <EASTL/memory.h>

struct DecalChain
{
  BurntGroundDecal decal = BurntGroundDecal{Point2(0, 0), 0, 0};
  int prevDecalIndex = -1;
};

dafg::NodeHandle create_burnt_ground_prepare_decals_node(
  uint32_t max_decals, Sbuffer *decals_staging_buffer, Sbuffer *decals_indices_staging_buf)
{
  return get_burnt_ground_namespace().registerNode("prepare_decals", DAFG_PP_NODE_SRC,
    [max_decals, decals_staging_buffer, decals_indices_staging_buf](dafg::Registry registry) {
      const auto bufferFlags = SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED;
      auto decalsBuffer =
        registry.create("burnt_ground_decals_buf")
          .buffer({.elementSize = sizeof(BurntGroundDecal), .elementCount = max_decals, .flags = bufferFlags, .format = 0})
          .atStage(dafg::Stage::TRANSFER)
          .useAs(dafg::Usage::COPY)
          .handle();
      auto decalsIndicesBuffer = registry.create("burnt_ground_decals_indices_buf")
                                   .buffer({.elementSize = sizeof(uint32_t),
                                     .elementCount = BURNT_GROUND_TILE_COUNT * BURNT_GROUND_TILE_COUNT,
                                     .flags = bufferFlags,
                                     .format = 0})
                                   .atStage(dafg::Stage::TRANSFER)
                                   .useAs(dafg::Usage::COPY)
                                   .handle();

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

      struct WorkingMemory
      {
        dag::Vector<int> lastDecalPerTile = dag::Vector<int>(BURNT_GROUND_TILE_COUNT * BURNT_GROUND_TILE_COUNT);
        dag::Vector<DecalChain> decalsChain = {};
      };

      return [workingMemory = eastl::make_unique<WorkingMemory>(), cameraHndl, decals_staging_buffer, decals_indices_staging_buf,
               decalsBuffer, decalsIndicesBuffer, max_decals,
               burnt_ground_landmesh_activeVarId = ::get_shader_variable_id("burnt_ground_landmesh_active")]() {
        const auto &frustum = cameraHndl.ref().jitterFrustum;
        const Point3 eyePos = cameraHndl.ref().cameraWorldPos;

        eastl::fill(workingMemory->lastDecalPerTile.begin(), workingMemory->lastDecalPerTile.end(), -1);
        workingMemory->decalsChain.clear();
        gather_burnt_ground_decals(eyePos, BURNT_GROUND_VISIBILITY_RANGE, [&](const BurntGroundDecalInfo &decal) {
          if (workingMemory->decalsChain.size() == max_decals)
            return;
          if (frustum.testSphere(decal.worldPos, decal.decalSize) == Frustum::OUTSIDE)
            return;
          const IPoint2 tileRangeStart =
            IPoint2(clamp(int((decal.worldPos.x - decal.decalSize - eyePos.x) / BURNT_GROUND_TILE_SIZE) + BURNT_GROUND_TILE_COUNT / 2,
                      0, BURNT_GROUND_TILE_COUNT - 1),
              clamp(int((decal.worldPos.z - decal.decalSize - eyePos.z) / BURNT_GROUND_TILE_SIZE) + BURNT_GROUND_TILE_COUNT / 2, 0,
                BURNT_GROUND_TILE_COUNT - 1));
          const IPoint2 tileRangeEnd =
            IPoint2(clamp(int((decal.worldPos.x + decal.decalSize - eyePos.x) / BURNT_GROUND_TILE_SIZE) + BURNT_GROUND_TILE_COUNT / 2,
                      0, BURNT_GROUND_TILE_COUNT - 1),
              clamp(int((decal.worldPos.z + decal.decalSize - eyePos.z) / BURNT_GROUND_TILE_SIZE) + BURNT_GROUND_TILE_COUNT / 2, 0,
                BURNT_GROUND_TILE_COUNT - 1));
          for (int y = tileRangeStart.y; y <= tileRangeEnd.y; ++y)
          {
            for (int x = tileRangeStart.x; x <= tileRangeEnd.x; ++x)
            {
              int tileIndex = y * BURNT_GROUND_TILE_COUNT + x;
              int nextDecalIndex = workingMemory->decalsChain.size();
              workingMemory->decalsChain.push_back(
                {.decal = {.posXZ = Point2(decal.worldPos.x, decal.worldPos.z), .radius = decal.decalSize, .progress = decal.progress},
                  .prevDecalIndex = workingMemory->lastDecalPerTile[tileIndex]});
              workingMemory->lastDecalPerTile[tileIndex] = nextDecalIndex;
              if (nextDecalIndex + 1 == max_decals)
                return;
            }
          }
        });
        ShaderGlobal::set_int(burnt_ground_landmesh_activeVarId, !workingMemory->decalsChain.empty() ? 1 : 0);
        if (workingMemory->decalsChain.empty())
          return;
        G_ASSERT(workingMemory->decalsChain.size() <= max_decals);
        if (
          auto decals = lock_sbuffer<BurntGroundDecal>(decals_staging_buffer, 0, workingMemory->decalsChain.size(), VBLOCK_WRITEONLY))
        {
          int nextDecalId = 0;
          for (int y = 0; y < BURNT_GROUND_TILE_COUNT; ++y)
          {
            for (int x = 0; x < BURNT_GROUND_TILE_COUNT; ++x)
            {
              int tileIndex = y * BURNT_GROUND_TILE_COUNT + x;
              int lastDecalIndex = workingMemory->lastDecalPerTile[tileIndex];
              int firstDecalId = nextDecalId;
              int decalCount = 0;
              while (lastDecalIndex != -1)
              {
                decals[nextDecalId++] = workingMemory->decalsChain[lastDecalIndex].decal;
                lastDecalIndex = workingMemory->decalsChain[lastDecalIndex].prevDecalIndex;
                decalCount++;
              }
              G_ASSERT(nextDecalId <= workingMemory->decalsChain.size());
              workingMemory->lastDecalPerTile[tileIndex] = (firstDecalId << 16u) | decalCount;
            }
          }
        }
        decals_indices_staging_buf->updateData(0, sizeof(uint32_t) * workingMemory->lastDecalPerTile.size(),
          workingMemory->lastDecalPerTile.data(), VBLOCK_WRITEONLY);


        decals_staging_buffer->copyTo(decalsBuffer.get());
        decals_indices_staging_buf->copyTo(decalsIndicesBuffer.get());
      };
    });
}