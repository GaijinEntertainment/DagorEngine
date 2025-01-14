//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animChannels.h>
#include <anim/dag_animPureInterface.h>
#include <anim/dag_animStateHolder.h>
#include <anim/dag_animIrq.h>
#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastStrMap.h>
#include <EASTL/hash_map.h>
#include <util/dag_string.h>

// forward declarations for external classes and structures
class DataBlock;
class GeomNodeTree;

namespace eastl
{
class allocator;
template <typename T, typename A>
class vector;
template <typename A, typename T, typename C>
class bitvector;
} // namespace eastl

namespace AnimResManagerV20
{
struct NodeMask
{
  NameMap nm;
  String name;
  AnimV20::AnimData *anim;
};
} // namespace AnimResManagerV20

namespace AnimV20
{
struct AnimDbgCtx;

// declare classes IDs first
decl_dclassid(0x20000001, IAnimBlendNode);
decl_dclassid(0x20000002, AnimBlendNodeLeaf);
decl_dclassid(0x20000003, AnimPostBlendCtrl);
decl_dclassid(0x20000004, AnimationGraph);


//
// Base animation blending node interface
//
class IAnimBlendNode : public DObject
{
public:
  struct BlendCtx
  {
    IPureAnimStateHolder *st = NULL;
    float *wt = NULL, *abnWt = NULL, *pbcWt = NULL;
    int *cKeyPos = NULL;
    float lastDt = 0;
    bool animBlendPass = false;
    intptr_t (*irqFunc)(int, intptr_t, intptr_t, intptr_t, void *) = nullptr;
    void *irqArg = nullptr;

    template <typename T>
    BlendCtx(IPureAnimStateHolder &s, T &t, bool bp) :
      st(&s), wt(t.bnlWt.data()), cKeyPos(t.bnlCT.data()), pbcWt(t.pbcWt.data()), animBlendPass(bp), irqFunc(t.irq), irqArg(t.irqArg)
    {}
    BlendCtx(IPureAnimStateHolder &s) : st(&s) {}

    intptr_t irq(int type, intptr_t p1, intptr_t p2, intptr_t p3)
    {
      return irqFunc ? irqFunc(type, p1, p2, p3, irqArg) : GIRQR_NoResponse;
    }
  };

public:
  IAnimBlendNode() { abnId = -1; }

  virtual void destroy() = 0;

  // general blending interface
  virtual void buildBlendingList(BlendCtx &bctx, real w) = 0;
  virtual void reset(IPureAnimStateHolder &st) = 0;
  virtual bool isAliasOf(IPureAnimStateHolder & /*st*/, IAnimBlendNode *n) { return this == n; }
  virtual real getDuration(IPureAnimStateHolder & /*st*/) { return 1.0; }
  virtual real getAvgSpeed(IPureAnimStateHolder & /*st*/) { return 0.0f; }
  virtual int getTimeScaleParamId(IPureAnimStateHolder & /*st*/) { return -1; }

  // interface for seek/tell (used for conditions in anim states graph)
  virtual void seek(IPureAnimStateHolder & /*st*/, real /*rel_pos*/) {}
  virtual real tell(IPureAnimStateHolder & /*st*/) { return 0; }

  // interface for sync and resume
  virtual void seekToSyncTime(IPureAnimStateHolder & /*st*/, real /*offset*/) {}
  virtual void pause(IPureAnimStateHolder & /*st*/) {}
  virtual void resume(IPureAnimStateHolder & /*st*/, bool /*rewind*/) {}

  // interface for named ranges
  virtual bool isInRange(IPureAnimStateHolder & /*st*/, int /*rangeId*/) { return false; }

  // validate subref against specific node (e.g. missing Null)
  virtual bool validateNodeNotUsed(AnimationGraph & /*g*/, IAnimBlendNode * /*test_n*/) { return true; }

  // collect blend nodes referenced by this node
  virtual void collectUsedBlendNodes(AnimationGraph & /*g*/, eastl::hash_map<IAnimBlendNode *, bool> & /*node_map*/) {}

