//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_mathBase.h>
#include <anim/dag_animDecl.h>
#include <anim/dag_animPureInterface.h>
#include <anim/dag_animStateHolder.h>
#include <anim/dag_animBlend.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_dynSceneRes.h>
#include <math/dag_geomTreeMap.h>

// forward declarations for external classes and structures
class GeomNodeTree;
class DynamicRenderableSceneLodsResource;
class DynamicSceneWithTreeResource;
class FastPhysSystem;
class PhysicsResource;
class DynamicRenderableSceneInstance;
class DataBlock;
class Occlusion;
struct RoDataBlock;
struct Point3_vec4;
struct Frustum;
class CharacterGameResFactory;

struct AnimCharData
{
  Ptr<AnimV20::AnimationGraph> graph;
  const char *agResName = nullptr;
};

namespace AnimV20
{
struct AnimMap
{
  dag::Index16 geomId;
  int16_t animId;
};
typedef unsigned attachment_uid_t;
static constexpr int INVALID_ATTACHMENT_UID = 0;

class GenericAnimStatesGraph : public IAnimStateDirector2
{
protected:
  IPureAnimStateHolder *st;

public:
  GenericAnimStatesGraph() { st = 0; }

  // to be implemented by descendants
  virtual bool init(AnimationGraph *anim, IPureAnimStateHolder *st) = 0;
  virtual void advance() = 0;
  virtual void atIrq(int irq_type, IAnimBlendNode *src) = 0;
};

class IAnimCharPostController
{
public:
  virtual void onPostAnimBlend(GeomNodeTree & /*tree*/, vec3f /*world_translate*/) {}
  virtual bool overridesBlender() const { return false; }

  virtual void update(real dt, GeomNodeTree &tree, AnimcharBaseComponent *ac) = 0;
};

struct IAnimCharacter2Info
{
  SimpleString resName;
};

//
// Basic animated character interface
//
struct AnimCharCreationProps
{
  SimpleString centerNode;
  eastl::unique_ptr<RoDataBlock> props;

  SimpleString rootNode;
  float pyOfs, pxScale, pyScale, pzScale;
  float sScale;
  bool useCharDep;
  bool createVisInst;

  real noAnimDist;
  real noRenderDist;
  real bsphRad;

  struct CtorNoInit
  {};
  AnimCharCreationProps(CtorNoInit);
  AnimCharCreationProps();
  void release();
};

struct AnimcharIrqHandler : public IGenericIrq
{
  using IrqFunc = intptr_t(int, intptr_t, intptr_t, intptr_t, void *);
  IrqFunc *f = nullptr;
  void *a = nullptr;

  AnimcharIrqHandler(IrqFunc *_f, void *_a) : f(_f), a(_a) {}
  intptr_t irq(int t, intptr_t p1, intptr_t p2, intptr_t p3) override { return f ? f(t, p1, p2, p3, a) : GIRQR_NoResponse; }
};

struct AnimcharFinalMat44
{
  SmallTab<mat44f, MidmemAlloc> nwtm; //< per-node world matrices (relative to wofs)
  vec4f wofs = v_zero();              //< world offset for matrices (to improve accuracy of nwtm and final nwtm+wofs positions)
  vec4f bsph = v_zero();              //< base bounding sphere in world coords

  AnimcharFinalMat44() = default;
  EA_NON_COPYABLE(AnimcharFinalMat44);
};

class AnimcharBaseComponent
{
public:
  AnimcharBaseComponent();
  ~AnimcharBaseComponent();

  AnimcharBaseComponent(const AnimcharBaseComponent &) = delete;            // non_copyable
  AnimcharBaseComponent &operator=(const AnimcharBaseComponent &) = delete; // non_copyable

  const IAnimCharacter2Info *getCreateInfo() const { return &creationInfo; }
  void reset();

  void setTmRel(const mat44f &tm)
  {
    nodeTree.setRootTm(tm, nodeTree.getWtmOfs());
    nodeTree.invalidateWtm();
  } //< don't change wofs
  void setTmWithOfs(const mat44f &tm, vec3f wofs)
  {
    nodeTree.setRootTm(tm, wofs);
    nodeTree.invalidateWtm();
  } //< set explicit wofs
  vec3f getWtmOfs() const { return nodeTree.getWtmOfs(); }

