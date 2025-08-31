// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "consoleHandler.h"
#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"

#include <util/dag_console.h>
#include <EASTL/vector_set.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <gameRes/dag_collisionResource.h>


#if DAGOR_DBGLEVEL > 0
static eastl::vector_set<int> hidden_extra_object_ids;
static eastl::vector_set<int> profiled_extra_object_ids;

bool should_hide_ri_extra_object_with_id(int riex_id)
{
  return hidden_extra_object_ids.find(riex_id) != hidden_extra_object_ids.end();
}

static void dump_collision_triangles_to_csv(eastl::string &csv_dump, const CollisionResource *coll_res)
{
  if (!coll_res)
  {
    csv_dump += ";;";
    return;
  }
  int physTrianges = 0;
  int traceTriangles = 0;
  dag::ConstSpan<CollisionNode> allNodes = coll_res->getAllNodes();
  for (const CollisionNode &cNode : allNodes)
  {
    if (cNode.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      physTrianges += CollisionResource::get_node_tris_count(cNode);
    if (cNode.checkBehaviorFlags(CollisionNode::TRACEABLE))
      traceTriangles += CollisionResource::get_node_tris_count(cNode);
  }
  csv_dump.append_sprintf("%d;%d;", physTrianges, traceTriangles);
}

static bool rendinst_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("rendinst", "hide_object", 1, 2)
  {
    if (argc == 1)
    {
      for (int i = 0; i < rendinst::rgLayer.size(); ++i)
      {
        if (!rendinst::rgLayer[i] || !rendinst::rgLayer[i]->rtData)
          continue;
        rendinst::rgLayer[i]->rtData->hiddenIdx.clear();
      }
      hidden_extra_object_ids.clear();
      console::print_d("All rendinst shown.\nUsage: rendinst.hide_object <rendinst name> (* hides all)");
    }
    else
    {
      const bool all = strcmp(argv[1], "*") == 0;
      for (int i = 0; i < rendinst::rgLayer.size(); ++i)
      {
        if (!rendinst::rgLayer[i] || !rendinst::rgLayer[i]->rtData)
          continue;
        rendinst::rgLayer[i]->rtData->hiddenIdx.clear();
        for (int j = 0; j < rendinst::rgLayer[i]->rtData->riResName.size(); ++j)
          if (all || strstr(rendinst::rgLayer[i]->rtData->riResName[j], argv[1]) == rendinst::rgLayer[i]->rtData->riResName[j])
            rendinst::rgLayer[i]->rtData->hiddenIdx.emplace(j);
      }
      hidden_extra_object_ids.clear();
      iterate_names(rendinst::riExtraMap, [&](int id, const char *name) {
        if (all || strstr(name, argv[1]) == name)
          hidden_extra_object_ids.emplace(id);
      });
      console::print_d("All objects with prefix '%s' are hidden", argv[1]);
    }
  }
  CONSOLE_CHECK_NAME("rendinst", "list", 1, 1)
  {
    iterate_names(rendinst::riExtraMap, [&](int, const char *name) { console::print_d(name); });
    for (int i = 0; i < rendinst::rgLayer.size(); ++i)
      if (rendinst::rgLayer[i] && rendinst::rgLayer[i]->rtData)
        for (int j = 0; j < rendinst::rgLayer[i]->rtData->riResName.size(); ++j)
          if (rendinst::riExtraMap.getNameId(rendinst::rgLayer[i]->rtData->riResName[j]) == -1)
            console::print_d(rendinst::rgLayer[i]->rtData->riResName[j]);
  }
  CONSOLE_CHECK_NAME("rendinst", "profiling", 1, 2)
  {
    if (argc == 1)
    {
      console::print(
        "Usage: rendinst.profiling <rendinst name>|all|none\n"
        "Toggles rendinst in profiles by name, or include all ri in profiler (all), or exclude all rendinst from profiler (none).");
    }
    else
    {
      if (strcmp("all", argv[1]) == 0)
      {
        iterate_names(rendinst::riExtraMap, [&](int id, const char *) { profiled_extra_object_ids.emplace(id); });
        console::print_d("All rendinst are being profiled.");
      }
      else if (strcmp("none", argv[1]) == 0)
      {
        profiled_extra_object_ids.clear();
        console::print_d("Per object rendinst profiling swithed off.");
      }
      else
      {
        iterate_names(rendinst::riExtraMap, [&](int id, const char *name) {
          if (String(name).prefix(argv[1]))
          {
            if (profiled_extra_object_ids.count(id))
              profiled_extra_object_ids.erase(id);
            else
              profiled_extra_object_ids.emplace(id);
          }
        });
      }
    }
  }

  CONSOLE_CHECK_NAME("rendinst", "verify_lods", 1, 2)
  {
    float fov = argc > 1 ? console::to_real(argv[1]) : 90.f;
    float tg = tan(fov / 180.f * PI * 0.5f);

    eastl::string cvs_dump = "name;count on map;bSphere rad;bBox rad;";
    constexpr int MAX_LODS = rendinst::RiExtraPool::MAX_LODS;
    for (uint32_t lod = 0; lod < MAX_LODS; lod++)
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "L%d dips;", lod);
    for (uint32_t lod = 0; lod < MAX_LODS; lod++)
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "L%d tris;", lod);
    cvs_dump += "Phys tris; Trace tris;";
    for (uint32_t lod = 0; lod < MAX_LODS; lod++)
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "L%d dist;", lod);
    for (uint32_t lod = 0; lod < MAX_LODS; lod++)
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "L%d screen;", lod);
    for (uint32_t lod = 0; lod < MAX_LODS; lod++)
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "L%d dist rec;", lod);
    cvs_dump += "L2 heavy shaders;L3 heavy shaders;"
                "\n";

    for (uint32_t i = 0, n = rendinst::riExtra.size(); i < n; i++)
    {
      const rendinst::RiExtraPool &pool = rendinst::riExtra[i];
      if (!pool.res)
        continue;
      const char *riName = rendinst::riExtraMap.getName(i);
      Point3 bboxWidth = pool.res->bbox.width();
      float maxBoxEdge = max(max(bboxWidth.x, bboxWidth.y), bboxWidth.z) * 0.5f; // half of edge like a radius
      float bSphereRad = pool.res->bsphRad;
      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "%s;%d;%.1f;%.1f;", riName, pool.riTm.size(), bSphereRad, maxBoxEdge);

      const uint32_t HEAVY_SHADERS_COUNT = 3;
      // TODO: Move this list to some blk
      const eastl::array<const char *, HEAVY_SHADERS_COUNT> HEAVY_SHADERS = {
        "rendinst_perlin_layered", "rendinst_mask_layered", "rendinst_vcolor_layered"};
      struct LodInfo
      {
        int drawCalls, totalFaces;
        float lodDistance, recomendedDistance, screenPercent;
      };

      eastl::fixed_vector<LodInfo, MAX_LODS> lodsInfo;
      const auto &lods = pool.res->lods;
      float triangleCountFactor = 0.1572f; // coefficient for adjusting the aggressiveness of the LOD shift depending on the number of
                                           // triangles
      float sizeFactor = 1.0f;             // coefficient, controls the importance of object size relative to LOD distance
      int totalFacesL0 = lods[0].scene->getMesh()->getMesh()->getMesh()->calcTotalFaces(); // total faces of LOD 0
      for (uint32_t lod = 0; lod < lods.size(); lod++)
      {
        const auto mesh = lods[lod].scene->getMesh()->getMesh()->getMesh();
        int totalFaces = mesh->calcTotalFaces();
        int drawCalls = mesh->getAllElems().size();
        float lodDistance = lods[lod].range;
        float sizeScale = 2.f / (tg * lodDistance);
        // float spherePartOfScreen = bSphereRad * sizeScale;
        float boxPartOfScreen = maxBoxEdge * sizeScale;
        float recomendedDistance =
          (sizeFactor / maxBoxEdge + triangleCountFactor * (sqrt(totalFacesL0) / log(totalFaces))) / sqrt(sizeScale);
        lodsInfo.emplace_back(LodInfo{drawCalls, totalFaces, lodDistance, recomendedDistance, boxPartOfScreen * 100});
      }