  // RTTI interface implementation
  virtual const char *class_name() const { return "IAnimBlendNode"; }
  virtual bool isSubOf(DClassID id) { return id == IAnimBlendNodeCID; }

  int getAnimNodeId() const { return abnId; }
  void setAnimNodeId(int id) { abnId = id; }

  virtual void debugSetIrqPos(const char * /*irq_name*/, float /*rel_pos*/) {}

  virtual void initChilds(AnimationGraph & /*graph*/, const DataBlock & /*settings*/, const char * /*nm_suffix*/) {}
  virtual void checkHasLoop(AnimationGraph & /*graph*/,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> & /*visited_nodes*/)
  {}

private:
  int abnId;
};


// used for temporary storage of weighted animation
struct AnimTmpWeightedNode
{
  IAnimBlendNode *n;
  real w;
};


//
// Leaf blending node that is physically blended
//
class AnimBlendNodeLeaf : public IAnimBlendNode
{
public:
  Ptr<AnimData> anim;

protected:
  AnimationGraph &graph;
  real timeRatio;
  int bnlId;
  int timeScaleParamId;
  vec3f addOriginVel;
  unsigned dontUseOriginVel : 1, additive : 1, foreignAnimation : 1, ignoredAnimation : 1;

  struct NamedRange
  {
    IPoint2 range;
    int rangeId;
  };
  Tab<NamedRange> namedRanges;

public:
  AnimBlendNodeLeaf(AnimationGraph &graph, AnimData *a);
  ~AnimBlendNodeLeaf();
  virtual void destroy();

  // returns relative position according to current state of curTime
  virtual real tellRaw() { return 0; }
  virtual int getTimeScaleParamId(IPureAnimStateHolder & /*st*/) { return timeScaleParamId; }
  bool isAdditive() const { return additive; }
  int getBnlId() const { return bnlId; }

  // creation-time routines
  void setTimeScaleParam(const char *name);
  void buildNamedRanges(const DataBlock *blk);
  void enableOriginVel(bool en);
  void setAdditive(bool add);
  void setAddOriginVel(const Point3 &add_ov);
  void setCharDep(bool char_dep) { foreignAnimation = char_dep; }

  bool isAnimationIgnored() { return ignoredAnimation; }
  void setAnimationIgnored(bool ignored) { ignoredAnimation = ignored; }

  virtual const char *class_name() const { return "AnimBlendNodeLeaf"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeLeafCID || IAnimBlendNode::isSubOf(id); }

  static int __cdecl _cmp_rangenames(const void *p1, const void *p2);

  friend class AnimBlender;

private:
  AnimBlendNodeLeaf &operator=(const AnimBlendNodeLeaf &);
};


//
// General interface for post-blend controllers (that act on geom nodes directly)
//
class AnimPostBlendCtrl : public IAnimBlendNode
{
protected:
  AnimationGraph &graph;
  int pbcId;

public:
  struct Context
  {
    const GeomNodeTree &origTree;
    AnimcharBaseComponent *ac;
    dag::Span<float> blendWt;
    vec3f worldTranslate;
    const vec3f *acScale;
    intptr_t (*irqFunc)(int, intptr_t, intptr_t, intptr_t, void *) = nullptr;
    void *irqArg = nullptr;

    Context(const GeomNodeTree &o_tree, AnimcharBaseComponent *_ac, float *pbc_wt_storage, int pbc_wt_cnt, const vec3f *ac_scale,
      intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *), void *irq_arg, vec3f w_trans) :
      origTree(o_tree),
      ac(_ac),
      acScale(ac_scale),
      worldTranslate(w_trans),
      irqFunc(irq),
      irqArg(irq_arg),
      blendWt(pbc_wt_storage, pbc_wt_cnt)
    {
      mem_set_0(blendWt);
    }
    intptr_t irq(int type, intptr_t p1, intptr_t p2, intptr_t p3)
    {
      return irqFunc ? irqFunc(type, p1, p2, p3, irqArg) : GIRQR_NoResponse;
    }
  };

public:
  AnimPostBlendCtrl(AnimationGraph &graph);
  ~AnimPostBlendCtrl();

