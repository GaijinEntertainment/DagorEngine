// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_assetThumbnail.h>

#include <3d/dag_texIdSet.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assetsGui/av_thumbnailRenderer.h>
#include <de3_bitMaskMgr.h>
#include <de3_composit.h>
#include <de3_dynModel.h>
#include <de3_dynRenderService.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_prefabs.h>
#include <debug/dag_debug3d.h>
#include <EditorCore/ec_interface.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <image/dag_texPixel.h>
#include <libTools/collision/physMatLegend.h>
#include <math/dag_geomTree.h>
#include <perfMon/dag_cpuFreq.h>
#include <render/debug3dSolid.h>
#include <render/extraRender.h>
#include <rendInst/rendInstConsts.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
#include <util/dag_bitwise_cast.h>
#include <util/dag_globDef.h>
#include <util/dag_hash.h>

#include <EASTL/algorithm.h>
#include <EASTL/array.h>
#include <EASTL/sort.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <imgui/imgui.h>

namespace rendinst
{
extern int maxExtraRiCount;
}

namespace
{

// The asset types that are drawn as dynModel instances, and their services.
const struct DynModelServiceMapping
{
  const char *assetTypeName;
  const char *serviceName;
} DYN_MODEL_SERVICE_MAPPINGS[] = {{"dynModel", "_dmEntMgr"}, {"physObj", "_poEntMgr"}, {"animChar", "_acEntMgr"}};

// A narrow FOV keeps the camera far from the object, so the empty box corners near the camera are not magnified.
const float THUMBNAIL_TAN_HALF_FOV = tanf(DegToRad(20.f));

struct IsolatedRendInst
{
  int resIdx;
  TMatrix tm;
};

struct IsolatedPrefab
{
  const DagorAsset *asset;
  TMatrix tm;
};

// A dynModel, a physObj or an animChar.
struct IsolatedDynModel
{
  const DynModelServiceMapping *serviceMapping;
  IDynModelIsolatedRender *render;
  const DagorAsset *asset;
  TMatrix tm;
};

struct IsolatedParts
{
  dag::Vector<IsolatedRendInst> rendInsts;
  dag::Vector<IsolatedPrefab> prefabs;
  dag::Vector<IsolatedDynModel> dynModels;
};

struct ThumbnailRequest
{
  const DagorAsset *asset = nullptr;
  int64_t waitStartTime = 0;
  int lastRequestFrame = 0;
};

struct WaitingDynModel
{
  const DynModelServiceMapping *serviceMapping; // Not IDynModelIsolatedRender pointer, because a workspace switch deletes the service.
  int assetNameId;                              // Not DagorAsset pointer, because the asset manager deletes a removed asset.
  int lastWaitFrame;
};

struct ThumbnailView
{
  TMatrix cameraTm;
  Driver3dPerspective persp;
};

dag::Vector<ThumbnailRequest> thumbnail_requests;
dag::Vector<WaitingDynModel> waiting_dyn_models; // The isolated entities of the waiting thumbnails.
UniqueBuf instance_tm_buf;

TMatrix make_thumbnail_camera_tm(dag::ConstSpan<Point3> points, const Point3 &center, float z_near)
{
  const Point3 forward = normalize(Point3(1.f, -0.6f, 1.f));
  TMatrix tm;
  tm.setcol(2, forward);
  tm.setcol(0, normalize(Point3(0.f, 1.f, 0.f) % forward));
  tm.setcol(1, forward % tm.getcol(0));

  // A point at camera space (x, y, z) relative to the center is in the region when
  // |x - shift| <= (distance + z) * fill, and the same for y. These limits are linear in shift and distance, so the
  // smallest distance and its shift have a closed form for each axis.
  const float fill = 0.95f * THUMBNAIL_TAN_HALF_FOV;
  float maxPos[2] = {-FLT_MAX, -FLT_MAX}, maxNeg[2] = {-FLT_MAX, -FLT_MAX}, minZ = 0.f;
  for (const Point3 &point : points)
  {
    const Point3 p = point - center;
    const float z = p * forward;
    for (int axis = 0; axis < 2; ++axis)
    {
      const float v = p * tm.getcol(axis) / fill;
      maxPos[axis] = max(maxPos[axis], v - z);
      maxNeg[axis] = max(maxNeg[axis], -v - z);
    }
    minZ = min(minZ, z);
  }

  // The front of the box must stay beyond the near plane.
  float distance = z_near * 2.f - minZ;
  Point3 target = center;
  for (int axis = 0; axis < 2; ++axis)
  {
    distance = max(distance, (maxPos[axis] + maxNeg[axis]) * 0.5f);
    target += tm.getcol(axis) * ((maxPos[axis] - maxNeg[axis]) * 0.5f * fill);
  }

  tm.setcol(3, target - forward * distance);
  return tm;
}

ThumbnailView make_thumbnail_view(dag::ConstSpan<Point3> points)
{
  BBox3 box;
  for (const Point3 &p : points)
    box += p;

  const float zNear = max(length(box.width()) * 0.05f, 0.001f);
  ThumbnailView view;
  view.cameraTm = make_thumbnail_camera_tm(points, box.center(), zNear);
  const float zFar = length(view.cameraTm.getcol(3) - box.center()) + length(box.width());
  view.persp = Driver3dPerspective(1.f / THUMBNAIL_TAN_HALF_FOV, 1.f / THUMBNAIL_TAN_HALF_FOV, zNear, zFar);
  return view;
}

// Returns true when the best LOD that the quality settings allow is loaded, and requests it when it is not.
bool request_best_lod(RenderableInstanceLodsResource &res)
{
  const unsigned bestLod = res.getQlMinAllowedLod();
  if (res.getQlBestLod() <= bestLod)
    return true;
  res.updateReqLod(bestLod);
  return false;
}

IPrefabIsolatedRender *get_prefab_isolated_render()
{
  IEditorService *prefabService = DAEDITOR3.findService("_prefabEntMgr");
  return prefabService ? prefabService->queryInterface<IPrefabIsolatedRender>() : nullptr;
}

IDynModelIsolatedRender *get_dyn_model_isolated_render(const char *service_name)
{
  IEditorService *service = DAEDITOR3.findService(service_name);
  return service ? service->queryInterface<IDynModelIsolatedRender>() : nullptr;
}

// Returns null when the asset type is not drawn as dynModel instances.
const DynModelServiceMapping *find_dyn_model_service_mapping(int asset_type)
{
  static const auto typeIds = [] {
    eastl::array<int, countof(DYN_MODEL_SERVICE_MAPPINGS)> ids;
    for (int i = 0; i < ids.size(); ++i)
      ids[i] = DAEDITOR3.getAssetTypeId(DYN_MODEL_SERVICE_MAPPINGS[i].assetTypeName);
    return ids;
  }();

  for (int i = 0; i < typeIds.size(); ++i)
    if (typeIds[i] == asset_type)
      return &DYN_MODEL_SERVICE_MAPPINGS[i];

  return nullptr;
}

// A placed rendInst gets the hash of its position as per-instance seed, and the parts of a composit get the hash of
// the composit position. Both are the origin here, and Asset Viewer gives a dynModel and a physObj the hash of the
// origin too. A part with ignoreParentInstSeed:b=yes gets the hash of its own position as a rendInst and 0 as a
// dynModel or a physObj, so it can look different. The tint of a rendInst can also differ in a project with
// instSeedStoreZeroForRandom:b=yes, where a rendInst gets 0 instead of the hash of its own position.
int get_part_inst_seed()
{
  const Point3 origin = Point3::ZERO;
  return mem_hash_fnv1(reinterpret_cast<const char *>(&origin), sizeof(origin));
}

// A fastPhys asset has no model. In the viewport, Asset Viewer uses the dynModel of the first animChar with the same skeleton.
const DagorAsset *find_fast_phys_dyn_model(const DagorAsset &fast_phys_asset)
{
  const char *skeletonName = fast_phys_asset.props.getStr("skeleton", "");
  if (!DAEDITOR3.getAssetByName(skeletonName, DAEDITOR3.getAssetTypeId("skeleton")))
    return nullptr;

  const DagorAssetMgr &mgr = fast_phys_asset.getMgr();
  const int animCharTypeId = DAEDITOR3.getAssetTypeId("animChar");
  for (const DagorAsset *asset : mgr.getAssets())
    if (asset->getType() == animCharTypeId && strcmp(asset->props.getStr("skeleton", ""), skeletonName) == 0)
      return DAEDITOR3.getAssetByName(asset->props.getStr("dynModel", ""), DAEDITOR3.getAssetTypeId("dynModel"));

  return nullptr;
}

// Returns false when the isolated render cannot draw the asset.
bool add_isolated_part(const DagorAsset &asset, const TMatrix &tm, IsolatedParts &out)
{
  static const int fastPhysTypeId = DAEDITOR3.getAssetTypeId("fastPhys");
  static const int prefabTypeId = DAEDITOR3.getAssetTypeId("prefab");
  static const int rendInstTypeId = DAEDITOR3.getAssetTypeId("rendInst");

  if (asset.getType() == prefabTypeId)
  {
    out.prefabs.push_back({&asset, tm});
    return true;
  }

  if (asset.getType() == fastPhysTypeId)
  {
    const DagorAsset *dynModelAsset = find_fast_phys_dyn_model(asset);
    return dynModelAsset && add_isolated_part(*dynModelAsset, tm, out);
  }

  if (const DynModelServiceMapping *serviceMapping = find_dyn_model_service_mapping(asset.getType()))
  {
    IDynModelIsolatedRender *render = get_dyn_model_isolated_render(serviceMapping->serviceName);
    if (!render)
      return false;
    out.dynModels.push_back({serviceMapping, render, &asset, tm});
    return true;
  }

  if (asset.getType() != rendInstTypeId || rendinst::maxExtraRiCount <= 0)
    return false;
  const int resIdx = rendinst::addRIGenExtraResIdx(asset.getName(), -1, -1, {});
  if (resIdx < 0)
    return false;
  out.rendInsts.push_back({resIdx, tm});
  return true;
}

// Returns true for a composit part that the thumbnail leaves out: a gameObj (even when its ECS template has an
// animChar, fx or rendInst that the viewport shows), and a loft mask prefab, which daEditorX hides by default ("Show
// ground objects").
bool can_skip_composit_leaf(const DagorAsset &leaf_asset)
{
  static const int gameObjTypeId = DAEDITOR3.getAssetTypeId("gameObj");
  static const int prefabTypeId = DAEDITOR3.getAssetTypeId("prefab");

  if (leaf_asset.getType() == gameObjTypeId)
    return true;

  IPrefabIsolatedRender *prefabRender = leaf_asset.getType() == prefabTypeId ? get_prefab_isolated_render() : nullptr;
  return prefabRender && prefabRender->isLoftMaskPrefab(leaf_asset);
}

// Returns false when the isolated render cannot draw the asset, so it gets no thumbnail.
bool gather_isolated_parts(const DagorAsset &asset, IsolatedParts &out)
{
  static const int compositTypeId = DAEDITOR3.getAssetTypeId("composit");

  if (asset.getType() != compositTypeId)
    return add_isolated_part(asset, TMatrix::IDENT, out);

  IEditorService *compositService = DAEDITOR3.findService("_compEntMgr");
  ICompositLeafEnumerator *enumerator = compositService ? compositService->queryInterface<ICompositLeafEnumerator>() : nullptr;
  if (!enumerator)
    return false;

  return enumerator->enumCompositLeaves(asset, [&](IObjEntity &leaf, const TMatrix &tm) {
    const DagorAsset *leafAsset = DAEDITOR3.getAssetByName(leaf.getObjAssetName(), leaf.getAssetTypeId());
    return leafAsset && (can_skip_composit_leaf(*leafAsset) || add_isolated_part(*leafAsset, tm, out));
  });
}

// Renders the parts alone, so nothing of the scene shows and no instance is added to the scene.
class IsolatedThumbnailRender
{
public:
  // Returns false when a rendInst part has no resource, the prefab service is missing, or no part draws anything.
  // Skips the other parts that cannot be drawn.
  bool prepare(const IsolatedParts &parts)
  {
    if (!prepareRendInsts(parts.rendInsts) || !preparePrefabs(parts.prefabs))
      return false;
    prepareDynModels(parts.dynModels);
    return !rendInstDraws.empty() || !prefabDraws.empty() || !dynModelDraws.empty();
  }

