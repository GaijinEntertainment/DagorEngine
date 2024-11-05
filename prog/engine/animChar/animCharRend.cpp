// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animChar/dag_animCharacter2.h>
#include <generic/dag_tab.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_dynSceneRes.h>
#include <3d/dag_render.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomTreeMap.h>
#include <drv/3d/dag_driver.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <animChar/dag_animCharVis.h>

#include <math/dag_frustum.h>
#include <math/dag_mathUtils.h>
#include <scene/dag_occlusion.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_bitArray.h>

using namespace AnimV20;

#define debug(...) logmessage(_MAKE4C('ANIM'), __VA_ARGS__)

// global TM data
namespace AnimCharV20
{
static struct RenderContext
{
  Frustum frustum;
  bool isMainCamera = true; // todo: remove
  Occlusion *occlusion = NULL;
  unsigned prepared_in_frame = 0;
  Point3 viewPos;
} context;
} // namespace AnimCharV20
float AnimV20::AnimcharRendComponent::invWkSq = -1;

void AnimCharV20::prepareGlobTm(bool is_main_camera, const Frustum &culling_frustum, float hk, const Point3 &viewPos,
  Occlusion *occlusion)
{
  G_UNUSED(AnimCharV20::context.prepared_in_frame);
#if DAGOR_DBGLEVEL > 0
  AnimCharV20::context.prepared_in_frame = dagor_frame_no();
#endif
  AnimCharV20::context.frustum = culling_frustum;
  AnimCharV20::context.isMainCamera = is_main_camera;
  AnimCharV20::context.viewPos = viewPos;

  if (hk > 0)
  {
    float wkSq = (hk * 0.75f) * (hk * 0.75f);
    AnimcharRendComponent::invWkSq = wkSq > 1e-6f ? 1.f / wkSq : 1e6f;
    AnimCharV20::context.occlusion = occlusion;
  }
  else
    AnimcharRendComponent::invWkSq = -1;
}

void AnimCharV20::prepareFrustum(bool is_main_camera, const Frustum &culling_frustum, const Point3 &viewPos, Occlusion *occlusion)
{
#if DAGOR_DBGLEVEL > 0
  AnimCharV20::context.prepared_in_frame = dagor_frame_no();
#endif
  AnimCharV20::context.isMainCamera = is_main_camera;
  AnimCharV20::context.frustum = culling_frustum;
  AnimCharV20::context.viewPos = viewPos;
  AnimCharV20::context.occlusion = occlusion;
}

//
// Animchar render component
//
AnimcharRendComponent::AnimcharRendComponent() :
  lodres(NULL),
  scene(NULL),
  sceneOwned(true),
  visBits(VISFLG_RENDERABLE),
  sceneBeforeRenderedCalled(false),
  magic(MAGIC_VALUE),
  noAnimDist2(0),
  noRenderDistSq(0),
  bsphRadExpand(0)
{
  ownerTm.row0 = V_C_UNIT_1000;
  ownerTm.row1 = V_C_UNIT_0100;
  ownerTm.row2 = V_C_UNIT_0010;
}

AnimcharRendComponent::~AnimcharRendComponent()
{
  if (sceneOwned)
    del_it(scene);
  lodres = NULL;
}

bool AnimcharRendComponent::calcWorldBox(bbox3f &out_wbb, const AnimcharFinalMat44 &finalWtm, bool accurate) const
{
  vec4f localExtents;
  if (lodres)
  {
    BBox3 localBox = lodres->getLocalBoundingBox();
    bbox3f lbox = v_ldu_bbox3(localBox);
    bbox3f localBounds;
    v_bbox3_init(localBounds, finalWtm.nwtm[0], lbox); // store model (root) transformed local bounds
    localExtents = v_mul(v_sub(localBounds.bmax, localBounds.bmin), V_C_HALF);

    // store offset without extents (to expand them by node offsets)
    out_wbb.bmin = out_wbb.bmax = v_add(localBounds.bmin, localExtents);
  }
  else
  {
    // no local bounds
    v_bbox3_init_empty(out_wbb);
    localExtents = v_zero();
  }

  if (!accurate)
  {
    // When adding extents before adding other points, it makes bbox smaller,
    // but there can be parts outside box (but rarely and not much).
    out_wbb.bmin = v_sub(out_wbb.bmin, localExtents);
    out_wbb.bmax = v_add(out_wbb.bmax, localExtents);
  }

  // expand by node offsets (approximation instead of heavy bbox mtx transforms)
  for (int i = 0; i < nodeMap.size(); i++)
    if (nodeMap[i].nodeIdx != dag::Index16(0))
      v_bbox3_add_pt(out_wbb, finalWtm.nwtm[nodeMap[i].nodeIdx.index()].col3);
  // apply world offset
  out_wbb.bmin = v_add(out_wbb.bmin, finalWtm.wofs);
  out_wbb.bmax = v_add(out_wbb.bmax, finalWtm.wofs);

  if (accurate)
  {
    // Apply original extents (from original bounds) AFTER expanding by node offsets.
    // It prevents geometry to be outside bbox in common situations.
    out_wbb.bmin = v_sub(out_wbb.bmin, localExtents);
    out_wbb.bmax = v_add(out_wbb.bmax, localExtents);
  }

  return true;
}