  int getPbcId() const { return pbcId; }

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree) = 0;
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, Context &ctx) = 0;
  virtual void advance(IPureAnimStateHolder & /*st*/, real /*dt*/) {}

  virtual const char *class_name() const { return "AnimPostBlendCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendCtrlCID || IAnimBlendNode::isSubOf(id); }

  friend class AnimBlender;

private:
  AnimPostBlendCtrl &operator=(const AnimPostBlendCtrl &);
  friend class AnimationGraph;
};

class AnimationGraph;

//
// Animation blender
//
class AnimBlender
{
public:
  // hard restrictions on blending
  static constexpr int MAX_ANIMS_IN_NODE = 16;

  // weighted pos and rot channels
  struct WeightedNode
  {
    real blendWt[MAX_ANIMS_IN_NODE];
    uint8_t blendMod[((MAX_ANIMS_IN_NODE + 1 + 7) & ~7) - 1];
    uint8_t readyFlg;
  };

  struct NodeSamplers;

  struct NodeWeight
  {
    int totalNum;
    real wTotal;
  };

  struct PrsResult
  {
    vec3f pos, scl;
    quat4f rot;
  };

  struct ChannelMap
  {
    union
    {
      int *mapPool;
      int *chanNodeId[3];
    };

    ChannelMap()
    {
      chanNodeId[0] = NULL;
      chanNodeId[1] = NULL;
      chanNodeId[2] = NULL;
    }
    ChannelMap(const ChannelMap &) = delete;
    ~ChannelMap()
    {
      if (mapPool)
        delete mapPool;
      chanNodeId[0] = NULL;
      chanNodeId[1] = NULL;
      chanNodeId[2] = NULL;
    }

    void alloc(int c1, int c2, int c3)
    {
      if (mapPool)
        delete mapPool;

      mapPool = new (midmem) int[c1 + c2 + c3];
      chanNodeId[1] = mapPool + c1;
      chanNodeId[2] = chanNodeId[1] + c2;
    }
  };

  struct CharNodeModif
  {
    vec3f sScale;
    union
    {
      vec3f pScale_id;
      struct
      {
        int pad[3];
        int id;
      };
    };
    vec3f pOfs;

    int chanNodeId() const { return id; }
    void setChanNodeId(int id_) { id = id_; }
  };

  enum
  {
    RM_POS_B = (1 << 0),
    RM_POS_A = (1 << 1),
    RM_ROT_B = (1 << 2),
    RM_ROT_A = (1 << 3),
    RM_SCL_B = (1 << 4),
    RM_SCL_A = (1 << 5),
  };

  struct TlsContext
  {
    TlsContext *volatile nextCtx = NULL;
    volatile int64_t threadId = -1;

    vec3f originLinVel = ZERO<vec3f>(), originAngVel = ZERO<vec3f>();

    // weighted animation in blend node
    dag::Span<real> bnlWt;
    dag::Span<int> bnlCT;

    NodeSamplers *chXfm = NULL;
    WeightedNode *chPos = NULL, *chScl = NULL, *chRot = NULL;
    NodeWeight *wtPos = NULL, *wtScl = NULL, *wtRot = NULL;
    PrsResult *chPrs = NULL;

    dag::Span<uint8_t> readyMark; // combination of RM_***_* bits //== optimize this!

    // list of post-blend controllers and their current weight
    dag::Span<real> pbcWt;

    intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *) = nullptr;
    void *irqArg = nullptr;

    void *dataPtr = NULL;
    int dataPtrSz = 0;

    void rebuildNodeList(int bnl_count, int pbc_count, int target_node_count);
    void clear()
    {
      chPos = NULL;
      chScl = NULL;
      chRot = NULL;
      chPrs = NULL;
      bnlWt.reset();
      bnlCT.reset();
      readyMark.reset();
      pbcWt.reset();
      if (dataPtr)
      {
        midmem->free(dataPtr);
        dataPtr = NULL;
        dataPtrSz = 0;
      }
      nextCtx = NULL;
    }
  };

  PtrTab<AnimBlendNodeLeaf> bnl;
  PtrTab<AnimPostBlendCtrl> pbCtrl;
  FastNameMap targetNode;
  Tab<ChannelMap> bnlChan;
  static TlsContext mainCtx;
  static int mainCtxRefCount;
  bool nodeListValid = false;


