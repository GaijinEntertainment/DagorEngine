//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animBlend.h>
#include <util/dag_string.h>

// forward declarations for external classes and structures
class DataBlock;

namespace AnimV20
{

// declare classes IDs first
decl_dclassid(0x20000101, AnimBlendNodeNull);

decl_dclassid(0x20000102, AnimBlendNodeContinuousLeaf);
decl_dclassid(0x20000103, AnimBlendNodeStillLeaf);
decl_dclassid(0x20000104, AnimBlendNodeParametricLeaf);
decl_dclassid(0x20000105, AnimBlendNodeSingleLeaf);

decl_dclassid(0x20000151, AnimBlendCtrl_1axis);
decl_dclassid(0x20000153, AnimBlendCtrl_Fifo3);
decl_dclassid(0x20000154, AnimBlendCtrl_RandomSwitcher);
decl_dclassid(0x20000155, AnimBlendCtrl_Hub);
decl_dclassid(0x20000157, AnimBlendCtrl_Blender);
decl_dclassid(0x20000158, AnimBlendCtrl_BinaryIndirectSwitch);
decl_dclassid(0x20000159, AnimBlendCtrl_LinearPoly);
decl_dclassid(0x2000015A, AnimBlendCtrl_ParametricSwitcher);
decl_dclassid(0x2000015B, AnimBlendCtrl_SetMotionMatchingTag);


//
// Null blend node (usefull for fadeout anim in Fifo controller and
//                  all other cases when blend node is needed but it should do nothing)
//
class AnimBlendNodeNull : public IAnimBlendNode
{
public:
  AnimBlendNodeNull() {}

  virtual void destroy() {}