  // Returns false while a resource or a texture of the parts is not loaded.
  bool isLoaded() const
  {
    // A texture that is not built yet builds on a texture manager thread, so this only checks it.
    return resourcesLoaded && prefetch_and_check_managed_textures_loaded(make_span_const(textureIds.data(), textureIds.size()));
  }

  TexPtr render(IDynRenderService &render_service, int size)
  {
    sortRendInstDrawsByLod();

    dag::Vector<Point3> points;
    addRendInstBoundPoints(points);
    addPrefabBoundPoints(points);
    addDynModelBoundPoints(points);

    const ThumbnailView view = make_thumbnail_view(make_span_const(points));
    return render_service.renderIsolated(size, size, view.cameraTm, view.persp, [this](const Point3 &world_offset) {
      renderPrefabs(world_offset);
      renderDynModels(world_offset);
      renderRendInsts(world_offset);
    });
  }

private:
  struct RendInstDraw
  {
    uint16_t resIdx;
    int lod;
    dag::Vector<TMatrix> tms;
  };

  struct PrefabDraw
  {
    const DagorAsset *asset;
    TMatrix tm;
    BBox3 localBox;
  };

  struct DynModelDraw
  {
    IDynModelIsolatedRender *render;
    const DagorAsset *asset;
    TMatrix tm;
    BBox3 localBox;
  };