  void setTm(const mat44f &tm, bool setup_wofs = false);
  void setTm(const TMatrix &tm, bool setup_wofs = false);
  void setTm(const Point3 &pos, const Point3 &dir, const Point3 &up);
  void setTm(vec4f pos, vec3f dir, vec3f up); // pos should has 1 in .w
  void setPos(const Point3 &pos);
  void setDir(float ax, float ay, float az);

  void getTm(mat44f &tm) const;
  void getTm(TMatrix &tm) const;
  Point3 getPos(bool recalc_anim_if_needed = false);

  dag::Index16 findINodeIndex(const char *name) const { return nodeTree.findINodeIndex(name); }

  const GeomNodeTree &getNodeTree() const { return nodeTree; }
  GeomNodeTree &getNodeTree() { return nodeTree; }
  const GeomNodeTree *getNodeTreePtr() const { return &nodeTree; }
  GeomNodeTree *getNodeTreePtr() { return &nodeTree; }
  const GeomNodeTree *getOriginalNodeTree() const { return originalNodeTree; }

  const AnimationGraph *getAnimGraph() const { return animGraph; }
  AnimationGraph *getAnimGraph() { return animGraph; }
  const IAnimStateHolder *getAnimState() const { return animState; }
  IAnimStateHolder *getAnimState() { return animState; }
  const Tab<AnimMap> &getAnimMap() const { return animMap; }

  // returns state director (if any) and sets up animState for it
  IAnimStateDirector2 *getAnimStateDirector() const { return stateDirector; }

  void enableAnimStateDirector(bool en) { stateDirectorEnabled = en; }
  bool isAnimStateDirectorEnabled() { return stateDirectorEnabled; }

  // advance animation state and invalidate animation
  void act(real dt, bool calc_anim);

  // just invalidate animation (anim state is not changed)
  void invalidateAnim() { animValid = false; }

  // just recalc wtm's, dont recalc animation
  void recalcWtm();

  // recalculates of animation and WTMs (not affected by visibiliy/animation radius)
  void doRecalcAnimAndWtm()
  {
    invalidateAnim();
    calcAnimWtm(true);
  }

  void forcePostRecalcWtm(real dt);

  void registerIrqHandler(int irq, IGenericIrq *handler);
  void unregisterIrqHandler(int irq, IGenericIrq *handler); // Note: if 'irq' is negative, than handler will be removed from everything

  void setOriginVelStorage(Point3 *linvel, Point3 *angvel)
  {
    originLinVelStorage = linvel;
    originAngVelStorage = angvel;
  }

  FastPhysSystem *getFastPhysSystem() const { return fastPhysSystem; }
  void setFastPhysSystem(FastPhysSystem *system);
  void setFastPhysSystemGravityDirection(const Point3 &gravity_direction);
  bool resetFastPhysSystem();
  void updateFastPhys(float dt);
  void resetFastPhysWtmOfs(vec3f wofs);

  void setPhysicsResource(PhysicsResource *phSys) { physSys = phSys; }
  PhysicsResource *getPhysicsResource() const { return physSys; }

  IAnimCharPostController *getPostController() const { return postCtrl; }
  void setPostController(IAnimCharPostController *ctrl);

  IMotionMatchingController *getMotionMatchingController() const { return motionMatchingController; }
  void setMotionMatchingController(IMotionMatchingController *controller) { motionMatchingController = controller; }

  //! attach animchar to named slot. return attachment_idx >= 0 in case success or negative otherwise
  int setAttachedChar(int slot_id, attachment_uid_t uid, AnimcharBaseComponent *attChar, bool recalcable = true);
  void releaseAttachment(int attachment_idx);

  //! returns animchar attached to named slot
  unsigned getAttachmentUid(int slot_id) const
  {
    int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
    return idx < 0 ? 0 : attachment[idx].uid;
  }
  //! returns animchar attached to named slot
  AnimcharBaseComponent *getAttachedChar(int slot_id) const;
  //! returns skeleton effectively attached to named slot
  GeomNodeTree *getAttachedSkeleton(int slot_id) const;
  //! returns tm for named slot (returns NULL when tm is not set)
  const mat44f *getAttachmentTm(int slot_id) const;
  //! returns wtm from original nodeTree for named slot (returns NULL when wtm is not set)
  const mat44f *getSlotNodeWtm(int slot_id) const;
  // getSlotNodeWtm + getAttachmentTm. Return false if slot is invalid
  bool initAttachmentTmAndNodeWtm(int slot_id, mat44f &attach_tm) const;
  //! returns skeleton node's index for named slot (returns -1 when node is not set)
  dag::Index16 getSlotNodeIdx(int slot_id) const;