  virtual void buildBlendingList(BlendCtx & /*bctx*/, real /*w*/) {}
  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  const char *class_name() const override { return "AnimBlendNodeNull"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeNullCID || IAnimBlendNode::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple 1-axis blend controller
//
class AnimBlendCtrl_1axis : public IAnimBlendNode
{
public:
  struct AnimSlice
  {
    Ptr<IAnimBlendNode> node;
    real start, end;
  };

protected:
  Tab<AnimSlice> slice;
  int paramId;

public:
  AnimBlendCtrl_1axis(AnimationGraph &graph, const char *param_name);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual real getAvgSpeed(IPureAnimStateHolder &st) { return slice.size() ? slice[0].node->getAvgSpeed(st) : 0; }
  virtual int getTimeScaleParamId(IPureAnimStateHolder &st) { return slice.size() ? slice[0].node->getTimeScaleParamId(st) : -1; }

  // creation-time routines
  void addBlendNode(IAnimBlendNode *n, real start, real end);

  const char *class_name() const override { return "AnimBlendCtrl_1axis"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_1axisCID || IAnimBlendNode::isSubOf(id); }

  dag::ConstSpan<AnimSlice> getChildren() const { return slice; }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;
};


//
// FIFO-3 blend controller (fast and simple; max queue length - 3 items)
//
class AnimBlendCtrl_Fifo3 : public IAnimBlendNode
{
protected:
  int fifoParamId;

public:
  AnimBlendCtrl_Fifo3(AnimationGraph &graph, const char *fifo_param_name);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);

  // run-time routines
  void enqueueState(IPureAnimStateHolder &st, IAnimBlendNode *n, real overlap_time, real max_lag = 0.1);
  void resetQueue(IPureAnimStateHolder &st, bool leave_cur_state);
  bool isEnqueued(IPureAnimStateHolder &st, IAnimBlendNode *n);

  const char *class_name() const override { return "AnimBlendCtrl_Fifo3"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_Fifo3CID || IAnimBlendNode::isSubOf(id); }
  int getParamId() const { return fifoParamId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Random switcher blend controller
//
class AnimBlendCtrl_RandomSwitcher : public IAnimBlendNode
{
public:
  struct RandomAnim
  {
    Ptr<IAnimBlendNode> node;
    real rndWt;
    int maxRepeat;
  };

protected:
  Tab<RandomAnim> list;
  int paramId, repParamId;

public:
  AnimBlendCtrl_RandomSwitcher(AnimationGraph &graph, const char *param_name);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual bool isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n);
  virtual real getDuration(IPureAnimStateHolder &st);

  virtual void seekToSyncTime(IPureAnimStateHolder &st, real offset);
  virtual void pause(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual bool isInRange(IPureAnimStateHolder &st, int rangeId);
  virtual real getAvgSpeed(IPureAnimStateHolder &st) { return list.size() ? list[0].node->getAvgSpeed(st) : 0; }
  virtual int getTimeScaleParamId(IPureAnimStateHolder &st) { return list.size() ? list[0].node->getTimeScaleParamId(st) : -1; }

  // creation-time routines
  void addBlendNode(IAnimBlendNode *n, real rnd_w, int max_rep);
  void recalcWeights(bool reverse = false);

  // run-time routines
  void setRandomAnim(IPureAnimStateHolder &st);
  bool setAnim(IPureAnimStateHolder &st, IAnimBlendNode *n);

  const char *class_name() const override { return "AnimBlendCtrl_RandomSwitcher"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_RandomSwitcherCID || IAnimBlendNode::isSubOf(id); }

  inline IAnimBlendNode *getCurAnim(IPureAnimStateHolder &st);
  dag::ConstSpan<RandomAnim> getChildren() const { return list; }
  int getParamId() const { return paramId; }
  int getRepParamId() const { return repParamId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;
};

//
// Parametric switcher blend controller
//
class AnimBlendCtrl_ParametricSwitcher : public IAnimBlendNode
{
public:
  struct ItemAnim
  {
    Ptr<IAnimBlendNode> node;
    real range[2];
    real baseVal;
  };

protected:
  real morphTime;
  Tab<ItemAnim> list;
  Tab<int> rewindBitmapParamsIds;
  int paramId;
  int residualParamId;
  int lastAnimParamId;
  int fifoParamId;
  bool continuousAnimMode;

public:
  AnimBlendCtrl_ParametricSwitcher(AnimationGraph &graph, const char *param_name, const char *res_pname = NULL);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual bool isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n);
  virtual real getAvgSpeed(IPureAnimStateHolder &st)
  {
    int currAnim = getAnimForRange(st.getParam(paramId));
    return currAnim >= 0 && currAnim < list.size() ? list[currAnim].node->getAvgSpeed(st) : 0;
  }
  virtual int getTimeScaleParamId(IPureAnimStateHolder &st)
  {
    int currAnim = getAnimForRange(st.getParam(paramId));
    return currAnim >= 0 && currAnim < list.size() ? list[currAnim].node->getTimeScaleParamId(st) : 0;
  }

  // creation-time routines
  void addBlendNode(IAnimBlendNode *n, real range0, real range1, real bval, AnimationGraph &graph, const char *ctrl_name);

  // run-time routines
  int getAnimForRange(real range);
  const char *class_name() const override { return "AnimBlendCtrl_ParametricSwitcher"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_ParametricSwitcherCID || IAnimBlendNode::isSubOf(id); }

  dag::ConstSpan<ItemAnim> getChildren() const { return list; }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;
};

//
// Hub blend controller
//
class AnimBlendCtrl_Hub : public IAnimBlendNode
{
protected:
  PtrTab<IAnimBlendNode> nodes;
  int paramId = -1;
  Tab<float> defNodeWt; // default properties; applied on reset

public:
  void finalizeInit(AnimationGraph &graph, const char *param_name);

  void destroy() override {}

  // general blending interface
  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual bool isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n);

  virtual void seekToSyncTime(IPureAnimStateHolder &st, real offset);
  virtual void pause(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual bool isInRange(IPureAnimStateHolder &st, int rangeId);

  // creation-time routines
  void addBlendNode(IAnimBlendNode *n, bool active, real wt);

  // run-time routines
  void setBlendNodeWt(IPureAnimStateHolder &st, IAnimBlendNode *n, real wt);
  void setBlendNodesWt(IPureAnimStateHolder &st, dag::ConstSpan<int> node_indexes, real wt)
  {
    if (paramId == -1)
      return;
    float *ptr = (float *)st.getInlinePtr(paramId);
    for (int nodeIdx : node_indexes)
      ptr[nodeIdx] = wt;
  }

  int getNodeIndex(IAnimBlendNode *n);

  virtual const char *class_name() const { return "AnimHubCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_HubCID || IAnimBlendNode::isSubOf(id); }

  dag::ConstSpan<Ptr<IAnimBlendNode>> getChildren() const { return nodes; }
  dag::ConstSpan<float> getDefNodeWt() const { return defNodeWt; }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;

protected:
  inline const float *getProps(IPureAnimStateHolder &st);
};

//
// Simple blender
//
class AnimBlendCtrl_Blender : public IAnimBlendNode
{
protected:
  static constexpr int NODES_NUM = 2;
  Ptr<IAnimBlendNode> node[NODES_NUM];
  real duration, blendTime;

  int paramId;

public:
  AnimBlendCtrl_Blender(AnimationGraph &graph, const char *param_name);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual bool isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n);
  virtual real getDuration(IPureAnimStateHolder &st);
  virtual real tell(IPureAnimStateHolder &st);

  virtual void seekToSyncTime(IPureAnimStateHolder &st, real offset);
  virtual void pause(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual bool isInRange(IPureAnimStateHolder &st, int rangeId);

  // creation-time routines
  void setBlendNode(int n, IAnimBlendNode *node);
  void setDuration(real dur);
  void setBlendTime(real dur);
  real getBlendTime() const { return blendTime; }

  dag::ConstSpan<Ptr<IAnimBlendNode>> getChildren() const { return dag::ConstSpan<Ptr<IAnimBlendNode>>(node, NODES_NUM); }

  // run-time routines

  const char *class_name() const override { return "AnimBlendCtrl_Blender"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_BlenderCID || IAnimBlendNode::isSubOf(id); }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;
};

//
// Binary indirect switch: checks condition and sets one of anims to fifo ctrl
//
class AnimBlendCtrl_BinaryIndirectSwitch : public IAnimBlendNode
{
protected:
  static constexpr int NODES_NUM = 2;
  Ptr<IAnimBlendNode> node[NODES_NUM];
  Ptr<AnimBlendCtrl_Fifo3> fifo;
  real morphTime[2];
  int paramId;
  int maskAnd, maskEq;

public:
  AnimBlendCtrl_BinaryIndirectSwitch(AnimationGraph &graph, const char *param_name, int mask_and, int mask_eq);

  virtual void destroy();

  // general blending interface
  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);

  // creation-time routines
  void setBlendNodes(IAnimBlendNode *n1, real n1_mtime, IAnimBlendNode *n2, real n2_mtime);
  void setFifoCtrl(AnimBlendCtrl_Fifo3 *n);

  virtual const char *class_name() const { return "AnimBISCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_BinaryIndirectSwitchCID || IAnimBlendNode::isSubOf(id); }

  dag::ConstSpan<Ptr<IAnimBlendNode>> getChildren() const { return dag::ConstSpan<Ptr<IAnimBlendNode>>(node, NODES_NUM); }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;
};

//
// Simplest linear blend controller
//
class AnimBlendCtrl_LinearPoly : public IAnimBlendNode
{
public:
  struct AnimPoint
  {
    Ptr<IAnimBlendNode> node;
    real p0;
    int wtPid;

    void applyWt(BlendCtx &bctx, real wt, real node_w);
  };

protected:
  Tab<AnimPoint> poly;
  Tab<int> rewindBitmapParamsIds;
  int paramId;
  real morphTime;
  real paramTau;
  real paramSpeed;
  int t0Pid;
  int curPosPid;
  bool enclosed;

public:
  AnimBlendCtrl_LinearPoly(AnimationGraph &graph, const char *param_name, const char *ctrl_name);

  virtual void destroy();

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);
  virtual bool validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n);
  virtual void collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map);
  virtual bool isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n);
  virtual void pause(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual real getDuration(IPureAnimStateHolder &st);
  virtual real getAvgSpeed(IPureAnimStateHolder &st) { return poly.size() ? poly[0].node->getAvgSpeed(st) : 0; }
  virtual int getTimeScaleParamId(IPureAnimStateHolder &st) { return poly.size() ? poly[0].node->getTimeScaleParamId(st) : -1; }

  // creation-time routines
  void addBlendNode(IAnimBlendNode *n, real p0, AnimationGraph &graph, const char *ctrl_name);

  const char *class_name() const override { return "AnimBlendCtrl_LinearPoly"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_LinearPolyCID || IAnimBlendNode::isSubOf(id); }

  dag::ConstSpan<AnimPoint> getChildren() const { return poly; }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);
  virtual void initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix) override;
  virtual void checkHasLoop(AnimationGraph &graph,
    eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes) override;

protected:
  static int anim_point_p0_cmp(const AnimPoint *p1, const AnimPoint *p2);
};