#define DUMP(format, var_name)                  \
  for (uint32_t lod = 0; lod < MAX_LODS; lod++) \
    cvs_dump += lod < lodsInfo.size() ? eastl::string(eastl::string::CtorSprintf{}, format, lodsInfo[lod].var_name) : ";";
      DUMP("%d;", drawCalls)
      DUMP("%d;", totalFaces)
      dump_collision_triangles_to_csv(cvs_dump, pool.collRes);
      DUMP("%.0f;", lodDistance)
      DUMP("%.1f;", screenPercent)
      DUMP("%.0f;", recomendedDistance)
      for (uint32_t lod = 2; lod < lods.size(); lod++)
      {
        eastl::array<uint32_t, HEAVY_SHADERS_COUNT> hasShader = {0, 0, 0};
        const auto mesh = lods[lod].scene->getMesh()->getMesh()->getMesh();
        for (const auto &elem : mesh->getAllElems())
        {
          const char *shaderName = elem.e->getShaderClassName();
          for (uint32_t i = 0; i < HEAVY_SHADERS_COUNT; ++i)
            hasShader[i] += strcmp(shaderName, HEAVY_SHADERS[i]) == 0;
        }
        for (uint32_t i = 0; i < HEAVY_SHADERS_COUNT; ++i)
          if (hasShader[i] > 0)
            cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "%s %d,", HEAVY_SHADERS[i], hasShader[i]);
        cvs_dump += ";";
      }
      cvs_dump += "\n";
    }
    FullFileSaveCB cb("ri_profiling_statistic.csv", DF_WRITE | DF_CREATE);
    cb.write(cvs_dump.c_str(), cvs_dump.size() - 1);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(rendinst_console_handler);
#endif