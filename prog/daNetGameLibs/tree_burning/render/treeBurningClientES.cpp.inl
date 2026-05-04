// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <regExp/regExp.h>
// gameLibs
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/rendInst/riExtra.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <math/dag_mathUtils.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGen.h>
#include <shaders/dag_rendInstRes.h>
#include "net/time.h"
// DNG
#include <render/renderEvent.h>
#include <render/world/shadowsManager.h>
#include <render/world/wrDispatcher.h>
// dngLibs
#include <tree_burning/common/treeBurningCommon.h>
#include <tree_burning/render/treeBurningManager.h>
#include <tree_burning/shaders/tree_burning.hlsli>


ECS_REGISTER_BOXED_TYPE(TreeBurningManager, nullptr);

template <typename Callable>
static void get_tree_burning_manager_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static void add_fire_source_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static void add_tree_chain_burn_source_ecs_query(ecs::EntityManager &manager, Callable);

static Point3 transform_to_tree_local_space(rendinst::riex_handle_t tree_handle, const Point3_vec4 &burning_source_wpos)
{
  mat44f invTreeTm, treeTm;
  rendinst::getRIGenExtra44(tree_handle, treeTm);
  v_mat44_inverse43(invTreeTm, treeTm);
  Point3_vec4 localPos;
  v_st(&localPos.x, v_mat44_mul_vec3p(invTreeTm, v_ld(&burning_source_wpos.x)));
  return localPos;
}