//
// Special controller that activates tags for Motion Matching
//
class AnimBlendCtrl_SetMotionMatchingTag : public IAnimBlendNode
{
  int tagsMaskParamId = -1;
  int tagIdx = -1;
  String tagName;

public:
  static constexpr int MAX_TAGS_COUNT = 64;
  virtual void destroy() {}
  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder & /*st*/) {}
  const char *class_name() const override { return "AnimBlendCtrl_SetMotionMatchingTag"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendCtrl_SetMotionMatchingTagCID || IAnimBlendNode::isSubOf(id); }
  static void createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);

  const char *getTagName() const { return tagName; }
  void setTagIdx(int idx) { tagIdx = idx; }
};


// Irq position struct to be used in BNLs
struct IrqPos
{
  int irqId;
  int irqTime;

  static void setupIrqs(Tab<IrqPos> &irqs, AnimData *anim, int t0, int dt, const DataBlock &blk, const char *anim_suffix);
  static int cmp_irq_time_id(const IrqPos *a, const IrqPos *b)
  {
    if (int d = a->irqTime - b->irqTime)
      return d;
    return a->irqId - b->irqId;
  }
  static void debug_set_irq_pos(const char *irq_name, float rel_pos, Tab<IrqPos> &irqs, int t0, int dt);
};