  bool prepareRendInsts(dag::ConstSpan<IsolatedRendInst> instances)
  {
    for (const IsolatedRendInst &instance : instances)
    {
      auto it =
        eastl::find_if(rendInstDraws.begin(), rendInstDraws.end(), [&](const RendInstDraw &d) { return d.resIdx == instance.resIdx; });
      if (it == rendInstDraws.end())
      {
        RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(instance.resIdx);
        if (!res)
          return false;
        resourcesLoaded = request_best_lod(*res) && resourcesLoaded;

        // The loaded check and the draw use the same LOD, so a LOD that streams
        // in later does not draw textures that isLoaded() did not wait for.
        const int lod = min<int>(res->getQlBestLod(), rendinst::RI_MAX_LODS - 1);
        if (RenderableInstanceResource *scene = res->lods[lod].scene)
          scene->gatherUsedTex(textureIds);

        rendInstDraws.push_back(RendInstDraw{uint16_t(instance.resIdx), lod, {}});
        it = &rendInstDraws.back();
      }
      it->tms.push_back(instance.tm);
    }
    return true;
  }

  // renderRIGenExtraFromBuffer() takes the resources sorted by LOD, with the index of the first one of each LOD.
  void sortRendInstDrawsByLod()
  {
    eastl::sort(rendInstDraws.begin(), rendInstDraws.end(),
      [](const RendInstDraw &a, const RendInstDraw &b) { return a.lod < b.lod; });

    int instanceDataSize = 0;
    for (const RendInstDraw &draw : rendInstDraws)
    {
      for (int lod = draw.lod + 1; lod < rendinst::RI_MAX_LODS; ++lod)
        lodOffsets[lod] = riIndices.size() + 1;
      riIndices.push_back(draw.resIdx);
      // The offset counts float4 vectors, 4 for each instance.
      offsetsAndCounts.push_back(IPoint2(instanceDataSize, draw.tms.size()));
      instanceDataSize += draw.tms.size() * 4;
    }
    lodCount = rendInstDraws.empty() ? 0 : (rendInstDraws.back().lod + 1);
  }