  void createDebugBlenderContext(bool dump_all_nodes = false);
  void destroyDebugBlenderContext();
  const DataBlock *getDebugBlenderState(bool dump_tm = false);
  const DataBlock *getDebugNodemasks();

  void setTraceContext(intptr_t ctx) { traceContext = ctx; }
  void setAlwaysUpdateAnimMode(bool on) { forceAnimUpdate = on; }

  static intptr_t irq(int type, intptr_t p1, intptr_t p2, intptr_t p3, void *arg);

  const AnimV20::AnimBlender::CharNodeModif *getCharDepModif() const
  {
    return (node0.chanNodeId() < -1 || !animGraph) ? nullptr : &node0;
  }
  bool updateCharDepScales(float pyOfs, float pxScale, float pyScale, float pzScale, float sScale);
  bool getCharDepScales(float &pyOfs, float &pxScale, float &pyScale, float &pzScale, float &sScale) const;
  bool applyCharDepScale(float scale)
  {
    if (node0.chanNodeId() == -2 || !animGraph || !charDepNodeId || !originalNodeTree)
      return false;
    return updateCharDepScales(charDepBasePYofs * scale, scale, scale, scale, scale);
  }

  bool load(const AnimCharCreationProps &props, int skelId, int acId, int physId);
  void cloneTo(AnimcharBaseComponent *as, bool reset_anim) const;

  vec4f copyNodesTo(AnimcharFinalMat44 &finalWtm) const // returns base bounding sphere
  {
    G_ASSERTF_RETURN(nodeTree.nodeCount() == finalWtm.nwtm.size(), v_zero(), "tree.nodeCount=%d finalWtm.nwtm.count=%d",
      nodeTree.nodeCount(), finalWtm.nwtm.size());

    finalWtm.wofs = nodeTree.getWtmOfs();
    memcpy(finalWtm.nwtm.data(), &nodeTree.getRootWtmRel(), data_size(finalWtm.nwtm));

    const mat44f &wtm = nodeTree.getNodeWtmRel(centerNodeIdx);
    vec4f max_scl_sq = v_max(v_max(v_length3_sq(wtm.col0), v_length3_sq(wtm.col1)), v_length3_sq(wtm.col2));
    return finalWtm.bsph = v_perm_xyzd(v_add(wtm.col3, finalWtm.wofs), v_mul(v_splat4(&centerNodeBsphRad), v_sqrt4(max_scl_sq)));
  }

protected:
  GeomNodeTree nodeTree, *originalNodeTree;

  Ptr<AnimationGraph> animGraph;
  AnimCommonStateHolder *animState;
  GenericAnimStatesGraph *stateDirector;

  Tab<AnimMap> animMap;
  Tab<vec4f> animMapPRS;

  AnimBlender::CharNodeModif node0;
  float charDepBasePYofs;
  dag::Index16 charDepNodeId;
  uint16_t stateDirectorEnabled : 1, animValid : 1, forceAnimUpdate : 1;
  static constexpr unsigned MAGIC_BITS = 13, MAGIC_VALUE = 1511;
  G_STATIC_ASSERT(MAGIC_VALUE < (1 << MAGIC_BITS));
  uint16_t magic : MAGIC_BITS;
  intptr_t traceContext = 0;
  real totalDeltaTime;
  real centerNodeBsphRad;
  dag::Index16 centerNodeIdx;
  Point3 *originLinVelStorage, *originAngVelStorage;

  struct Attachment
  {
    mat44f tm;
    AnimcharBaseComponent *animChar;
    attachment_uid_t uid;
    dag::Index16 nodeIdx;
    bool recalcable;
  };
  uint64_t recalcableAttachments = 0; // Bit mask of `recalcable` flags for first 64 attachment slots
  SmallTab<Attachment, MidmemAlloc> attachment;
  SmallTab<int16_t, MidmemAlloc> attachmentSlotId;