//
// Different types of blend node leafs:
//   * Continuous(in time) blending
//   * Still animation
//   * Parametric animation
//
class AnimBlendNodeContinuousLeaf : public AnimBlendNodeLeaf
{
protected:
  int paramId;
  int t0, dt;
  real rate, duration;
  real syncTime;
  real avgSpeed;
  bool startOffsetEnabled;
  bool eoaIrq, canRewind;

  Tab<IrqPos> irqs;

public:
  AnimBlendNodeContinuousLeaf(AnimationGraph &graph, AnimData *a, const char *time_param_name);

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void seekToSyncTime(IPureAnimStateHolder &st, real offset);
  virtual void pause(IPureAnimStateHolder &st);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  virtual void reset(IPureAnimStateHolder &st);
  virtual void seek(IPureAnimStateHolder &st, real rel_pos);
  virtual real tell(IPureAnimStateHolder &st);

  virtual bool isInRange(IPureAnimStateHolder &st, int rangeId);
  virtual real getDuration(IPureAnimStateHolder & /*st*/) { return duration; }
  virtual real getAvgSpeed(IPureAnimStateHolder & /*st*/) { return avgSpeed; }

  inline int calcAnimTimePos(float time_pos) { return t0 + int(time_pos * rate) % dt; }

  // creation-time routines
  void setAnim(AnimData *a);
  void setRange(int tStart, int tEnd, real anim_time, float move_dist, const char *name);
  void setRange(const char *keyStart, const char *keyEnd, real anim_time, float move_dist, const char *name);
  void setSyncTime(const char *syncKey);
  void setSyncTime(real stime);
  void setEndOfAnimIrq(bool on);
  void enableRewind(bool en);
  void enableStartOffset(bool en);
  bool isStartOffsetEnabled();
  void setupIrq(const DataBlock &blk, const char *anim_suffix) { IrqPos::setupIrqs(irqs, anim, t0, dt, blk, anim_suffix); }
  void debugSetIrqPos(const char *irq_name, float rel_pos) override { IrqPos::debug_set_irq_pos(irq_name, rel_pos, irqs, t0, dt); }