real AnimcharRendComponent::getSqDistanceToViewerLegacy(const mat44f &root_wtm, const Point3 &view_pos) const
{
  vec4f rootPos = v_mat43_mul_vec3p(ownerTm, root_wtm.col3);
  return v_extract_x(v_length3_sq_x(v_sub(v_ldu(&view_pos.x), rootPos)));
}

vec4f AnimcharRendComponent::prepareSphere(const AnimcharFinalMat44 &finalWtm) const
{
  vec3f max_dist2 = v_zero();
  vec3f c = v_sub(finalWtm.bsph, finalWtm.wofs);
  for (int i = 0; i < nodeMap.size(); i++)
    if (nodeMap[i].nodeIdx != dag::Index16(0))
      max_dist2 = v_max(max_dist2, v_length3_sq(v_sub(c, finalWtm.nwtm[nodeMap[i].nodeIdx.index()].col3)));
  return v_perm_xyzd(finalWtm.bsph, v_max(finalWtm.bsph, v_add(v_sqrt(max_dist2), v_splats(bsphRadExpand))));
}

uint16_t AnimcharRendComponent::beforeRender(const AnimcharFinalMat44 &finalWtm, vec4f &out_rendBsph, bbox3f &out_rendBbox,
  const Frustum &f, float inv_wk_sq, Occlusion *occl, const Point3 &cam_pos, float lodDistMul)
{
  sceneBeforeRenderedCalled = false;
  out_rendBsph = finalWtm.bsph;
  visBits &= VISFLG_RENDERABLE;
  // DEBUG_CTX("beforeRender (vis=%d)", visBits);
  if (!visBits || !scene)
    return visBits;

  vec4f rootPos = v_mat43_mul_vec3p(ownerTm, finalWtm.nwtm[0].col3);
  const float dist2 = v_extract_x(v_length3_sq_x(v_sub(v_ldu(&cam_pos.x), rootPos)));
  if (dist2 < 0.f)
    return visBits;

  float scaledDist2 = dist2 * inv_wk_sq;
  if (scaledDist2 > noRenderDistSq)
    return visBits;
  visBits |= VISFLG_IN_RANGE;

  // todo: we should not actually check visibility or prepare in beforeRender at all
  {
    out_rendBsph = prepareSphere(finalWtm);
    calcWorldBox(out_rendBbox, finalWtm);

    if (f.testSphereB(out_rendBsph, v_splat_w(out_rendBsph)))
    {
      visBits |= VISFLG_BSPH;
      if (occl)
      {
        if (occl->isVisibleBox(out_rendBbox.bmin, out_rendBbox.bmax))
          visBits |= VISFLG_MAIN;
      }
      else
      {
        if (f.testBoxB(out_rendBbox.bmin, out_rendBbox.bmax))
          visBits |= VISFLG_MAIN;
      }
    }
  }

  if (visBits & VISFLG_BSPH) //== trivial heuristic to render shadows when big bsphere in visible
    visBits |= VISFLG_SHADOW;

  // todo: we should not actually prepare in beforeRender at all, only do that for each render.
  {
    prepareForRender(finalWtm, out_rendBbox);
    if (!nodeMap.size())
      scene->chooseLod(cam_pos, lodDistMul);
    else
    {
      vec3f cam_to_obj = v_sub(v_sub(v_ldu(&cam_pos.x), v_ldu(&scene->getOrigin().x)), out_rendBsph);
      scene->chooseLodByDistSq(v_extract_x(v_length3_sq_x(cam_to_obj)), lodDistMul);
    }
    scene->setDisableAutoChooseLod(true);
  }

  // todo: we should not actually prepare at all, only do that for each render.
  // DEBUG_CTX("  visBits=%02X, d2=%.3f, noRenderDist=%.3f noAnimDist2=%.3f", visBits, dist2, noRenderDist, noAnimDist2);
  return visBits;
}
uint16_t AnimcharRendComponent::beforeRenderLegacy(const AnimcharFinalMat44 &finalWtm, vec4f &out_rendBsph, bbox3f &out_rendBbox,
  float lodDistMul = 1.0f)
{
  // G_ASSERTF(AnimCharV20::prepared_in_frame == dagor_frame_no(),
  //   "AnimCharV20::prepareGlobTm() or AnimCharV20::prepareFrustum() last called on frame %d, but current is %d",
  //   AnimCharV20::prepared_in_frame, dagor_frame_no());
  return beforeRender(finalWtm, out_rendBsph, out_rendBbox, AnimCharV20::context.frustum, AnimcharRendComponent::invWkSq,
    AnimCharV20::context.occlusion, AnimCharV20::context.viewPos, lodDistMul);
}