public:
  AnimBlender();
  ~AnimBlender();

  TlsContext &selectCtx(intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *), void *irq_arg);

  void buildNodeList();
  void postBlendInit(IPureAnimStateHolder &st, const GeomNodeTree &tree);

  int registerBlendNodeLeaf(AnimBlendNodeLeaf *n);
  void unregisterBlendNodeLeaf(int id, AnimBlendNodeLeaf *n);

  int registerPostBlendCtrl(AnimPostBlendCtrl *n);
  void unregisterPostBlendCtrl(int id, AnimPostBlendCtrl *n);

  bool blend(TlsContext &tls, IPureAnimStateHolder &st, IAnimBlendNode *root, const CharNodeModif *cmm, const AnimationGraph &graph);
  void blendOriginVel(TlsContext &tls, IPureAnimStateHolder &st, IAnimBlendNode *root, bool rebuild_list);
  void postBlendProcess(TlsContext &tls, IPureAnimStateHolder &st, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  // valid just after blend() call
  bool getBlendedNodePRS(TlsContext &tls, int id, const vec3f **p, const quat4f **r, const vec3f **s, const vec4f *orig_prs);
  bool getBlendedNodeTm(TlsContext &tls, int id, mat44f &tm, const vec4f *orig_prs);

  static void releaseContextMemory();

protected:
  TlsContext *addNewContext(int64_t tid);
  TlsContext &getUnpreparedCtx()
  {
    if (is_main_thread())
      return mainCtx;

    int64_t tid = get_current_thread_id();
    TlsContext *ctx = interlocked_acquire_load_ptr(mainCtx.nextCtx);
    while (ctx)
      if (ctx->threadId == tid)
        return *ctx;
      else
        ctx = interlocked_acquire_load_ptr(ctx->nextCtx);

    return *addNewContext(tid);
  }
};


//
// Animation Graph holder
//
class AnimationGraph : public DObject
{
public:
  enum
  {
    PID_GLOBAL_TIME,    // for PT_TimeParam   "::GlobalTime"
    PID_GLOBAL_LAST_DT, // for PT_ScalarParam "::GlobalLastDT"
  };

  struct StateDestRec
  {
    Ptr<AnimBlendCtrl_Fifo3> fifo;
    int defNodemaskIdx;
    int condNodemaskIdx;
    int condNodemaskTarget;
    float morphTimeToSetNullOnSingleAnimFinished;
  };
  struct StateRec
  {
    enum
    {
      NODEID_NULL = -2,
      NODEID_LEAVECUR = -3
    };
    int nodeId;
    float morphTime;
    float forcedStateDur, forcedStateSpd;
    float minTimeScale, maxTimeScale;
  };

private:
  AnimBlender blender;
  Ptr<IAnimBlendNode> root;

  FastNameMap paramNames;
  Tab<uint8_t> paramTypes;
  Tab<int16_t> paramTimeInd;
  Tab<int16_t> paramFifo3Ind;

  FastNameMap nodeNames;
  PtrTab<IAnimBlendNode> nodePtr;

  FastNameMap rangeNames;

  Tab<int> ignoredAnimationNodes;
  DataBlock *initState;

  SmallTab<StateRec, MidmemAlloc> stRec;
  FastStrMap stateNames;
  SmallTab<StateDestRec, MidmemAlloc> stDest;
  Ptr<AnimBlendNodeNull> nullAnim;
  Tab<int> pbcWtParam;
#if DAGOR_DBGLEVEL > 0
  FastNameMap tagNames;
#endif
  enum
  {
    STATE_ALIAS_FLG = 0x40000000,
    STATE_TAG_SHIFT = 20,
    STATE_TAG_MASK = 0xFF << STATE_TAG_SHIFT
  };

public:
  Tab<AnimResManagerV20::NodeMask> dbgNodeMask;
  int resId = -1;

public:
  AnimationGraph();