  const char *class_name() const override { return "AnimBlendNodeContinuousLeaf"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeContinuousLeafCID || AnimBlendNodeLeaf::isSubOf(id); }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation, bool def_foreign,
    const char *nm_suffix);
};

class AnimBlendNodeSingleLeaf : public AnimBlendNodeContinuousLeaf
{
public:
  AnimBlendNodeSingleLeaf(AnimationGraph &graph, AnimData *a, const char *time_param_name);
  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void seekToSyncTime(IPureAnimStateHolder &st, real offset);
  virtual void resume(IPureAnimStateHolder &st, bool rewind);
  const char *class_name() const override { return "AnimBlendNodeSingleLeaf"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeSingleLeafCID || AnimBlendNodeLeaf::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation, bool def_foreign,
    const char *nm_suffix);
};

class AnimBlendNodeStillLeaf : public AnimBlendNodeLeaf
{
  int ctPos;

public:
  AnimBlendNodeStillLeaf(AnimationGraph &graph, AnimData *a, int ct) : AnimBlendNodeLeaf(graph, a) { ctPos = ct; }

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder & /*st*/) {}
  virtual real getAvgSpeed(IPureAnimStateHolder & /*st*/) { return 0; }

  // creation-time routines
  void setAnim(AnimData *a);
  void setPos(int t) { ctPos = t; }
  int getPos() const { return ctPos; }

  const char *class_name() const override { return "AnimBlendNodeStillLeaf"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeStillLeafCID || AnimBlendNodeLeaf::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation, bool def_foreign,
    const char *nm_suffix);
};


class AnimBlendNodeParametricLeaf : public AnimBlendNodeLeaf
{
  int paramId;
  int paramLastId;
  int t0, dt;
  real p0, dp;
  real paramMulK, paramAddK;
  bool looping;
  bool updateOnParamChanged;

  Tab<IrqPos> irqs;

public:
  AnimBlendNodeParametricLeaf(AnimationGraph &graph, AnimData *a, const char *param_name, bool upd_pc);

  virtual void buildBlendingList(BlendCtx &bctx, real w);
  virtual void reset(IPureAnimStateHolder &st);

  virtual bool isInRange(IPureAnimStateHolder &st, int rangeId);
  virtual real getDuration(IPureAnimStateHolder & /*st*/) { return 0.0; }
  virtual real tell(IPureAnimStateHolder &st);

  // creation-time routines
  void setAnim(AnimData *a);
  void setRange(int t0, int t1, real p0, real p1, const char *name);
  void setRange(const char *keyStart, const char *keyEnd, real p0, real p1, const char *name);
  void setParamKoef(real mulk, real addk);
  void setLooping(bool loop);
  void setupIrq(const DataBlock &blk, const char *anim_suffix)
  {
    IrqPos::setupIrqs(irqs, anim, t0, dt, blk, anim_suffix);
    if (irqs.size() && paramLastId < 0)
      paramLastId = graph.addParamId(String(0, ":%s:p", graph.getParamName(paramId)), IPureAnimStateHolder::PT_ScalarParamInt);
  }
  void debugSetIrqPos(const char *irq_name, float rel_pos) override { IrqPos::debug_set_irq_pos(irq_name, rel_pos, irqs, t0, dt); }

  const char *class_name() const override { return "AnimBlendNodeParametricLeaf"; }
  virtual bool isSubOf(DClassID id) { return id == AnimBlendNodeParametricLeafCID || AnimBlendNodeLeaf::isSubOf(id); }
  int getParamId() const { return paramId; }

  static void createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation, bool def_foreign,
    const char *nm_suffix);
};
} // end of namespace AnimV20