  typedef Tab<IGenericIrq *> IrqTab;
  Tab<IrqTab> irqHandlers;

  FastPhysSystem *fastPhysSystem;
  PhysicsResource *physSys;
  IAnimCharPostController *postCtrl;
  IMotionMatchingController *motionMatchingController;
  AnimDbgCtx *animDbgCtx;
  IAnimCharacter2Info creationInfo;

protected:
  void recalcWtm(vec3f world_translate);
  void postRecalcWtm();
  void calcAnimWtm(bool may_calc_anim);
  void recalcAttachments();
  real getSqDistanceToViewerLegacy() const;

  void setupAnim();
  void resetAnim();
  void setOriginalNodeTreeRes(GeomNodeTree *n);
  void loadData(const AnimCharCreationProps &props, GeomNodeTree *skeleton, AnimationGraph *ag);
  bool loadAnimGraphCpp(const char *res_name);

  friend class ::CharacterGameResFactory;
};

class AnimcharRendComponent
{
public:
  enum
  {
    VISFLG_RENDERABLE = (1 << 0),
    VISFLG_IN_RANGE = (1 << 1),
    VISFLG_BSPH = (1 << 2),
    VISFLG_MAIN = (1 << 3),
    VISFLG_SHADOW = (1 << 4)
  };

public:
  AnimcharRendComponent();
  ~AnimcharRendComponent();

  AnimcharRendComponent(const AnimcharRendComponent &) = delete;            // non_copyable
  AnimcharRendComponent &operator=(const AnimcharRendComponent &) = delete; // non_copyable

  //! computes worldbox using current animated nodetree (no anim recalc even when invalid)
  bool calcWorldBox(bbox3f &out_wbb, const AnimcharFinalMat44 &finalWtm, bool accurate = true) const;
  vec4f prepareSphere(const AnimcharFinalMat44 &finalWtm) const;
  float getRenderDistanceSq() const { return noRenderDistSq; }
  float getAnimDistanceSq() const { return noAnimDist2; }

  void setVisible(bool v)
  {
    if (v)
      visBits |= VISFLG_RENDERABLE;
    else
      visBits &= ~VISFLG_RENDERABLE;
  }
  uint16_t getVisBits() const { return visBits; }
  bool isVisibleInMainCamera() const { return (visBits & VISFLG_MAIN) != 0; }

  // computes visibility and returns visBits
  uint16_t beforeRenderLegacy(const AnimcharFinalMat44 &finalWtm, vec4f &out_rendBsph, bbox3f &out_rendBbox);
  uint16_t beforeRender(const AnimcharFinalMat44 &finalWtm, vec4f &out_rendBsph, bbox3f &out_rendBbox, const Frustum &f,
    float inv_wk_sq, Occlusion *occl, const Point3 &cameraPos);
  Point3 getNodePosForBone(uint32_t bone_id) const;

  void render(const Point3 &view_pos, real tr = 1.f);
  void renderTrans(const Point3 &view_pos, real tr = 1.f);

  void setRenderDistances(real anim_dist, real rend_dist)
  {
    noAnimDist2 = anim_dist * anim_dist;
    noRenderDistSq = rend_dist < 0.f ? -1.f : rend_dist * rend_dist;
  }

  DynamicRenderableSceneLodsResource *getVisualResource() const { return lodres; }

  bool setVisualResource(DynamicRenderableSceneLodsResource *resource, bool create_visual_instance, const GeomNodeTree &tree);

  DynamicRenderableSceneInstance *getSceneInstance() { return scene; }
  const DynamicRenderableSceneInstance *getSceneInstance() const { return scene; }
  void setSceneInstance(DynamicRenderableSceneInstance *instance);
  void updateLodResFromSceneInstance() { lodres = scene->getLodsResource(); };

  void setOwnerTm(const TMatrix &tm)
  {
    mat44f m4;
    v_mat44_make_from_43cu_unsafe(m4, tm.m[0]);
    v_mat44_transpose_to_mat43(ownerTm, m4);
  }

  bool load(const AnimCharCreationProps &props, int modelId, const GeomNodeTree &tree, const AnimcharFinalMat44 &finalWtm);
  void cloneTo(AnimcharRendComponent *as, bool create_inst, const GeomNodeTree &tree) const;