  inline IAnimBlendNode *getRoot() { return root; }
  void setInitState(const DataBlock &b);
  const DataBlock *getInitState() const { return initState; }

  void initChannelsForStates(const DataBlock &stDescBlk, dag::ConstSpan<AnimResManagerV20::NodeMask *> nodemask);
  void initStates(const DataBlock &stDescBlk);
  dag::ConstSpan<StateDestRec> getStDest() const { return stDest; }
  dag::ConstSpan<StateRec> getStRec() const { return stRec; }

  const DataBlock &getEnumList(const char *enum_cls) const;

  void replaceRoot(IAnimBlendNode *new_root);
  void resetBlendNodes(IPureAnimStateHolder &st);
  void sortPbCtrl(const DataBlock &ord);
  inline void postBlendInit(IPureAnimStateHolder &st, const GeomNodeTree &tree) { blender.postBlendInit(st, tree); }
  inline void postBlendCtrlAdvance(IPureAnimStateHolder &st, float dt)
  {
    for (int i = 0; i < blender.pbCtrl.size(); i++)
      blender.pbCtrl[i]->advance(st, dt);
  }
  inline int getPbcWtParamCount() const { return pbcWtParam.size(); }

  int addParamId(const char *name, int type);
  int addInlinePtrParamId(const char *name, size_t size_bytes, int type = IPureAnimStateHolder::PT_InlinePtr);
  int addEffectorParamId(const char *name)
  {
    return addInlinePtrParamId(name, sizeof(IAnimStateHolder::EffectorVar), IAnimStateHolder::PT_Effector);
  }
  inline int addParamIdEx(const char *name, int type) { return (name && *name) ? addParamId(name, type) : -1; }
  int addPbcWtParamId(const char *name);

  int getParamId(const char *name, int type) const;
  int getParamId(const char *name) const { return paramNames.getNameId(name); }

  void setIgnoredAnimationNodes(const Tab<int> &nodes);
  bool isNodeWithIgnoredAnimation(int nodeId);

  inline int registerBlendNodeLeaf(AnimBlendNodeLeaf *n) { return blender.registerBlendNodeLeaf(n); }
  inline void unregisterBlendNodeLeaf(int id, AnimBlendNodeLeaf *n) { blender.unregisterBlendNodeLeaf(id, n); }

  inline int registerPostBlendCtrl(AnimPostBlendCtrl *n) { return blender.registerPostBlendCtrl(n); }
  inline void unregisterPostBlendCtrl(int id, AnimPostBlendCtrl *n) { blender.unregisterPostBlendCtrl(id, n); }

  int registerBlendNode(IAnimBlendNode *n, const char *name, const char *nm_suffix = NULL);
  void unregisterBlendNode(IAnimBlendNode *n);

  inline int addNamedRange(const char *name) { return rangeNames.addNameId(name); }
  inline int getNamedRangeId(const char *name) { return rangeNames.getNameId(name); }

  inline IAnimBlendNode *getBlendNodePtr(int id) const { return (unsigned)id < (unsigned)nodePtr.size() ? nodePtr[id] : NULL; }
  inline IAnimBlendNode *getBlendNodePtr(const char *name, const char *nm_suffix = NULL)
  {
    int id = getBlendNodeId(name, nm_suffix);
    if (id != -1)
      return nodePtr[id];
    return NULL;
  }
  inline const char *getBlendNodeName(const IAnimBlendNode *bn) const
  {
    if (bn && getBlendNodePtr(bn->getAnimNodeId()) == bn)
      return nodeNames.getName(bn->getAnimNodeId());
    for (int i = nodePtr.size() - 1; i >= 0; i--)
      if (nodePtr[i] == bn)
        return nodeNames.getName(i);
    return NULL;
  }