static void add_first_burning_source(TreeBurningManager &mgr, rendinst::riex_handle_t tree_handle, const Point3 &burning_source_wpos)
{
  if (mgr.burningSources.size() >= MAX_TREE_BURNING_SOURCES)
  {
    logerr("Exceeded tree burning sources limit. Burning trees count %d, max source size %f", mgr.burningTreesAdditionalData.size(),
      mgr.burningSources[0].w);
    mgr.burntTrees.emplace(tree_handle);
    rendinst::RiExtraPerInstanceGpuItem gpuAddData = {FULLY_BURNT_TREE, 0, 0, 0};
    rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(tree_handle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
      rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
    return;
  }
  Point4 burningSource = Point4::xyz0(transform_to_tree_local_space(tree_handle, burning_source_wpos));
  uint16_t burningSourceOffset = mgr.burningSources.size();
  mgr.burningSources.push_back(burningSource);
  uint32_t additionalData = (burningSourceOffset << 16) | 1;
  mgr.burningTreesAdditionalData.emplace(tree_handle, additionalData);
  rendinst::RiExtraPerInstanceGpuItem gpuAddData = {additionalData, 0, 0, 0};
  rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(tree_handle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
    rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
}

static void add_additional_burning_source(TreeBurningManager &mgr,
  rendinst::riex_handle_t tree_handle,
  const Point3 &burning_source_wpos,
  float min_distance_from_other_sources)
{
  if (mgr.burningSources.size() >= MAX_TREE_BURNING_SOURCES)
    return;
  auto iter = mgr.burningTreesAdditionalData.find(tree_handle);
  G_ASSERT(iter != mgr.burningTreesAdditionalData.end());
  Point4 newBurningSource = Point4::xyz0(transform_to_tree_local_space(tree_handle, burning_source_wpos));

  uint32_t currentTreeOffsetCount = iter->second;
  int burningSourcesOffset = currentTreeOffsetCount >> 16;
  int burningSourcesCount = currentTreeOffsetCount & 0xffff;
  for (const Point4 &oldBurningSource : make_span_const(mgr.burningSources.data() + burningSourcesOffset, burningSourcesCount))
  {
    // Ignore this burning source if it is too close or inside another one.
    if (lengthSq(Point3::xyz(oldBurningSource - newBurningSource)) <= sqr(oldBurningSource.w + min_distance_from_other_sources))
      return;
  }
  insert_item_at(mgr.burningSources, burningSourcesOffset + burningSourcesCount, newBurningSource);
  for (auto &[riHandle, offsetCount] : mgr.burningTreesAdditionalData)
  {
    if (offsetCount < currentTreeOffsetCount)
      continue;
    if (offsetCount == currentTreeOffsetCount)
      offsetCount += 1; // Increasing burning sources count for current tree
    else if (offsetCount > currentTreeOffsetCount)
      offsetCount += 1 << 16; // Updating offset for other trees
    rendinst::RiExtraPerInstanceGpuItem gpuAddData = {offsetCount, 0, 0, 0};
    rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(riHandle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
      rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
  }
}

ECS_ON_EVENT(on_appear)
static void tree_burning_add_fire_source_es(
  const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, float tree_burning__ignite_radius, const TMatrix &transform)
{
  add_fire_source_ecs_query(manager,
    [&](TreeBurningManager &tree_burning_manager, float tree_burning__additional_source_min_distance) {
      FRAMEMEM_REGION;
      dag::Vector<rendinst::RendInstDesc, framemem_allocator> treeDescs;
      struct ForeachTreeCB final : public rendinst::ForeachCB
      {
        dag::Vector<rendinst::RendInstDesc, framemem_allocator> *descs;
        const TreeBurningManager *mgr;
        virtual void executeForTm(RendInstGenData * /* rgl */, const rendinst::RendInstDesc &desc, const TMatrix & /* tm */)
        {
          if (!desc.isRiExtra())
            return;
          rendinst::riex_handle_t handle = desc.getRiExtraHandle();
          if (mgr->burningTreesAdditionalData.find(handle) != mgr->burningTreesAdditionalData.end())
            descs->push_back(desc);
          else if (mgr->burnableRiExtraTypes.find(rendinst::handle_to_ri_type(handle)) != mgr->burnableRiExtraTypes.end() &&
                   mgr->burntTrees.find(handle) == mgr->burntTrees.end())
            descs->push_back(desc);
        }

        virtual void executeForPos(RendInstGenData * /* rgl */, const rendinst::RendInstDesc &desc, const TMatrix & /* tm */)
        {
          descs->push_back(desc);
        }
      } cb;
      cb.descs = &treeDescs;
      cb.mgr = &tree_burning_manager;
      BSphere3 sph(transform.col[3], tree_burning__ignite_radius);
      rendinst::foreachRIGenInSphere(sph, rendinst::GatherRiTypeFlag::RiGenAndExtra | rendinst::GatherRiTypeFlag::RiGenCanopy, cb);

      // We have no API to gather not collidable RI in sphere, so we will use inscribed box for that. It should be enough
      // because RI intersection is checked by bounding sphere, which usually larger than visual geometry itself.
      // There is also no way to get riex_handle_t from tiled scene nodes, so at first I collect only pools, and then collect
      // nearby instances from found pools.
      bbox3f gatherBbox;
      v_bbox3_init_by_bsph(gatherBbox, v_ldu(transform.m[3]), v_splats(tree_burning__ignite_radius / sqrtf(3)));
      dag::VectorSet<uint32_t, eastl::less<uint32_t>, framemem_allocator> burnedRiExtraPools;
      rendinst::gatherRIGenExtraRenderableNotCollidable(gatherBbox, false, rendinst::SceneSelection::All,
        [&](scene::node_index, mat44f_cref m) {
          uint32_t poolId = scene::get_node_pool(m);
          if (tree_burning_manager.burnableRiExtraTypes.find(poolId) != tree_burning_manager.burnableRiExtraTypes.end())
            burnedRiExtraPools.insert(poolId);
        });
      Tab<rendinst::riex_handle_t> nonCollidableHandles;
      for (uint32_t poolId : burnedRiExtraPools)
      {
        rendinst::getRiGenExtraInstances(nonCollidableHandles, poolId, gatherBbox);
        for (rendinst::riex_handle_t h : nonCollidableHandles)
          if (tree_burning_manager.burntTrees.find(h) == tree_burning_manager.burntTrees.end())
            treeDescs.push_back(rendinst::RendInstDesc(h));
      }

      for (const rendinst::RendInstDesc &treeDesc : treeDescs)
      {
        if (!treeDesc.isRiExtra())
        {
          G_ASSERT(rendinst::isRIGenPosInst(treeDesc));
          BBox3 canopyBox;
          rendinst::getRIGenCanopyBBox(treeDesc, TMatrix::IDENT, canopyBox);
          TMatrix tm = rendinst::getRIGenMatrix(treeDesc);
          int canopyType = getRIGenCanopyShape(treeDesc);
          // NOTE: shadow will be slightly different because riExtra don't have impostors.
          rendinst::riex_handle_t riExHandle = rendinst::replaceRIGenWithRIExtra(treeDesc);
          if (riExHandle == rendinst::RIEX_HANDLE_NULL)
            continue;

          g_entity_mgr->sendEvent(eid,
            EventCreateTreeBurningChain(riExHandle, tm, canopyType, canopyBox.lim[0], canopyBox.lim[1], transform.col[3]));
          add_first_burning_source(tree_burning_manager, riExHandle, transform.col[3]);
        }
        else // riExtra case
        {
          rendinst::riex_handle_t riExHandle = treeDesc.getRiExtraHandle();
          bool isBurning =
            tree_burning_manager.burningTreesAdditionalData.find(riExHandle) != tree_burning_manager.burningTreesAdditionalData.end();
          if (isBurning)
          {
            add_tree_chain_burn_source_ecs_query(manager,
              [&](ecs::EntityId eid, rendinst::riex_handle_t tree_burning_chain__treeHandle) {
                if (tree_burning_chain__treeHandle == riExHandle)
                  g_entity_mgr->sendEvent(eid, EventAddTreeBurningChainSource(transform.col[3]));
              });
            add_additional_burning_source(tree_burning_manager, riExHandle, transform.col[3],
              tree_burning__additional_source_min_distance);
          }
          else
            add_first_burning_source(tree_burning_manager, riExHandle, transform.col[3]);
        }
      }
    });
}

static bool is_tree_fully_burnt(rendinst::riex_handle_t tree_handle, dag::ConstSpan<Point4> burning_sources)
{
  vec4f treeBsphere = rendinst::getRIGenExtraPoolBSphere(rendinst::handle_to_ri_type(tree_handle));
  for (const Point4 &burningSource : burning_sources)
  {
    vec4f posDiff_radiusDiff = v_sub(v_ldu(&burningSource.x), treeBsphere);
    if (v_test_vec_x_le(v_length3_x(posDiff_radiusDiff), v_perm_wwww(posDiff_radiusDiff)))
      return true;
  }
  return false;
}

static void remove_burning_sources_if_unused(TreeBurningManager &mgr, uint32_t remove_offset_count)
{
  if (eastl::find(mgr.burningTreesAdditionalData.begin(), mgr.burningTreesAdditionalData.end(), remove_offset_count,
        [](eastl::pair<rendinst::riex_handle_t, uint32_t> p, uint32_t offset_count) { return p.second == offset_count; }) !=
      mgr.burningTreesAdditionalData.end())
    return;
  uint32_t offsetDelta = remove_offset_count << 16;
  for (auto &[riHandle, offsetCount] : mgr.burningTreesAdditionalData)
    if (offsetCount > remove_offset_count)
    {
      offsetCount -= offsetDelta;
      rendinst::RiExtraPerInstanceGpuItem gpuAddData = {offsetCount, 0, 0, 0};
      rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(riHandle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
        rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
    }
  int offset = remove_offset_count >> 16;
  int count = remove_offset_count & 0xffff;
  erase_items(mgr.burningSources, offset, count);
  if (mgr.burningSources.empty() && mgr.burningSourcesConstBuffer.getVarId() != -1)
    ShaderGlobal::set_buffer(mgr.burningSourcesConstBuffer.getVarId(), BAD_D3DRESID);
}

static void invalidate_tree_shadow(rendinst::riex_handle_t tree_handle)
{
  bbox3f localBBox = rendinst::riex_get_lbb(rendinst::handle_to_ri_type(tree_handle));
  mat44f treeTm;
  rendinst::getRIGenExtra44(tree_handle, treeTm);
  bbox3f worldBBox;
  v_bbox3_init(worldBBox, treeTm, localBBox);
  BBox3 bbox;
  v_stu_bbox3(bbox, worldBBox);
  WRDispatcher::getShadowsManager().shadowsInvalidate(bbox);
}

ECS_TAG(render)
static void tree_burning_update_es(const UpdateStageInfoBeforeRender &stg,
  TreeBurningManager &tree_burning_manager,
  float tree_burning__fire_spread_speed,
  float tree_burning__time_to_invalidate_shadows)
{
  if (tree_burning_manager.burningSources.empty())
    return;

  tree_burning_manager.staticShadowsInvalidationTimer += stg.dt;
  bool invalidateShadow = false;
  if (tree_burning_manager.staticShadowsInvalidationTimer >= tree_burning__time_to_invalidate_shadows)
  {
    invalidateShadow = true;
    tree_burning_manager.staticShadowsInvalidationTimer = 0;
  }
  for (int i = 0, ie = tree_burning_manager.burningSources.size(); i < ie; ++i)
  {
    tree_burning_manager.burningSources[i].w += stg.dt * tree_burning__fire_spread_speed;
  }
  for (auto iter = tree_burning_manager.burningTreesAdditionalData.begin();
       iter != tree_burning_manager.burningTreesAdditionalData.end();)
  {
    rendinst::riex_handle_t treeHandle = iter->first;
    uint32_t offsetCount = iter->second;
    Point4 *burningSourcesFrom = tree_burning_manager.burningSources.data() + (offsetCount >> 16);
    int burningSourcesCount = offsetCount & 0xffff;
    dag::ConstSpan<Point4> burningSources = make_span_const(burningSourcesFrom, burningSourcesCount);
    if (is_tree_fully_burnt(treeHandle, burningSources))
    {
      iter = tree_burning_manager.burningTreesAdditionalData.erase(iter);
      remove_burning_sources_if_unused(tree_burning_manager, offsetCount);
      int poolId = rendinst::handle_to_ri_type(treeHandle);
      invalidate_tree_shadow(treeHandle);
      if (tree_burning_manager.leavesOnlyRiExtra.find(poolId) == tree_burning_manager.leavesOnlyRiExtra.end())
      {
        tree_burning_manager.burntTrees.emplace(treeHandle);
        rendinst::RiExtraPerInstanceGpuItem gpuAddData = {FULLY_BURNT_TREE, 0, 0, 0};
        rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(treeHandle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
          rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
      }
      else
      {
        // This riExtra (bush without branches, ivy and etc) will be invisible now, so better to just delete it.
        rendinst::delRIGenExtra(treeHandle);
      }
    }
    else
    {
      if (invalidateShadow)
        invalidate_tree_shadow(iter->first);
      ++iter;
    }
  }

  if (tree_burning_manager.burningSources.size())
  {
    if (!tree_burning_manager.burningSourcesConstBuffer)
    {
      tree_burning_manager.burningSourcesConstBuffer =
        dag::buffers::create_one_frame_cb(MAX_TREE_BURNING_SOURCES, "tree_burning_sources");
    }
    tree_burning_manager.burningSourcesConstBuffer->updateData(0, tree_burning_manager.burningSources.size() * sizeof(Point4),
      tree_burning_manager.burningSources.data(), VBLOCK_DISCARD);
    tree_burning_manager.burningSourcesConstBuffer.setVar();
  }
}

static void on_burned_tree_destr_created_cb(const rendinst::RendInstDesc &old_desc, rendinst::riex_handle_t tree_destr_riex_handle)
{
  if (!old_desc.isRiExtra())
    return;
  rendinst::riex_handle_t treeHandle = old_desc.getRiExtraHandle();
  get_tree_burning_manager_ecs_query(*g_entity_mgr, [&](TreeBurningManager &tree_burning_manager) {
    if (auto burningTreeIter = tree_burning_manager.burningTreesAdditionalData.find(treeHandle);
        burningTreeIter != tree_burning_manager.burningTreesAdditionalData.end())
    {
      rendinst::RiExtraPerInstanceGpuItem gpuAddData = {burningTreeIter->second, 0, 0, 0};
      rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(tree_destr_riex_handle,
        rendinst::RiExtraPerInstanceDataType::TREE_BURNING, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
      tree_burning_manager.burningTreesAdditionalData.emplace(tree_destr_riex_handle, burningTreeIter->second);
    }
    if (auto burntTreeIter = tree_burning_manager.burntTrees.find(treeHandle); burntTreeIter != tree_burning_manager.burntTrees.end())
    {
      rendinst::RiExtraPerInstanceGpuItem gpuAddData = {FULLY_BURNT_TREE, 0, 0, 0};
      rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(tree_destr_riex_handle,
        rendinst::RiExtraPerInstanceDataType::TREE_BURNING, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
      tree_burning_manager.burntTrees.emplace(tree_destr_riex_handle);
    }
  });
}

static void invalidate_riex_handle_cb(rendinst::riex_handle_t handle)
{
  get_tree_burning_manager_ecs_query(*g_entity_mgr, [handle](TreeBurningManager &tree_burning_manager) {
    if (auto burningTreeIter = tree_burning_manager.burningTreesAdditionalData.find(handle);
        burningTreeIter != tree_burning_manager.burningTreesAdditionalData.end())
    {
      uint32_t offsetCount = burningTreeIter->second;
      tree_burning_manager.burningTreesAdditionalData.erase(burningTreeIter);
      remove_burning_sources_if_unused(tree_burning_manager, offsetCount);
      return;
    }
    if (auto burntTreeIter = tree_burning_manager.burntTrees.find(handle); burntTreeIter != tree_burning_manager.burntTrees.end())
      tree_burning_manager.burntTrees.erase(burntTreeIter);
  });
}

ECS_TAG(render)
static void register_tree_burning_callbacks_on_level_loaded_es(const OnLevelLoaded &)
{
  rendinstdestr::set_on_tree_destr_created_cb(on_burned_tree_destr_created_cb);
  rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_riex_handle_cb);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, EventRendinstsLoaded)
static void init_burnable_ri_extra_es(const ecs::Event &,
  const ecs::StringList &tree_burning__burnableRiExtra,
  const ecs::StringList &tree_burning__leavesShaders,
  TreeBurningManager &tree_burning_manager)
{
  if (!rendinst::isRiExtraLoaded())
    return;
  for (const ecs::string &riExtraName : tree_burning__burnableRiExtra)
  {
    if (riExtraName.find('*') == ecs::string::npos)
    {
      int riExtraResIdx = rendinst::getRIGenExtraResIdx(riExtraName.c_str());
      if (riExtraResIdx >= 0)
        tree_burning_manager.burnableRiExtraTypes.insert(riExtraResIdx);
      // Need to ignore missing riExtra because they can be present only on specific levels
    }
    else
    {
      RegExp re;
      if (!re.compile(riExtraName.c_str()))
      {
        logerr("Bad regexp '%s' in tree_burning__burnableRiExtra", riExtraName.c_str());
        continue;
      }
      rendinst::iterateRIExtraMap([&](int res_idx, const char *name) {
        if (re.test(name))
          tree_burning_manager.burnableRiExtraTypes.insert(res_idx);
      });
    }
  }

  static int atestVariableId = VariableMap::getVariableId("atest");
  auto riResHasOnlyLeaves = [&](const RenderableInstanceLodsResource *res) {
    G_ASSERT_RETURN(res && !res->lods.empty(), false);
    dag::Span<ShaderMesh::RElem> elems =
      res->lods[0].scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::Stage::STG_opaque, ShaderMesh::Stage::STG_decal);
    for (const ShaderMesh::RElem &elem : elems)
    {
      int atest = 0;
      if (!elem.mat->getIntVariable(atestVariableId, atest) || atest <= 0)
        return false;
      bool isLeavesShader = false;
      for (const ecs::string &leavesShader : tree_burning__leavesShaders)
      {
        if (leavesShader == elem.mat->getShaderClassName())
        {
          isLeavesShader = true;
          break;
        }
      }
      if (!isLeavesShader)
        return false;
    }
    return true;
  };

  G_ASSERT(tree_burning_manager.leavesOnlyRiExtra.empty());
  for (uint32_t poolId = 0, n = rendinst::getRIGenExtraPoolCount(); poolId < n; ++poolId)
  {
    if (rendinst::isRIExtraGenPosInst(poolId) && riResHasOnlyLeaves(rendinst::getRIGenExtraRes(poolId)))
      tree_burning_manager.leavesOnlyRiExtra.push_back_unsorted(poolId);
  }
  for (uint32_t poolId : tree_burning_manager.burnableRiExtraTypes)
  {
    if (riResHasOnlyLeaves(rendinst::getRIGenExtraRes(poolId)))
      tree_burning_manager.leavesOnlyRiExtra.insert(poolId);
  }
}

void apply_tree_burning_data_for_render(dag::ConstSpan<rendinst::riex_handle_t> burnt_trees)
{
  G_ASSERT(g_entity_mgr->getSingletonEntity(ECS_HASH("tree_burning_manager")));
  get_tree_burning_manager_ecs_query(*g_entity_mgr, [&](TreeBurningManager &tree_burning_manager) {
    for (rendinst::riex_handle_t treeHandle : burnt_trees)
    {
      rendinst::RiExtraPerInstanceGpuItem gpuAddData = {FULLY_BURNT_TREE, 0, 0, 0};
      rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(treeHandle, rendinst::RiExtraPerInstanceDataType::TREE_BURNING,
        rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &gpuAddData, 1);
      tree_burning_manager.burntTrees.emplace(treeHandle);
    }
  });
}