bool AnimcharRendComponent::check_visibility_legacy(uint16_t vis_bits, bbox3f_cref bbox)
{
  // G_ASSERTF(AnimCharV20::prepared_in_frame == dagor_frame_no(),
  //   "AnimCharV20::prepareGlobTm() or AnimCharV20::prepareFrustum() last called on frame %d, but current is %d",
  //   AnimCharV20::prepared_in_frame, dagor_frame_no());
  return check_visibility(vis_bits, bbox, AnimCharV20::context.isMainCamera, AnimCharV20::context.frustum,
    AnimCharV20::context.occlusion);
}

void AnimcharRendComponent::render(const Point3 &view_pos, real transp)
{
  // DEBUG_CTX("render (scene=%p, visBits=%02X)", scene, visBits);
  if (!sceneBeforeRenderedCalled)
    scene->beforeRender(view_pos);
  scene->render(transp);
  sceneBeforeRenderedCalled = true;
}

Point3 AnimcharRendComponent::getNodePosForBone(uint32_t bone_id) const
{
  if (!scene)
    return Point3();
  int nodeIdx = scene->getNodeForBone(bone_id);
  if (nodeIdx < 0)
    return Point3();
  return scene->getNodeWtm(nodeIdx).getcol(3);
}

void AnimcharRendComponent::renderTrans(const Point3 &view_pos, real transp)
{
  // DEBUG_CTX("renderTrans (scene=%p, visBits=%02X)", scene, visBits);
  if (!sceneBeforeRenderedCalled)
    scene->beforeRender(view_pos);
  scene->renderTrans(transp);
  sceneBeforeRenderedCalled = true;
}

bool AnimcharRendComponent::setVisualResource(DynamicRenderableSceneLodsResource *res, bool create_inst, const GeomNodeTree &tree)
{
  lodres = res;
  if (create_inst)
    createInstance(tree);
  return true;
}

void AnimcharRendComponent::setSceneInstance(DynamicRenderableSceneInstance *instance)
{
  if (sceneOwned)
    del_it(scene);
  sceneOwned = false;
  if (lodres != instance->getLodsResource())
    lodres = instance->getLodsResource();
  scene = instance;
  if (nodeMap.isIncomplete())
    patchInvalidInstanceNodes();
}

void AnimcharRendComponent::updateLodResFromSceneInstance() { lodres = scene->getLodsResource(); }

bool AnimcharRendComponent::load(const AnimCharCreationProps &props, int modelId, const GeomNodeTree &tree,
  const AnimcharFinalMat44 &finalWtm)
{
  DynamicRenderableSceneLodsResource *model = (DynamicRenderableSceneLodsResource *)::get_game_resource(modelId);
  if (!model)
    return false;

  loadData(props, model, tree);

  // compute bsphRadExpand
  if (!props.createVisInst && lodres)
    nodeMap.init(tree, lodres->getNames().node);
  if (nodeMap.size())
  {
    if (!validateNodeMap())
      nodeMap.markIncomplete();

    vec4f expand = v_splat_w(finalWtm.bsph);
    for (int i = 0; i < nodeMap.size(); i++)
    {
      if (nodeMap[i].nodeIdx.index() == 0)
        continue;
      expand = v_min(expand,
        v_sub_x(v_splat_w(finalWtm.bsph), v_length3(v_sub(finalWtm.nwtm[nodeMap[i].nodeIdx.index()].col3, finalWtm.bsph))));
    }
    bsphRadExpand = v_extract_x(expand);
    if (bsphRadExpand < 0 || bsphRadExpand >= as_point4(&finalWtm.bsph).w)
      bsphRadExpand = 0;
  }
  if (!props.createVisInst)
    nodeMap.clear();

  ::release_game_resource(modelId);
  return true;
}

void AnimcharRendComponent::loadData(const AnimCharCreationProps &props, DynamicRenderableSceneLodsResource *model,
  const GeomNodeTree &tree)
{
  lodres = model;

  noAnimDist2 = props.noAnimDist;
  noAnimDist2 *= noAnimDist2;
  noRenderDistSq = props.noRenderDist;
  noRenderDistSq *= noRenderDistSq;

  // create instance of resources
  if (props.createVisInst)
    createInstance(tree);
}