  inline int getBlendNodeId(const char *name, const char *nm_suffix = NULL) const
  {
    return nodeNames.getNameId(nm_suffix ? String(0, "%s:%s", name, nm_suffix).str() : name);
  }
  inline int getNodeId(const char *name)
  {
    blender.buildNodeList();
    return blender.targetNode.getNameId(name);
  }

  inline int getStateIdx(const char *state_name) const
  {
    int idx = stateNames.getStrId(state_name);
    return (idx < 0) ? idx : (idx & ~(STATE_ALIAS_FLG | STATE_TAG_MASK));
  }
  inline dag::ConstSpan<StateRec> getState(int state_idx)
  {
    return state_idx >= 0 ? make_span_const(stRec).subspan(state_idx * stDest.size(), stDest.size()) : make_span_const(stRec).first(0);
  }
  // TODO: decouple these two functions
  void enqueueState(IPureAnimStateHolder &st, dag::ConstSpan<StateRec> state, float force_dur = -1, float force_speed = -1);
  void setStateSpeed(IPureAnimStateHolder &st, dag::ConstSpan<StateRec> state, float force_speed);

  void onSingleAnimFinished(IPureAnimStateHolder &st, IAnimBlendNode *n);

  inline void allocateGlobalTimer()
  {
    G_VERIFY(PID_GLOBAL_TIME == addParamId("::GlobalTime", IPureAnimStateHolder::PT_TimeParam));
    G_VERIFY(PID_GLOBAL_LAST_DT == addParamId("::GlobalLastDT", IPureAnimStateHolder::PT_ScalarParam));
  }

  inline void debugSetIrqPosInAllNodes(const char *irq_name, float rel_pos)
  {
    for (int i = nodePtr.size() - 1; i >= 0; i--)
    {
      if (nodePtr[i])
        nodePtr[i]->debugSetIrqPos(irq_name, rel_pos);
    }
  }

  AnimBlender::TlsContext &selectBlenderCtx(intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *) = nullptr,
    void *irq_arg = nullptr)
  {
    return blender.selectCtx(irq, irq_arg);
  }

  // access to Bnl list and weights to enable post-blending processing (e.g. mapped sync)
  inline PtrTab<AnimBlendNodeLeaf> &getBnlList() { return blender.bnl; }

  // perform animation blending
  inline bool blend(AnimBlender::TlsContext &tls, IPureAnimStateHolder &st, const AnimBlender::CharNodeModif *cmm = NULL)
  {
    if (root)
      return blender.blend(tls, st, root, cmm, *this);
    return false;
  }
  inline void blendOriginVel(AnimBlender::TlsContext &tls, IPureAnimStateHolder &st, bool rebuild_list = true)
  {
    if (root)
      blender.blendOriginVel(tls, st, root, rebuild_list);
  }

  // valid just after blend() call
  inline bool getNodePRS(AnimBlender::TlsContext &tls, int id, const vec3f **p, const quat4f **r, const vec3f **s,
    const vec4f *orig_prs)
  {
    return blender.getBlendedNodePRS(tls, id, p, r, s, orig_prs);
  }
  inline bool getNodeTm(AnimBlender::TlsContext &tls, int id, mat44f &tm, const vec4f *orig_prs)
  {
    return blender.getBlendedNodeTm(tls, id, tm, orig_prs);
  }

  // post-blending processing
  inline void postBlendProcess(AnimBlender::TlsContext &tls, IPureAnimStateHolder &st, GeomNodeTree &tree,
    AnimPostBlendCtrl::Context &ctx)
  {
    blender.postBlendProcess(tls, st, tree, ctx);
  }

  // Debugging support
  int getParamCount() const { return paramNames.nameCount(); }
  const char *getParamName(int idx) const { return paramNames.getName(idx); }
  unsigned getParamType(int idx) const { return paramTypes[idx]; }
  const FastNameMap &getParamNames() const { return paramNames; }

  int getAnimNodeCount() const { return nodeNames.nameCount(); }
  const char *getAnimNodeName(int idx) const { return nodeNames.getName(idx); }
  const FastNameMap &getAnimNodeNames() const { return nodeNames; }

  int getBNLCount() const { return blender.bnl.size(); }
  int getPBCCount() const { return blender.pbCtrl.size(); }
  const char *getPBCName(int idx) const { return getBlendNodeName(blender.pbCtrl[idx]); }

  int getRangeCount() const { return rangeNames.nameCount(); }
  const char *getRangeName(int idx) const { return rangeNames.getName(idx); }

  int getStateCount() const { return stateNames.strCount(); }
  const char *getStateName(int idx) const { return stateNames.getMapRaw()[idx].name; }
  const char *getStateNameByStateIdx(int state_idx) const
  {
    dag::ConstSpan<FastStrMap::Entry> map = stateNames.getMapRaw();
    for (const FastStrMap::Entry *it = map.begin(); it != map.end(); ++it)
      if (it->id == state_idx)
        return it->name;
    return nullptr;
  }
  bool isStateNameAlias(int idx) const { return (stateNames.getMapRaw()[idx].id & STATE_ALIAS_FLG) != 0; }
  int getStateNameTag(int idx) const { return (stateNames.getMapRaw()[idx].id & STATE_TAG_MASK) >> STATE_TAG_SHIFT; }