  void addRendInstBoundPoints(dag::Vector<Point3> &points) const
  {
    for (const RendInstDraw &draw : rendInstDraws)
    {
      const BBox3 &resBox = rendinst::getRIGenExtraRes(draw.resIdx)->bbox;
      for (const TMatrix &tm : draw.tms)
        for (int i = 0; i < 8; ++i)
          points.push_back(tm * resBox.point(i));
    }
  }

  void renderRendInstPass(rendinst::RenderPass pass, rendinst::OptimizeDepthPass depth_pass,
    rendinst::OptimizeDepthPrepass depth_prepass) const
  {
    rendinst::render::renderRIGenExtraFromBuffer(instance_tm_buf.getBuf(), make_span_const(offsetsAndCounts),
      make_span_const(riIndices), dag::ConstSpan<uint32_t>(lodOffsets, lodCount), pass, depth_pass, depth_prepass,
      rendinst::IgnoreOptimizationLimits::No, rendinst::LayerFlag::Opaque);
  }

  void renderRendInsts(const Point3 &world_offset) const
  {
    if (rendInstDraws.empty())
      return;

    const int instSeed = get_part_inst_seed();
    dag::Vector<Point4> instanceData;
    for (const RendInstDraw &draw : rendInstDraws)
    {
      // @RiExtraTmLayout: each instance has its 3x4 tm by rows, then the per-draw data offset of its pool, the per-instance
      // data offset and the seed.
      const float perDrawDataOffset = bitwise_cast<float>(rendinst::render::get_per_draw_offset(draw.resIdx));
      for (const TMatrix &tm : draw.tms)
      {
        const Point3 pos = tm.getcol(3) + world_offset;
        for (int row = 0; row < 3; ++row)
          instanceData.push_back(Point4(tm.getcol(0)[row], tm.getcol(1)[row], tm.getcol(2)[row], pos[row]));
        instanceData.push_back(Point4(perDrawDataOffset, 0, bitwise_cast<float>(instSeed), 0));
      }
    }

    if (!instance_tm_buf || instance_tm_buf->getNumElements() < int(instanceData.size()))
    {
      instance_tm_buf.close();
      // Dynamic, so several thumbnails in one frame can discard the data that the GPU still reads.
      instance_tm_buf = dag::create_sbuffer(sizeof(Point4), instanceData.size(),
        SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_A32B32G32R32F, "asset_thumbnail_instance_tm");
      if (!instance_tm_buf)
        return;
    }
    instance_tm_buf->updateData(0, instanceData.size() * sizeof(Point4), instanceData.data(), VBLOCK_DISCARD | VBLOCK_WRITEONLY);

    rendinst::render::ensureElemsRebuiltRIGenExtra(/*gpu_instancing = */ false);

    // With game shaders, the color pass of a rendInst with an impostor or a tree shader has no alpha test and draws
    // only where the depth prepass wrote depth.
    {
      // The depth pass shaders support only this block. Without a scene block, the pass sets rendinst_scene.
      static ShaderBlockIdHolder rendinstDepthSceneBlockId{"rendinst_depth_scene"};
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      renderRendInstPass(rendinst::RenderPass::Depth, rendinst::OptimizeDepthPass::Yes, rendinst::OptimizeDepthPrepass::Yes);
    }
    renderRendInstPass(rendinst::RenderPass::Normal, rendinst::OptimizeDepthPass::No, rendinst::OptimizeDepthPrepass::No);
  }

  bool preparePrefabs(dag::ConstSpan<IsolatedPrefab> parts)
  {
    if (parts.empty())
      return true;

    prefabRender = get_prefab_isolated_render();
    if (!prefabRender)
      return false;

    for (const IsolatedPrefab &prefab : parts)
    {
      BBox3 localBox;
      if (prefabRender->getIsolatedRenderData(*prefab.asset, localBox, textureIds))
        prefabDraws.push_back(PrefabDraw{prefab.asset, prefab.tm, localBox});
    }

    return true;
  }

  void addPrefabBoundPoints(dag::Vector<Point3> &points) const
  {
    for (const PrefabDraw &draw : prefabDraws)
      for (int i = 0; i < 8; ++i)
        points.push_back(draw.tm * draw.localBox.point(i));
  }

  void renderPrefabs(const Point3 &world_offset) const
  {
    for (const PrefabDraw &draw : prefabDraws)
    {
      TMatrix tm = draw.tm;
      tm.setcol(3, tm.getcol(3) + world_offset);
      prefabRender->renderIsolatedOpaque(*draw.asset, tm);
    }
  }

  void prepareDynModels(dag::ConstSpan<IsolatedDynModel> parts)
  {
    for (const IsolatedDynModel &dynModel : parts)
    {
      BBox3 localBox;
      if (dynModel.render->getIsolatedRenderData(*dynModel.asset, localBox, textureIds))
        dynModelDraws.push_back(DynModelDraw{dynModel.render, dynModel.asset, dynModel.tm, localBox});
    }
  }

  void addDynModelBoundPoints(dag::Vector<Point3> &points) const
  {
    for (const DynModelDraw &draw : dynModelDraws)
      for (int i = 0; i < 8; ++i)
        points.push_back(draw.tm * draw.localBox.point(i));
  }