  bool shouldBeAnimatedLegacy(mat44f &root_wtm, const Point3 &view_pos) const
  {
    const real dist2 = getSqDistanceToViewerLegacy(root_wtm, view_pos);
    if (dist2 < 0)
      return false;
    return dist2 * AnimcharRendComponent::invWkSq < noAnimDist2;
  }

  static float invWkSq;

  static bool check_visibility_legacy(uint16_t vis_bits, bbox3f_cref bbox);
  static inline bool check_visibility(uint16_t vis_bits, bbox3f_cref bbox, bool is_main, const Frustum &f, Occlusion *occl);

  GeomTreeIdxMap &getNodeMap() { return nodeMap; }
  const GeomTreeIdxMap &getNodeMap() const { return nodeMap; }
  bool sceneBeforeRenderWasCalled() const { return sceneBeforeRenderedCalled != 0; } // to be removed
  void setSceneBeforeRenderCalled() { sceneBeforeRenderedCalled = 1; }               // to be removed
  void setSceneBeforeRenderCalled(bool v) { sceneBeforeRenderedCalled = v ? 1 : 0; } // to be removed
  void setNodes(DynamicRenderableSceneInstance *, const GeomTreeIdxMap &nodemap, const AnimcharFinalMat44 &finalWtm);

protected:
  mat43f ownerTm;
  GeomTreeIdxMap nodeMap;
  DynamicRenderableSceneInstance *scene;
  Ptr<DynamicRenderableSceneLodsResource> lodres;
  real noAnimDist2, noRenderDistSq, bsphRadExpand;
  uint16_t visBits : 6, sceneBeforeRenderedCalled : 1, sceneOwned : 1;
  static constexpr unsigned MAGIC_BITS = 8, MAGIC_VALUE = 107;
  G_STATIC_ASSERT(MAGIC_VALUE < (1 << MAGIC_BITS));
  uint16_t magic : MAGIC_BITS;


protected:
  void prepareForRender(const AnimcharFinalMat44 &finalWtm, bbox3f_cref bbox);
  real getSqDistanceToViewerLegacy(const mat44f &root_wtm, const Point3 &view_pos) const;

  void createInstance(const GeomNodeTree &tree);
  void loadData(const AnimCharCreationProps &props, DynamicRenderableSceneLodsResource *model, const GeomNodeTree &tree);
  bool validateNodeMap() const;
  int patchInvalidInstanceNodes();
};


class IAnimCharacter2
{
public:
  void destroy() { delete this; }
  const IAnimCharacter2Info *getCreateInfo() const { return base.getCreateInfo(); }

  bool load(const AnimCharCreationProps &props, int modelId, int skelId, int acId, int physId)
  {
    if (!base.load(props, skelId, acId, physId))
      return false;
    clear_and_resize(finalWtm.nwtm, base.getNodeTree().nodeCount());
    rendBsph = copyNodes();
    return rend.load(props, modelId, base.getNodeTree(), finalWtm);
  }

  void cloneTo(IAnimCharacter2 *as, bool create_visual_instance = true, bool reset_anim = false) const
  {
    base.cloneTo(&as->base, reset_anim);
    rend.cloneTo(&as->rend, create_visual_instance, base.getNodeTree());
    clear_and_resize(as->finalWtm.nwtm, finalWtm.nwtm.size());
    as->rendBsph = as->copyNodes();
  }

  IAnimCharacter2 *clone(bool create_visual_instance = true, bool reset_anim = false) const
  {
    IAnimCharacter2 *as = new (midmem) IAnimCharacter2;
    cloneTo(as, create_visual_instance, reset_anim);
    return as;
  }

  AnimcharBaseComponent &baseComp() { return base; }
  AnimcharRendComponent &rendComp() { return rend; }
  const AnimcharRendComponent &rendComp() const { return rend; }

  // base component
  void setTm(const mat44f &tm) { finalWtm.nwtm[0] = tm; }
  void getTm(mat44f &tm) const { tm = finalWtm.nwtm[0]; }
  void setTm(vec4f pos, vec3f dir, vec3f up);
  void setTm(const TMatrix &tm);
  void setTm(const Point3 &pos, const Point3 &dir, const Point3 &up);
  void getTm(TMatrix &tm) const;
  Point3 getPos()
  {
    Point3 ret;
    v_stu_p3(&ret.x, finalWtm.nwtm[0].col3);
    return ret;
  }
  void invalidateAnim() { base.invalidateAnim(); }