bool AnimcharRendComponent::validateNodeMap() const
{
  String err(framemem_ptr());
  char buf[512];
  Bitarray mask(framemem_ptr());
  mask.resize(lodres->getNames().node.nameCount());

  mask.reset();
  for (const GeomTreeIdxMap::Pair &p : nodeMap.map())
  {
    if (mask.get(p.id))
    {
      const char *charn = dgs_get_fatal_context(buf, sizeof(buf));
      if (const char *linebr = strrchr(charn, '\n'))
        charn = linebr + 1;
      logmessage(DAGOR_DBGLEVEL > 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN, "%s: node[%d]=%s is duplicated", charn, p.id,
        lodres->getNames().node.getNameSlow(p.id));
    }
    mask.set(p.id);
  }

  for (int l = 0; l < lodres->lods.size(); l++)
  {
    DynamicRenderableSceneResource *lod = lodres->lods[l].scene;

    mask.reset();
    for (const DynamicRenderableSceneResource::RigidObject &r : lod->getRigidsConst())
      mask.set(r.nodeId);
    for (const ShaderSkinnedMeshResource *r : lod->getSkins())
      for (int i = 0, ie = r->getMesh()->bonesCount(); i < ie; i++)
        mask.set(r->getMesh()->getNodeForBone(i));

    for (const GeomTreeIdxMap::Pair &p : nodeMap.map())
      mask.reset(p.id);

    for (int i = 0; i < mask.size(); i++)
      if (mask.get(i))
      {
        err.printf(0, "dynModel.lod%d has node[%d]=%s that is not set from animChar (missing in skeleton?)", l, i,
          lodres->getNames().node.getNameSlow(i));

#if DAGOR_DBGLEVEL > 0
        logerr("%s%s", err, dgs_get_fatal_context(buf, sizeof(buf)));
#else
        logwarn("%s", err);
#endif
#if _TARGET_PC_WIN && !_TARGET_STATIC_LIB
        DAG_FATAL(err);
#endif
      }
  }
  return err.empty();
}
int AnimcharRendComponent::patchInvalidInstanceNodes()
{
  int patched = 0;
  Bitarray mask;
  mask.resize(lodres->getNames().node.nameCount());
  mask.reset();

  for (int l = 0; l < lodres->lods.size(); l++)
  {
    DynamicRenderableSceneResource *lod = lodres->lods[l].scene;

    for (const DynamicRenderableSceneResource::RigidObject &r : lod->getRigidsConst())
      mask.set(r.nodeId);
    for (const ShaderSkinnedMeshResource *r : lod->getSkins())
      for (int i = 0, ie = r->getMesh()->bonesCount(); i < ie; i++)
        mask.set(r->getMesh()->getNodeForBone(i));
  }

  for (const GeomTreeIdxMap::Pair &p : nodeMap.map())
    mask.reset(p.id);

  for (int i = 0; i < mask.size(); i++)
    if (mask.get(i))
    {
      scene->showNode(i, false);
      scene->setNodeWtm(i, ZERO<TMatrix>());
      patched++;
    }
  return patched;
}

void AnimcharRendComponent::cloneTo(AnimcharRendComponent *as, bool create_inst, const GeomNodeTree &tree) const
{
  as->setVisualResource(lodres, create_inst, tree);

  //== more
  as->noAnimDist2 = noAnimDist2;
  as->noRenderDistSq = noRenderDistSq;
  as->bsphRadExpand = bsphRadExpand;
}

void AnimcharRendComponent::createInstance(const GeomNodeTree &tree)
{
  if (sceneOwned)
    del_it(scene);
  nodeMap.init(tree, lodres->getNames().node);
  if (!validateNodeMap())
    nodeMap.markIncomplete();

  sceneOwned = true;
  scene = new DynamicRenderableSceneInstance(lodres);

  if (nodeMap.isIncomplete())
    patchInvalidInstanceNodes();
}

void AnimcharRendComponent::setNodes(DynamicRenderableSceneInstance *s, const GeomTreeIdxMap &nodemap,
  const AnimcharFinalMat44 &finalWtm)
{
  if (!s)
    return;
  if (memcmp(&finalWtm.wofs, ZERO_PTR<vec3f>(), sizeof(vec3f)) == 0)
    for (int i = 0; i < nodemap.size(); i++)
      s->setNodeWtm(nodemap[i].id, finalWtm.nwtm[nodemap[i].nodeIdx.index()]);
  else
    for (int i = 0; i < nodemap.size(); i++)
    {
      mat44f m4 = finalWtm.nwtm[nodemap[i].nodeIdx.index()];
      m4.col3 = v_add(m4.col3, finalWtm.wofs);
      s->setNodeWtm(nodemap[i].id, m4);
    }
}

void AnimcharRendComponent::prepareForRender(const AnimcharFinalMat44 &finalWtm, bbox3f_cref bbox)
{
  setNodes(scene, nodeMap, finalWtm);
  scene->setBoundingBox(BBox3(as_point3(&bbox.bmin), as_point3(&bbox.bmax)));
}