  void renderDynModels(const Point3 &world_offset) const
  {
    if (dynModelDraws.empty())
      return;

    const int instSeed = get_part_inst_seed();
    for (const DynModelDraw &draw : dynModelDraws)
    {
      TMatrix tm = draw.tm;
      tm.setcol(3, tm.getcol(3) + world_offset);
      draw.render->renderIsolatedOpaque(*draw.asset, tm, instSeed);
    }
  }

  TextureIdSet textureIds;
  bool resourcesLoaded = true;

  dag::Vector<RendInstDraw> rendInstDraws;
  dag::Vector<uint16_t> riIndices;
  dag::Vector<IPoint2> offsetsAndCounts;
  uint32_t lodOffsets[rendinst::RI_MAX_LODS] = {};
  int lodCount = 0;

  IPrefabIsolatedRender *prefabRender = nullptr;
  dag::Vector<PrefabDraw> prefabDraws;

  dag::Vector<DynModelDraw> dynModelDraws;
};

// The solid draw takes one color, so a mesh node draws once per material, as in Asset Viewer.
void draw_mesh_node_by_material(const CollisionResource &res, int node_id, const TMatrix &node_tm, const PhysMatLegend &phys_mats,
  DrawSolidMeshCull cull)
{
  const int faceCount = res.getNodeFaceCount(node_id);
  const int vertCount = res.getNodeVertCount(node_id);
  if (faceCount <= 0 || vertCount <= 0 || vertCount > 65536) // the index buffer is uint16
    return;

  dag::Vector<Point3_vec4> verts(vertCount);
  res.iterateNodeVerts(node_id, [&](int vi, vec4f v) { v_st(&verts[vi].x, v); });

  // phys_mats holds the face count of each material, so the range of each material is known before the scan.
  dag::ConstSpan<NodeMaterialFaces> mats = phys_mats.nodeMaterials(node_id);
  dag::Vector<uint32_t> cursors(mats.size());
  uint32_t totalFaces = 0;
  for (int i = 0; i < mats.size(); ++i)
  {
    cursors[i] = totalFaces;
    totalFaces += mats[i].faces;
  }

  // One scan fills the buffer sorted by material.
  dag::Vector<uint16_t> indices(totalFaces * 3);
  res.iterateNodeFacesWithLeafMaterial(node_id, [&](int, uint32_t i0, uint32_t i1, uint32_t i2, int mat) {
    const auto it = eastl::find_if(mats.begin(), mats.end(), [mat](const NodeMaterialFaces &m) { return m.matId == mat; });
    uint32_t &cursor = cursors[it - mats.begin()];
    uint16_t *dst = indices.data() + cursor * 3;
    dst[0] = (uint16_t)i0;
    dst[1] = (uint16_t)i1;
    dst[2] = (uint16_t)i2;
    ++cursor;
  });

  uint32_t start = 0;
  for (const NodeMaterialFaces &m : mats)
  {
    draw_debug_solid_mesh(indices.data() + start * 3, (int)m.faces, &verts[0].x, sizeof(Point3_vec4), vertCount, node_tm,
      phys_mats.colorOf(m.matId), false, cull);
    start += m.faces;
  }
}

// Calls draw() to render into a new texture, with a camera that shows all of the box.
template <typename DrawFn>
TexPtr render_debug_draw_thumbnail(const BBox3 &box, int size, const char *tex_name, DrawFn draw)
{
  Point3 points[8];
  for (int i = 0; i < 8; ++i)
    points[i] = box.point(i);
  const ThumbnailView view = make_thumbnail_view(make_span_const(points, 8));

  TexPtr result = dag::create_tex(nullptr, size, size, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, tex_name);
  UniqueTex depth = dag::create_tex(nullptr, size, size, TEXFMT_DEPTH32 | TEXCF_RTARGET, 1, String(0, "%s_depth", tex_name).c_str());
  if (!result || !depth)
    return {};

  static int worldViewPosVarId = get_shader_variable_id("world_view_pos");

  d3d::GpuAutoLock gpuLock;
  ScopeViewProjMatrix scopeViewProj;
  SCOPE_RENDER_TARGET;
  const Color4 prevWorldViewPos = ShaderGlobal::get_float4(worldViewPosVarId);
  mat44f prevWorldTm;
  d3d::gettm(TM_WORLD, prevWorldTm);

  TMatrix4 projTm;
  d3d::calcproj(view.persp, projTm);
  d3d::settm(TM_VIEW, orthonormalized_inverse(view.cameraTm));
  d3d::settm(TM_PROJ, &projTm);
  d3d::settm(TM_WORLD, TMatrix::IDENT); // globtm includes TM_WORLD, and an earlier render can leave it set.
  ShaderGlobal::set_float4(worldViewPosVarId, Color4::xyz1(view.cameraTm.getcol(3)));

  // Alpha stays 0 where nothing draws, because the debug shaders write alpha 1 for an opaque color.
  d3d::set_render_target({depth.getTex2D(), 0, 0}, DepthAccess::RW, {{result.get(), 0, 0}});
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
  draw();

  d3d::settm(TM_WORLD, prevWorldTm);
  ShaderGlobal::set_float4(worldViewPosVarId, prevWorldViewPos);
  return result;
}

// Draws the nodes as Asset Viewer's solid collision view does. The colors are those of its default "Color by phys
// material" view. The nodes are in their default pose, with no skeleton.
TexPtr render_collision_thumbnail(const CollisionResource &res, int size)
{
  const auto nodes = res.getAllNodes();
  if (nodes.empty() || res.boundingBox.isempty())
    return {};

  PhysMatLegend physMats;
  physMats.build(res);

  return render_debug_draw_thumbnail(res.boundingBox, size, "asset_thumbnail_collision", [&] {
    for (const CollisionNode &node : nodes)
    {
      // Asset Viewer flips the cull mode for mesh nodes. A mirrored placement reverses the winding again.
      TMatrix nodeTm;
      res.getCollisionNodeTm(&node, TMatrix::IDENT, nullptr, nodeTm);
      const DrawSolidMeshCull cull = nodeTm.det() < 0.f ? DrawSolidMeshCull::NORMAL : DrawSolidMeshCull::FLIP;
      if (node.type == COLLISION_NODE_TYPE_MESH)
        draw_mesh_node_by_material(res, node.nodeIndex, nodeTm, physMats, cull);
      else // Asset Viewer gives a primitive or a convex node the color of its first material
        draw_debug_solid_collision_node(node.nodeIndex, res, TMatrix::IDENT, physMats.colorOf(res.getNodePhysMatId(node.nodeIndex, 0)),
          false, cull);
    }
  });
}

TexPtr render_collision_asset_thumbnail(const DagorAsset &asset, int size)
{
  // The game resource is built when it is out of date.
  auto *res = (CollisionResource *)::get_game_resource_ex(asset.getName(), CollisionGameResClassId);
  if (!res)
    return {};
  TexPtr tex = render_collision_thumbnail(*res, size);
  ::release_game_resource_ex((GameResource *)res, CollisionGameResClassId);
  return tex;
}

// Draws the links and the nodes as Asset Viewer's skeleton view does with its default settings, in the default pose.
TexPtr render_skeleton_thumbnail(const GeomNodeTree &shared_tree, int size)
{
  static const E3DCOLOR LINK_COLOR(255, 255, 0);
  static const E3DCOLOR NODE_COLOR(125, 200, 0);

  if (shared_tree.empty())
    return {};

  // A copy, because the game resource is shared and the pose writes its matrices.
  GeomNodeTreeUniquePtr tree(GeomNodeTree::make(shared_tree));
  v_mat44_ident(tree->getRootTm());
  tree->invalidateWtm();
  tree->calcWtm();

  // The same node radius as in Asset Viewer's skeleton view.
  BBox3 importantBox;
  tree->calcWorldBoxFromImportantNodes(importantBox);
  const Point3 width = importantBox.width();
  const float radius = max(min(min(width.x, width.y), width.z), 0.5f) * 0.05f;

  BBox3 box;
  for (dag::Index16 i(0), ie(tree->nodeCount()); i != ie; ++i)
    box += tree->getNodeWposRelScalar(i);
  box.inflate(radius);

  return render_debug_draw_thumbnail(box, size, "asset_thumbnail_skeleton", [&] {
    begin_draw_cached_debug_lines();
    for (dag::Index16 i(0), ie(tree->nodeCount()); i != ie; ++i)
    {
      const Point3 pos = tree->getNodeWposRelScalar(i);
      if (const dag::Index16 parent = tree->getParentNodeIdx(i))
        draw_cached_debug_line(tree->getNodeWposRelScalar(parent), pos, LINK_COLOR);
      draw_cached_debug_sphere(pos, radius, NODE_COLOR);
    }
    end_draw_cached_debug_lines();
  });
}

TexPtr render_skeleton_asset_thumbnail(const DagorAsset &asset, int size)
{
  // The game resource is built when it is out of date.
  auto *tree = (GeomNodeTree *)::get_game_resource_ex(asset.getName(), GeomNodeTreeGameResClassId);
  if (!tree)
    return {};
  TexPtr tex = render_skeleton_thumbnail(*tree, size);
  ::release_game_resource_ex((GameResource *)tree, GeomNodeTreeGameResClassId);
  return tex;
}

// The value of a mask pixel with all bits set, or 0 for a format that the Asset Viewer viewport does not show.
// getMaskPixel1/2/4() return the value in the high bits of a byte.
int get_mask_full_scale(int bpp)
{
  switch (bpp)
  {
    case 1: return 0x80;
    case 2: return 0xC0;
    case 4: return 0xF0;
    case 8: return 0xFF;
    default: return 0;
  }
}

// Shows the mask as an opaque image. size bounds each axis, and the image keeps the aspect ratio of the mask and is not
// larger than the mask.
TexPtr render_tif_mask_thumbnail(const DagorAsset &asset, int size)
{
  // No more than MAX_SAMPLES samples per thumbnail pixel on each axis, so a large mask does not block the Asset Browser for long.
  static constexpr int MAX_SAMPLES = 8;

  IBitMaskImageMgr *bitMaskMgr = EDITORCORE->queryEditorInterface<IBitMaskImageMgr>();
  IBitMaskImageMgr::BitmapMask mask;
  if (!bitMaskMgr->loadImage(mask, nullptr, asset.getTargetFilePath()))
    return {};

  const int w = mask.getWidth(), h = mask.getHeight();
  const int fullScale = get_mask_full_scale(mask.getBitsPerPixel());
  if (fullScale == 0 || w <= 0 || h <= 0)
  {
    bitMaskMgr->destroyImage(mask);
    return {};
  }

  const float scale = min(1.f, float(size) / max(w, h));
  const int thumbW = max(int((w * scale) + 0.5f), 1), thumbH = max(int((h * scale) + 0.5f), 1);
  const double toByte = 255.0 / fullScale;

  eastl::unique_ptr<TexImage32> image(TexImage32::create(thumbW, thumbH));
  TexPixel32 *pixel = image->getPixels();
  for (int ty = 0; ty < thumbH; ++ty)
  {
    const int y0 = ty * h / thumbH;
    const int y1 = (ty + 1) * h / thumbH;
    const int yStep = (y1 - y0 + MAX_SAMPLES - 1) / MAX_SAMPLES;
    for (int tx = 0; tx < thumbW; ++tx, ++pixel)
    {
      const int x0 = (tx * w) / thumbW;
      const int x1 = ((tx + 1) * w) / thumbW;
      const int xStep = (x1 - x0 + MAX_SAMPLES - 1) / MAX_SAMPLES;
      unsigned sum = 0;
      int count = 0;
      for (int y = y0; y < y1; y += yStep)
        for (int x = x0; x < x1; x += xStep, ++count)
          sum += mask.getMaskPixelAnyBpp(x, y);

      // Yellow as in Asset Viewer's viewport.
      pixel->r = pixel->g = uint8_t(min(((double(sum) / count) * toByte) + 0.5, 255.0));
      pixel->b = 0;
      pixel->a = 255;
    }
  }
  bitMaskMgr->destroyImage(mask);

  return dag::create_tex(image.get(), thumbW, thumbH, TEXFMT_A8R8G8B8, 1, "asset_thumbnail_tif_mask");
}

bool can_render_asset_thumbnail(const DagorAsset &asset)
{
  static const int collisionTypeId = DAEDITOR3.getAssetTypeId("collision");
  static const int compositTypeId = DAEDITOR3.getAssetTypeId("composit");
  static const int fastPhysTypeId = DAEDITOR3.getAssetTypeId("fastPhys");
  static const int prefabTypeId = DAEDITOR3.getAssetTypeId("prefab");
  static const int rendInstTypeId = DAEDITOR3.getAssetTypeId("rendInst");
  static const int skeletonTypeId = DAEDITOR3.getAssetTypeId("skeleton");
  static const int tifMaskTypeId = DAEDITOR3.getAssetTypeId("tifMask");

  if (asset.getType() == collisionTypeId)
    return shader_exists("debug_ri");

  if (asset.getType() == skeletonTypeId)
    return true;

  if (asset.getType() == tifMaskTypeId)
    return EDITORCORE->queryEditorInterface<IBitMaskImageMgr>() != nullptr;

  if (asset.getType() != rendInstTypeId && asset.getType() != prefabTypeId && asset.getType() != compositTypeId &&
      asset.getType() != fastPhysTypeId && !find_dyn_model_service_mapping(asset.getType()))
    return false;

  IDynRenderService *renderService = EDITORCORE->queryEditorInterface<IDynRenderService>();
  return renderService && renderService->canRenderIsolated();
}

TEXTUREID register_thumbnail_tex(TexPtr tex)
{
  if (!tex)
    return BAD_TEXTUREID;
  static int thumbnailCounter = 0;
  return register_managed_tex(String(0, "asset_browser_thumbnail_%d", ++thumbnailCounter), eastl::move(tex).release());
}

WaitingDynModel *find_waiting_dyn_model(const IsolatedDynModel &dyn_model)
{
  auto it = eastl::find_if(waiting_dyn_models.begin(), waiting_dyn_models.end(), [&dyn_model](const WaitingDynModel &w) {
    return w.serviceMapping == dyn_model.serviceMapping && w.assetNameId == dyn_model.asset->getNameId();
  });
  return it != waiting_dyn_models.end() ? it : nullptr;
}

void mark_waiting_dyn_models(dag::ConstSpan<IsolatedDynModel> dyn_models)
{
  const int frame = ImGui::GetFrameCount();
  for (const IsolatedDynModel &dynModel : dyn_models)
    if (WaitingDynModel *waiting = find_waiting_dyn_model(dynModel))
      waiting->lastWaitFrame = frame;
    else
      waiting_dyn_models.push_back(WaitingDynModel{dynModel.serviceMapping, dynModel.asset->getNameId(), frame});
}

// The same rule as for the dropped requests.
void release_unused_waiting_dyn_models(int frame)
{
  for (int i = waiting_dyn_models.size() - 1; i >= 0; --i)
    if ((waiting_dyn_models[i].lastWaitFrame + 1) < frame)
    {
      if (IDynModelIsolatedRender *render = get_dyn_model_isolated_render(waiting_dyn_models[i].serviceMapping->serviceName))
        render->releaseIsolatedEntity(waiting_dyn_models[i].assetNameId);
      waiting_dyn_models.erase(waiting_dyn_models.begin() + i);
    }
}

eastl::optional<TEXTUREID> render_asset_thumbnail_from_data(DagorAsset &asset, int size, bool stop_waiting)
{
  if (!can_render_asset_thumbnail(asset))
    return BAD_TEXTUREID;

  if (asset.getType() == DAEDITOR3.getAssetTypeId("collision"))
    return register_thumbnail_tex(render_collision_asset_thumbnail(asset, size));

  if (asset.getType() == DAEDITOR3.getAssetTypeId("skeleton"))
    return register_thumbnail_tex(render_skeleton_asset_thumbnail(asset, size));

  if (asset.getType() == DAEDITOR3.getAssetTypeId("tifMask"))
    return register_thumbnail_tex(render_tif_mask_thumbnail(asset, size));

  IDynRenderService *renderService = EDITORCORE->queryEditorInterface<IDynRenderService>();
  // Asset Browser renders each thumbnail only once. Before the environment light is ready, the side away from the sun
  // is too dark.
  if (!stop_waiting && !renderService->isEnvironmentLightingReady())
    return eastl::nullopt;

  IsolatedParts isolatedParts;
  if (!gather_isolated_parts(asset, isolatedParts))
    return BAD_TEXTUREID;

  IsolatedThumbnailRender thumbnailRender;
  const bool prepared = thumbnailRender.prepare(isolatedParts);
  if (prepared && !stop_waiting && !thumbnailRender.isLoaded())
  {
    mark_waiting_dyn_models(isolatedParts.dynModels);
    return eastl::nullopt;
  }

  const TEXTUREID texId = prepared ? register_thumbnail_tex(thumbnailRender.render(*renderService, size)) : BAD_TEXTUREID;

  // The thumbnail is done, so free the entities that keep the dynModels and their textures loaded.
  for (const IsolatedDynModel &dynModel : isolatedParts.dynModels)
  {
    dynModel.render->releaseIsolatedEntity(dynModel.asset->getNameId());
    if (WaitingDynModel *waiting = find_waiting_dyn_model(dynModel))
      waiting_dyn_models.erase(waiting);
  }

  return texId;
}

ThumbnailRequest &start_thumbnail_request(DagorAsset &asset)
{
  ThumbnailRequest &request = thumbnail_requests.push_back();
  request.asset = &asset;
  request.waitStartTime = ref_time_ticks();
  return request;
}

ThumbnailRequest *find_thumbnail_request(const DagorAsset &asset)
{
  auto it = eastl::find_if(thumbnail_requests.begin(), thumbnail_requests.end(),
    [&asset](const ThumbnailRequest &r) { return r.asset == &asset; });
  return it != thumbnail_requests.end() ? it : nullptr;
}

eastl::optional<TEXTUREID> render_asset_thumbnail(DagorAsset &asset, int size)
{
  static constexpr int DATA_WAIT_USEC = 5'000'000;

  // A request that nobody repeats is dropped, so a later one waits the full time again. Not dagor_frame_no(), because
  // each viewport render increments it.
  const int frame = ImGui::GetFrameCount();
  thumbnail_requests.erase(eastl::remove_if(thumbnail_requests.begin(), thumbnail_requests.end(),
                             [frame](const ThumbnailRequest &r) { return (r.lastRequestFrame + 1) < frame; }),
    thumbnail_requests.end());
  release_unused_waiting_dyn_models(frame);

  ThumbnailRequest *foundRequest = find_thumbnail_request(asset);
  ThumbnailRequest &request = foundRequest ? *foundRequest : start_thumbnail_request(asset);
  request.lastRequestFrame = frame;

  const bool stopWaiting = get_time_usec(request.waitStartTime) > DATA_WAIT_USEC;
  const int64_t renderStartTime = ref_time_ticks();
  eastl::optional<TEXTUREID> texId = render_asset_thumbnail_from_data(asset, size, stopWaiting);

  // The render builds the parts that are not built yet, which can take seconds, and requests the best LODs only after
  // that. So this time does not count as waiting time.
  const int64_t renderTicks = ref_time_ticks() - renderStartTime;
  for (ThumbnailRequest &r : thumbnail_requests)
    r.waitStartTime += renderTicks;

  // A console message in the render can draw the Asset Browser again, which calls this function, so the request is
  // found again.
  if (texId.has_value())
    if (ThumbnailRequest *done = find_thumbnail_request(asset))
      thumbnail_requests.erase(done);
  return texId;
}

} // namespace

class AssetThumbnailRenderer final : public IAssetThumbnailRenderer
{
public:
  bool canRenderThumbnail(const DagorAsset &asset) const override { return can_render_asset_thumbnail(asset); }
  eastl::optional<TEXTUREID> renderThumbnail(DagorAsset &asset, int size) override { return render_asset_thumbnail(asset, size); }
};

IAssetThumbnailRenderer &get_asset_thumbnail_renderer()
{
  static AssetThumbnailRenderer renderer;
  return renderer;
}

void term_asset_thumbnail_rendering()
{
  thumbnail_requests.clear();
  waiting_dyn_models.clear();
  instance_tm_buf.close();

  for (const auto &serviceMapping : DYN_MODEL_SERVICE_MAPPINGS)
    if (IDynModelIsolatedRender *dynModelRender = get_dyn_model_isolated_render(serviceMapping.serviceName))
      dynModelRender->releaseIsolatedRenderData();
}