#if DAGOR_DBGLEVEL > 0
  int getTagsCount() const { return tagNames.nameCount() + 1; }
  const char *getTagName(int tag) const { return tagNames.getName(tag - 1); }
#else
  int getTagsCount() const { return 1; }
  const char *getTagName(int) const { return NULL; }
#endif

  AnimDbgCtx *createDebugBlenderContext(const GeomNodeTree *gtree, bool dump_all_nodes = false);
  void destroyDebugBlenderContext(AnimDbgCtx *ctx);
  const DataBlock *getDebugBlenderState(AnimDbgCtx *ctx, IPureAnimStateHolder &st, bool dump_tm);
  const DataBlock *getDebugNodemasks(AnimDbgCtx *ctx);

  void getUsedBlendNodes(eastl::hash_map<IAnimBlendNode *, bool> &usedNodes);

  // RTTI
  virtual const char *class_name() const { return "AnimationGraph"; }
  virtual bool isSubOf(DClassID id) { return id == AnimationGraphCID; }

  friend class AnimCommonStateHolder;
  friend struct AnimDbgCtx;

protected:
  virtual ~AnimationGraph();
};

extern int getEnumValueByName(const char *name);
extern int addEnumValue(const char *name);
extern const char *getEnumName(int name_id);

extern int addIrqId(const char *irq_name);
extern const char *getIrqName(int irq_id);

extern void setDebugAnimParam(bool enable);
extern bool getDebugAnimParam();
extern int getDebugAnimParamIrqId();

struct AnimMap;
// very tiny interface for motion matching implementation
class IMotionMatchingController
{
public:
  virtual bool getPose(AnimBlender::TlsContext &tls, const Tab<AnimMap> &) const = 0;
};

} // end of namespace AnimV20
DAG_DECLARE_RELOCATABLE(AnimV20::AnimBlender::ChannelMap);


//
// Animation Manager (for sharing animation graphs)
//
namespace AnimResManagerV20
{
using namespace AnimV20;

AnimationGraph *loadUniqueAnimGraph(const DataBlock &blk, dag::ConstSpan<AnimData *> anim_list,
  const Tab<int> &nodesWithIgnoredAnimation);
__forceinline void releaseAnimGraph(AnimationGraph *anim)
{
  if (anim)
    anim->delRef();
}
} // end of namespace AnimResManagerV20

#define BUILD_BLENDING_LIST_PROLOGUE_BNL(BCTX, ST, WT) \
  IPureAnimStateHolder &ST = *BCTX.st;                 \
  real *WT = BCTX.wt

#define BUILD_BLENDING_LIST_PROLOGUE(BCTX, ST) \
  IPureAnimStateHolder &ST = *BCTX.st;         \
  if (BCTX.abnWt && getAnimNodeId() >= 0)      \
  BCTX.abnWt[getAnimNodeId()] += w