  void reset() { base.reset(); }
  void act(real dt, const Point3 &view_pos, bool force_anim = false)
  {
    base.setTm(finalWtm.nwtm[0]);
    base.act(dt, force_anim || rend.shouldBeAnimatedLegacy(finalWtm.nwtm[0], view_pos));
    copyNodes();
  }
  void doRecalcAnimAndWtm()
  {
    base.setTm(finalWtm.nwtm[0]);
    base.doRecalcAnimAndWtm();
    copyNodes();
  }
  void recalcWtm()
  {
    base.setTm(finalWtm.nwtm[0]);
    base.recalcWtm();
    copyNodes();
  }

  dag::Index16 findINodeIndex(const char *name) const { return base.findINodeIndex(name); }
  const GeomNodeTree &getNodeTree() const { return base.getNodeTree(); }
  GeomNodeTree &getNodeTree() { return base.getNodeTree(); }
  const GeomNodeTree *getOriginalNodeTree() const { return base.getOriginalNodeTree(); }

  AnimationGraph *getAnimGraph() { return base.getAnimGraph(); }
  const AnimationGraph *getAnimGraph() const { return base.getAnimGraph(); }
  IAnimStateHolder *getAnimState() { return base.getAnimState(); }
  const IAnimStateHolder *getAnimState() const { return base.getAnimState(); }
  IAnimStateDirector2 *getAnimStateDirector() const { return base.getAnimStateDirector(); }

  void enableAnimStateDirector(bool en) { base.enableAnimStateDirector(en); }
  bool isAnimStateDirectorEnabled() { return base.isAnimStateDirectorEnabled(); }

  IAnimCharPostController *getPostController() const { return base.getPostController(); }
  void setPostController(IAnimCharPostController *ctrl) { base.setPostController(ctrl); }

  bool resetFastPhysSystem() { return base.resetFastPhysSystem(); }
  void setFastPhysSystem(FastPhysSystem *system) { base.setFastPhysSystem(system); }
  FastPhysSystem *getFastPhysSystem() const { return base.getFastPhysSystem(); }
  PhysicsResource *getPhysicsResource() const { return base.getPhysicsResource(); }

  int setAttachedChar(int slot_id, attachment_uid_t uid, AnimcharBaseComponent *attChar, bool recalcable = true)
  {
    return base.setAttachedChar(slot_id, uid, attChar, recalcable);
  }
  AnimcharBaseComponent *getAttachedChar(int slot_id) { return base.getAttachedChar(slot_id); }
  const mat44f *getSlotNodeWtm(int slot_id) const { return base.getSlotNodeWtm(slot_id); }
  const mat44f *getAttachmentTm(int slot_id) const { return base.getAttachmentTm(slot_id); }

  void registerIrqHandler(int irq, IGenericIrq *handler) { base.registerIrqHandler(irq, handler); }
  void unregisterIrqHandler(int irq, IGenericIrq *handler) { base.unregisterIrqHandler(irq, handler); }
  void setTraceContext(intptr_t ctx) { base.setTraceContext(ctx); }
  void setOriginVelStorage(Point3 *linvel, Point3 *angvel) { base.setOriginVelStorage(linvel, angvel); }

  bool applyCharDepScale(float scale) { return base.applyCharDepScale(scale); }
  bool getCharDepScales(float &pyOfs, float &pxScale, float &pyScale, float &pzScale, float &sScale)
  {
    return base.getCharDepScales(pyOfs, pxScale, pyScale, pzScale, sScale);
  }
  bool updateCharDepScales(float pyOfs, float pxScale, float pyScale, float pzScale, float sScale)
  {
    return base.updateCharDepScales(pyOfs, pxScale, pyScale, pzScale, sScale);
  }

  void createDebugBlenderContext(bool dump_all_nodes = false) { base.createDebugBlenderContext(dump_all_nodes); }
  void destroyDebugBlenderContext() { base.destroyDebugBlenderContext(); }
  const DataBlock *getDebugBlenderState(bool dump_tm = false) { return base.getDebugBlenderState(dump_tm); }
  const DataBlock *getDebugNodemasks() { return base.getDebugNodemasks(); }

