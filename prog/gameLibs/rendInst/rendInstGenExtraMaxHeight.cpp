// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/riGenExtraMaxHeight.h"
#include "riGen/riGenExtra.h"
#include <shaders/dag_rendInstRes.h>

#include <EASTL/vector_map.h>


// Functions for querying rendInstExtra max height based on material variable id

// When variable id is used for the first time, every rendInstExtra pool is iterated to see
// if any of its lods has a material with this variable set to 1,
// if so, this pool is marked to update max height value on every instance added

static eastl::vector_map<int, float> variableIdToMaxHeight;

static bool ri_extra_max_height_check_pool_variable_set(rendinst::RiExtraPool &pool, int variableId)
{
  for (int i = 0; i < pool.res->lods.size(); ++i)
  {
    ShaderMesh *m = pool.res->lods[i].scene->getMesh()->getMesh()->getMesh();
    dag::Span<ShaderMesh::RElem> elems = m->getElems(m->STG_opaque, m->STG_decal);

    for (ShaderMesh::RElem &elem : elems)
    {
      int value = 0;
      if (elem.mat->getIntVariable(variableId, value) && value > 0)
      {
        return true;
      }
    }
  }
  return false;
}

void ri_extra_max_height_on_ri_resource_added(rendinst::RiExtraPool &pool)
{
  pool.variableIdsToUpdateMaxHeight.clear();
  for (const auto &pair : variableIdToMaxHeight)
  {
    if (ri_extra_max_height_check_pool_variable_set(pool, pair.first))
      pool.variableIdsToUpdateMaxHeight.push_back(pair.first);
  }
}

void ri_extra_max_height_on_ri_added_or_moved(rendinst::RiExtraPool &pool, float ri_height)
{
  for (int variableId : pool.variableIdsToUpdateMaxHeight)
  {
    auto it = variableIdToMaxHeight.find(variableId);
    it->second = max(it->second, ri_height);
  }
}

void ri_extra_max_height_on_terminate() { variableIdToMaxHeight.clear(); }

float ri_extra_max_height_get_max_height(int variableId)
{
  {
    rendinst::ScopedRIExtraReadLock rd;
    auto it = variableIdToMaxHeight.find(variableId);
    if (it != variableIdToMaxHeight.end())
      return it->second;
  }

  rendinst::ScopedRIExtraWriteLock wr;
  auto it = variableIdToMaxHeight.insert(variableId).first;
  for (int id = 0; id < rendinst::riExtra.size(); ++id)
  {
    rendinst::RiExtraPool &pool = rendinst::riExtra[id];
    if (ri_extra_max_height_check_pool_variable_set(pool, variableId))
    {
      pool.variableIdsToUpdateMaxHeight.push_back(variableId);

      // Add contribution of existing instances
      for (const mat43f &tm : pool.riTm)
      {
        mat44f tm44;
        v_mat43_transpose_to_mat44(tm44, tm);
        bbox3f wabb;
        v_bbox3_init(wabb, tm44, pool.lbb);
        it->second = max(it->second, v_extract_y(wabb.bmax));
      }
    }
  }
  return it->second;
}