  // render component
  Point3 getCenterPos()
  {
    Point3 ret;
    v_stu_p3(&ret.x, rendBsph);
    return ret;
  }
  bool calcWorldBox(bbox3f &out_wbb) { return rend.calcWorldBox(out_wbb, finalWtm); }
  bool calcWorldBox() { return calcWorldBox(rendBbox); }
  void setVisible(bool v) { rend.setVisible(v); }
  void setAlwaysUpdateAnimMode(bool on) { base.setAlwaysUpdateAnimMode(on); }
  void setOwnerTm(const TMatrix &tm) { rend.setOwnerTm(tm); }
  void setRenderDistances(real anim_dist, real rend_dist) { rend.setRenderDistances(anim_dist, rend_dist); }
  void setSceneInstance(DynamicRenderableSceneInstance *inst) { rend.setSceneInstance(inst); }
  void setBoundingBox(const BBox3 &in_bbox)
  {
    rendBbox.bmin = v_ldu(&in_bbox[0].x);
    rendBbox.bmax = v_ldu(&in_bbox[1].x);
  }

  bool beforeRender() { return (rend.beforeRenderLegacy(finalWtm, rendBsph, rendBbox) & rend.VISFLG_MAIN) != 0; }
  bool beforeRender(const Frustum &f, float inv_wk_sq, Occlusion *occl, const Point3 &cam_pos)
  {
    return (rend.beforeRender(finalWtm, rendBsph, rendBbox, f, inv_wk_sq, occl, cam_pos) & rend.VISFLG_MAIN) != 0;
  }
  void render(const Point3 &view_pos, real tr = 1)
  {
    if (AnimcharRendComponent::check_visibility_legacy(rend.getVisBits(), rendBbox))
      rend.render(view_pos, tr);
  }
  void renderTrans(const Point3 &view_pos, real tr = 1)
  {
    if (AnimcharRendComponent::check_visibility_legacy(rend.getVisBits(), rendBbox))
      rend.renderTrans(view_pos, tr);
  }

  DynamicRenderableSceneInstance *getSceneInstance() { return rend.getSceneInstance(); }
  const DynamicRenderableSceneInstance *getSceneInstance() const { return rend.getSceneInstance(); }
  DynamicRenderableSceneLodsResource *getVisualResource() { return rend.getVisualResource(); }
  bool setVisualResource(DynamicRenderableSceneLodsResource *r, bool inst)
  {
    return rend.setVisualResource(r, inst, base.getNodeTree());
  }

  BSphere3 getBoundingSphere() { return BSphere3(getCenterPos(), v_extract_w(rendBsph)); }
  bool isVisibleInMainCamera() const { return rend.isVisibleInMainCamera(); }
  void renderForShadow(const Point3 &view_pos) { rend.render(view_pos); }

  vec4f copyNodes() { return base.copyNodesTo(finalWtm); }
  const bbox3f &get_bbox() const { return rendBbox; }
  const AnimcharFinalMat44 &getFinalWtm() const { return finalWtm; };

protected:
  AnimcharBaseComponent base;
  AnimcharRendComponent rend;
  vec4f rendBsph;
  bbox3f rendBbox;
  AnimcharFinalMat44 finalWtm;
};
} // end of namespace AnimV20


namespace AnimCharV20
{
using namespace AnimV20;
void prepareGlobTm(bool is_main_camera, const Frustum &culling_frustum, float hk, const Point3 &viewPos, Occlusion *occlusion);
void prepareFrustum(bool is_main_camera, const Frustum &culling_frustum, const Point3 &viewPos, Occlusion *occlusion);

int getSlotId(const char *slot_name);
int addSlotId(const char *slot_name);

extern bool (*trace_static_ray_down)(const Point3_vec4 &from, float max_t, float &out_t, intptr_t ctx);
extern bool (*trace_static_ray_dir)(const Point3_vec4 &from, Point3_vec4 dir, float max_t, float &out_t, intptr_t ctx);

// registers factory for loading statesGraph from compiled-in source code
void registerGraphCppFactory();
} // end of namespace AnimCharV20
