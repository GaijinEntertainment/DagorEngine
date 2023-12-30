//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <anim/dag_animBlend.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <math/dag_interpolator.h>
#include <util/dag_index16.h>
#include <util/dag_simpleString.h>
#include <generic/dag_staticTab.h>

// forward declarations for external classes and structures
class DataBlock;

namespace AnimV20
{
typedef unsigned attachment_uid_t;

// declare classes IDs first
decl_dclassid(0x20000301, AnimPostBlendAlignCtrl);
decl_dclassid(0x20000302, AnimPostBlendRotateCtrl);
decl_dclassid(0x20000303, AnimPostBlendMoveCtrl);
decl_dclassid(0x20000304, AnimPostBlendCondHideCtrl);
decl_dclassid(0x20000305, ApbParamCtrl);
decl_dclassid(0x20000306, ApbAnimateCtrl);
decl_dclassid(0x20000307, LegsIKCtrl);
decl_dclassid(0x20000308, DeltaAnglesCtrl);
decl_dclassid(0x20000309, AnimPostBlendAlignExCtrl);
decl_dclassid(0x2000030A, AnimPostBlendAimCtrl);
decl_dclassid(0x2000030B, MultiChainFABRIKCtrl);
decl_dclassid(0x2000030C, AttachGeomNodeCtrl);
decl_dclassid(0x2000030D, AnimPostBlendNodeLookatCtrl);
decl_dclassid(0x2000030E, AnimPostBlendEffFromAttachement);
decl_dclassid(0x2000030F, AnimPostBlendNodesFromAttachement);
decl_dclassid(0x20000310, AnimPostBlendParamFromNode);
decl_dclassid(0x20000311, AnimPostBlendMatVarFromNode);
decl_dclassid(0x20000312, AnimPostBlendCompoundRotateShift);
decl_dclassid(0x20000313, AnimPostBlendSetParam);
decl_dclassid(0x20000314, AnimPostBlendTwistCtrl);
decl_dclassid(0x20000315, AnimPostBlendNodePosesSubstraction);
decl_dclassid(0x20000316, AnimPostBlendScaleCtrl);
decl_dclassid(0x20000317, DefClampParamCtrl);
decl_dclassid(0x20000318, AnimPostBlendRotateAroundCtrl);
decl_dclassid(0x20000319, AnimPostBlendNodeLookatNodeCtrl);
decl_dclassid(0x2000031A, DeltaRotateShiftCtrl);

//
// Controller to compute node's rotation and shift
//
class DeltaRotateShiftCtrl : public AnimPostBlendCtrl
{
  struct LocalData
  {
    dag::Index16 targetNode;
  };
  struct VarId
  {
    int yaw = -1, pitch = -1, lean = -1, ofsX = -1, ofsY = -1, ofsZ = -1;
  } varId;

  int localVarId = -1;
  bool relativeToOrig = false;
  SimpleString targetNode;

public:
  DeltaRotateShiftCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "DeltaRotateShiftCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == DeltaRotateShiftCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to compute delta angles
//
class DeltaAnglesCtrl : public AnimPostBlendCtrl
{
  int varId, destRotateVarId, destPitchVarId, destLeanVarId;
  SimpleString nodeName;
  int fwdAxisIdx, sideAxisIdx;
  float scaleR, scaleP, scaleL;
  bool invFwd, invSide;

public:
  DeltaAnglesCtrl(AnimationGraph &g) :
    AnimPostBlendCtrl(g),
    varId(-1),
    destRotateVarId(-1),
    destPitchVarId(-1),
    destLeanVarId(-1),
    fwdAxisIdx(0),
    sideAxisIdx(1),
    invFwd(false),
    invSide(false),
    scaleR(1.0f),
    scaleP(1.0f),
    scaleL(1.0f)
  {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder & /*st*/);

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "DeltaAnglesCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == DeltaAnglesCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple align controller: aligns target node as src node
//
class AnimPostBlendAlignCtrl : public AnimPostBlendCtrl
{
  struct NodeId
  {
    attachment_uid_t lastRefUid;
    dag::Index16 target, src;
  };

  mat33f tmRot, lr;
  SimpleString targetNodeName, srcNodeName;
  int srcSlotId = -1;
  int paramId = -1;
  int wScaleVarId = -1;
  bool wScaleInverted = false;
  bool useLocal = false;
  bool binaryWt = false;
  bool copyPos = false;
  bool copyBlendResultToSrc = false;

public:
  AnimPostBlendAlignCtrl(AnimationGraph &g, const char *var_name);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendAlignCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendAlignCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Advanced align controller: aligns helper node to root and distributes rotation among target nodes
//
class AnimPostBlendAlignExCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
    float wt;
  };
  struct NodeId
  {
    dag::Index16 hlp;
    struct
    {
      dag::Index16 target;
      uint8_t nodeRemap;
    } id[1];
  };

  Tab<NodeDesc> targetNode;
  SimpleString hlpNodeName;
  int hlpCol[3];
  int varId;

public:
  AnimPostBlendAlignExCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g), varId(-1)
  {
    hlpCol[0] = 1;
    hlpCol[1] = 2;
    hlpCol[2] = 3;
  }

  virtual void destroy() { targetNode.clear(); }

  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendAlignExCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendAlignExCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple rotating controller: rotates target node around given local axis
//
class AnimPostBlendRotateCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
    float wt;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap;
  };

  vec3f rotAxis, dirAxis;
  Tab<NodeDesc> targetNode;
  real kMul, kAdd, kCourseAdd;
  int varId, paramId;
  int axisCourseParamId;
  real kMul2, kAdd2;
  int addParam2Id, addParam3Id;
  bool leftTm;
  bool swapYZ;
  bool relRot;
  bool saveScale;
  int axisFromCharIndex;

public:
  AnimPostBlendRotateCtrl(AnimationGraph &g, const char *param_name, const char *ac_pname, const char *add_pname = NULL);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendRotateCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendRotateCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Rotate around another node controller: rotates target node around given node by given axis
//
class AnimPostBlendRotateAroundCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
    float wt;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap;
  };

  vec3f rotAxis;
  Tab<NodeDesc> targetNode;
  float kMul = 1.f;
  float kAdd = 0.f;
  int varId = -1;
  int paramId;

public:
  AnimPostBlendRotateAroundCtrl(AnimationGraph &g, const char *param_name);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendRotateAroundCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendRotateAroundCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};


//
// Simple scale controller: scale target node(s) along given axes
//
class AnimPostBlendScaleCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
    float wt;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap;
  };

  vec3f scaleAxis;
  Tab<NodeDesc> targetNode;
  int varId, paramId;
  float defaultValue;

public:
  AnimPostBlendScaleCtrl(AnimationGraph &g, const char *param_name, const char *ac_pname, const char *add_pname = NULL);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendScaleCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendScaleCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple move (translation) controller: moves target node(s) along given axes
//
class AnimPostBlendMoveCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
    float wt;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap;
  };

  vec3f rotAxis, dirAxis;
  Tab<NodeDesc> targetNode;
  real kMul, kAdd, kCourseAdd;
  int varId, paramId;
  int axisCourseParamId;
  enum FrameRef
  {
    FRAME_REF_LOCAL,
    FRAME_REF_PARENT,
    FRAME_REF_GLOBAL,
    FRAME_REF_MAX
  } frameRef;
  bool additive;
  bool saveOtherAxisMove;

public:
  AnimPostBlendMoveCtrl(AnimationGraph &g, const char *param_name, const char *ac_pname);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendMoveCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendMoveCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple hide controller: zero tm for node when coditions met
//
class AnimPostBlendCondHideCtrl : public AnimPostBlendCtrl
{
  struct CompCondition
  {
    int pid;
    int op;
    float p0, p1;
  };
  struct Condition
  {
    int op;
    CompCondition leaf;
    Tab<Condition *> branches;
  };
  struct NodeDesc
  {
    Condition condition;
    bool hideOnly;
    SimpleString name;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap;
  };

  Tab<NodeDesc> targetNode;
  int varId;

public:
  static bool loadCondition(const DataBlock &blk, AnimationGraph &graph, const char *name, int index, Condition &out_condition);
  static void deleteCondition(Condition &condition);
  static bool checkCondMet(const IPureAnimStateHolder &st, const Condition &condition);

  AnimPostBlendCondHideCtrl(AnimationGraph &g);
  ~AnimPostBlendCondHideCtrl();

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendCondHideCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendCondHideCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Simple aim controller
//
class AnimPostBlendAimCtrl : public AnimPostBlendCtrl
{
  struct AngleLimits
  {
    Point2 yaw = ZERO<Point2>();
    Point2 pitch = ZERO<Point2>();

    void load(const Point4 &row)
    {
      yaw.set(row.x, row.y);
      pitch.set(row.z, row.w);
    }

    void clamp(float &y, float &p) const;
  };

  struct AimDesc
  {
    SimpleString name;

    float defaultYaw = 0.f;
    float defaultPitch = 0.f;

    float yawSpeed = 0.f;
    float yawAccel = -1.f;

    float pitchSpeed = 0.f;
    float pitchAccel = -1.f;

    int minYawAngleId = -1;
    int maxYawAngleId = -1;
    int minPitchAngleId = -1;
    int maxPitchAngleId = -1;

    float stabilizerMaxDelta = 5.0;

    AngleLimits limits;
    Tab<AngleLimits> limitsTable;
  };

  struct AimState
  {
    float dt = 0.f;

    float yaw = 0.f;
    float pitch = 0.f;

    float yawVel = 0.f;
    float pitchVel = 0.f;

    float yawSpeed = 0.f;
    float pitchSpeed = 0.f;
    float yawSpeedMul = 1.f;
    float pitchSpeedMul = 1.f;
    float yawAccel = 0.f;
    float pitchAccel = 0.f;

    float yawDst = 0.f;
    float pitchDst = 0.f;

    dag::Index16 nodeId;

    bool lockYaw = false;
    bool lockPitch = false;
    AngleLimits limits;

    void init();

    void updateDstAngles(const vec4f &target_local_pos);
    void clampAngles(const AimDesc &desc, float &yaw_in_out, float &pitch_in_out);
    void updateRotation(const AimDesc &desc, float stab_error, bool stab_yaw, bool stab_pitch, float stab_yaw_mult,
      float stab_pitch_mult, float &prev_yaw, float &prev_pitch);
  };

  AimState aimState;
  AimDesc aimDesc;

  int varId = -1;

  int curYawId = -1;
  int curPitchId = -1;

  int curWorldYawId = -1;
  int curWorldPitchId = -1;

  int targetYawId = -1;
  int targetPitchId = -1;

  int yawSpeedId = -1;
  int pitchSpeedId = -1;
  int yawSpeedMulId = -1;
  int pitchSpeedMulId = -1;
  int yawAccelId = -1;
  int pitchAccelId = -1;

  int prevYawId = -1;
  int prevPitchId = -1;

  int hasStabId = -1;
  int stabYawId = -1;
  int stabPitchId = -1;
  int stabErrorId = -1;
  int stabYawMultId = -1;
  int stabPitchMultId = -1;

public:
  AnimPostBlendAimCtrl(AnimationGraph &g);

  virtual void destroy() { clear_and_shrink(aimDesc.limitsTable); }

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);
  virtual void advance(IPureAnimStateHolder &st, real dt);

  const char *class_name() const override { return "AnimPostBlendAimCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendAimCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to change state values procedurally
//
class ApbParamCtrl : public AnimPostBlendCtrl
{
public:
  struct ParamOp
  {
    int destPid;
    int type;
    int op;
    float p0;
    float p1;
    int v0Pid;
    int v1Pid;
    int skipNum;
    bool weighted;
  };

private:
  struct ChangeRec
  {
    int destPid, chRatePid, scalePid;
    float chRate, modVal;
  };
  struct RemapRec
  {
    int recPid, destPid, remapPid;
    InterpolateTabFloat remapVal;
  };
  Tab<ChangeRec> rec;
  Tab<RemapRec> mapRec;
  Tab<ParamOp> ops;
  int wtPid;
  int lastDtPid;

public:
  ApbParamCtrl(AnimationGraph &g, const char *param_name);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &st);
  virtual void advance(IPureAnimStateHolder &st, real dt);

  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &) {}
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "ApbParamCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == ApbParamCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

// controller for default value and clamp
class DefClampParamCtrl : public AnimPostBlendCtrl
{
private:
  struct Params
  {
    int destPid;
    float defaultValue;
    Point2 clampValue;
  };
  Tab<Params> ps;
  int wtPid;

public:
  DefClampParamCtrl(AnimationGraph &g, const char *param_name);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &st);

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &);
  virtual void process(IPureAnimStateHolder &st, real, GeomNodeTree &, AnimPostBlendCtrl::Context &);

  const char *class_name() const override { return "DefClampParamCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == DefClampParamCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to sample animations directly to nodes
//
class ApbAnimateCtrl : public AnimPostBlendCtrl
{
  struct NodeAnim
  {
    const AnimV20::AnimChanPoint3 *pos, *scl;
    const AnimV20::AnimChanQuat *rot;
    SimpleString name;
  };
  struct AnimRec
  {
    int pid;
    float pMul, pAdd;
    short animIdx, animCnt;
  };
  typedef dag::Index16 NodeId;

  Tab<AnimRec> rec;
  Tab<NodeAnim> anim;
  PtrTab<AnimV20::AnimData> usedAnim;
  int varId;

public:
  ApbAnimateCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g), rec(midmem), varId(-1) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  virtual bool isEmpty() const { return rec.empty(); }

  const char *class_name() const override { return "ApbAnimateCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == ApbAnimateCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Create ApbAnimateCtrl and AnimPostBlendRotateCtrl, but keep only one of them active
//

typedef void (*CreateAnimNodeFunc)(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix);

void createNodeApbAnimateAndPostBlendProc(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix,
  CreateAnimNodeFunc create_anim_node_func);

//
// Legs IK Controller
//
class LegsIKCtrl : public AnimPostBlendCtrl
{
  struct NodeId
  {
    dag::Index16 footId, kneeId, legId, footStepId;
    float dy, da, dt;
  };
  struct IKRec
  {
    vec3f vFootFwd, vFootSide;
    Point2 footStepRange, footRotRange;
    float maxFootUp, maxDyRate, maxDaRate;
    SimpleString foot, knee, leg, footStep;
    bool useAnimcharUpDir;
  };

  Tab<IKRec> rec;
  int varId;
  Point3 crawlKneeOffsetVec{Point3()};
  float crawlFootOffset{0.f}, crawlFootAngle{0.f}, crawlMaxRay{0.f};
  bool alwaysSolve{false}, isCrawl{false};

public:
  LegsIKCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g), rec(midmem), varId(-1), alwaysSolve(false) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder & /*st*/) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);
  virtual void advance(IPureAnimStateHolder &st, real dt);

  const char *class_name() const override { return "LegsIKCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == LegsIKCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Multi-chain FABRIK Controller
//
class MultiChainFABRIKCtrl : public AnimPostBlendCtrl
{
public:
  enum
  {
    MAX_NODES_PER_CTRL = 24,
    MAX_CHAINS = 6
  };
  typedef dag::Index16 chain_node_t;
  typedef StaticTab<chain_node_t, MAX_NODES_PER_CTRL> chain_tab_t;
  struct ChainSlice
  {
    uint8_t ofs = 0, cnt = 0;
  };

  typedef Tab<Tab<vec3f>> debug_state_t;

private:
  struct BodyTriangle
  {
    SimpleString node0, node1, add, sub;
  };
  struct BodyTriangleIds
  {
    dag::Index16 node0, node1, add, sub;
    int8_t chain0 = -1, chain1 = -1;
  };
  BodyTriangle bodyTriNames[2];
  SimpleString bodyChainEndsNames[2];
  Tab<SimpleString> chainEndsNames;

  struct PerAnimStateData
  {
    ChainSlice bodyChainSlice, chainSlice[MAX_CHAINS];
    int16_t chainSliceCnt = 0;
    BodyTriangleIds bodyTri[2];
    chain_tab_t nodeIds;

    dag::Span<chain_node_t> bodyChainNodeIds() { return make_span(nodeIds).subspan(bodyChainSlice.ofs, bodyChainSlice.cnt); }
    dag::Span<chain_node_t> chainNodeIds(int i) { return make_span(nodeIds).subspan(chainSlice[i].ofs, chainSlice[i].cnt); }
  };
  struct VarId
  {
    int eff, wScale;
    bool wScaleInverted;
  };
  Tab<VarId> effectorVarId;
  int bodyChainEffectorVarId = -1;
  int bodyChainEffectorEnd = 0;

  int perAnimStateDataVarId = -1;
  int dbgVarId[2] = {-1, -1};

public:
  MultiChainFABRIKCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &st);

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);
  virtual void advance(IPureAnimStateHolder &st, real dt);

  const char *class_name() const override { return "MultiChainFABRIKCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == MultiChainFABRIKCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// GeomNode attach Controller
//
class AttachGeomNodeCtrl : public AnimPostBlendCtrl
{
  struct PerAnimStateData
  {
    Tab<dag::Index16> nodeIds;
  };
  struct VarId
  {
    int nodeWtm, wScale;
    bool wScaleInverted;
  };
  vec3f minNodeScale = ZERO<vec3f>(), maxNodeScale = ZERO<vec3f>();
  Tab<SimpleString> nodeNames;
  Tab<VarId> destVarId;
  int perAnimStateDataVarId = -1;

public:
  struct AttachDesc
  {
    TMatrix wtm; //< destination (where we should attach node)
    float w;     //< weight of attach application (to perform smooth node matrix blending)
  };

public:
  AttachGeomNodeCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &st);

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);
  virtual void advance(IPureAnimStateHolder &st, real dt);

  const char *class_name() const override { return "AttachGeomNodeCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AttachGeomNodeCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);

  static AttachDesc &getAttachDesc(IPureAnimStateHolder &st, int var_id) { return *(AttachDesc *)st.getInlinePtr(var_id); }
};


//
// Node dir controller: calculates orientation from position of node and target
//
class AnimPostBlendNodeLookatCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap = -1;
  };

  Tab<NodeDesc> targetNodes;
  mat33f tmRot, itmRot;
  int varId = -1;
  int paramXId = -1, paramYId = -1, paramZId = -1;
  bool leftTm = false;
  bool swapYZ = false;

public:
  AnimPostBlendNodeLookatCtrl(AnimationGraph &g, const char *param_name);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendNodeLookatCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendNodeLookatCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};


//
// Node dir controller: calculates orientation for target nodes from positions of lookat node and up node
//
class AnimPostBlendNodeLookatNodeCtrl : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString name;
  };
  struct NodeId
  {
    dag::Index16 target;
    short nodeRemap = -1;
  };

  Tab<NodeDesc> targetNodes;
  SimpleString lookatNodeName;
  SimpleString upNodeName;
  dag::Index16 lookatNodeId;
  dag::Index16 upNodeId;
  int lookatAxis = -1, upAxis = -1;
  bool negAxes[3] = {false};
  int varId = -1;
  bool valid = false;

public:
  AnimPostBlendNodeLookatNodeCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendNodeLookatNodeCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendNodeLookatNodeCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to setup effector by nodes of attached objects
//
class AnimPostBlendEffFromAttachement : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString src, dest;
  };
  struct LocalData
  {
    attachment_uid_t lastRefUid;
    struct NodePair
    {
      dag::Index16 src, dest;
    };
    NodePair node[2];
  };
  struct VarId
  {
    int eff, effWt, wtm, wtmWt, wScale;
    bool wScaleInverted;
  };

  int varId = -1;
  int namedSlotId = -1;
  int varSlotId = -1;
  bool ignoreZeroWt = false;
  Tab<VarId> destVarId;
  Tab<NodeDesc> nodes;

public:
  AnimPostBlendEffFromAttachement(AnimationGraph &g);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendEffFromAttachement"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendEffFromAttachementCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Produces new effector and wtm for parent in depends of its new child pos
// as if parent pos and rotate were fixed relative to child (link constrain)
//
class AnimPostBlendNodeEffectorFromChildIK : public AnimPostBlendCtrl
{
  struct LocalData
  {
    dag::Index16 parentNodeId;
    dag::Index16 childNodeId;
    int srcVarId;
    int destVarId;
    int destEffId;
  };
  int varSlotId = -1;
  int localDataVarId = -1;
  int resetEffByValId = -1;
  bool resetEffInvVal = false;
  SimpleString parentNodeName, childNodeName, srcVarName, destVarName;

public:
  AnimPostBlendNodeEffectorFromChildIK(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendNodeEffectorFromChildIK"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendMatVarFromNodeCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};
//
// Controller to copy node matrix to mat animvar
//
class AnimPostBlendMatVarFromNode : public AnimPostBlendCtrl
{
  struct LocalData
  {
    attachment_uid_t lastRefDestUid, lastRefSrcUid;
    int destVarId;
    dag::Index16 srcNodeId;
  };

  int varId = -1;
  int destSlotId = -1, srcSlotId = -1;
  int destVarWtId = -1;
  SimpleString destVarName, srcNodeName;

public:
  AnimPostBlendMatVarFromNode(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}

  virtual void init(IPureAnimStateHolder &st, const GeomNodeTree &tree);
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendMatVarFromNode"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendMatVarFromNodeCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to copy nodes subtree from attached object
//
class AnimPostBlendNodesFromAttachement : public AnimPostBlendCtrl
{
  struct NodeDesc
  {
    SimpleString srcNode, destNode;
    bool recursive = false, includingRoot = false;
  };
  struct LocalData
  {
    attachment_uid_t lastRefUid;
    SmallTab<dag::Index16, MidmemAlloc> nodePairs;
  };

  int varId = -1;
  int namedSlotId = -1;
  int varSlotId = -1;
  bool copyWtm = false;
  bool wScaleInverted = false;
  int wScaleVarId = -1;
  Tab<NodeDesc> nodes;

public:
  AnimPostBlendNodesFromAttachement(AnimationGraph &g);

  virtual void destroy() {}

  virtual void reset(IPureAnimStateHolder &);

  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &) {}
  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  const char *class_name() const override { return "AnimPostBlendNodesFromAttachement"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendNodesFromAttachementCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to set parameter value from animation (local node tm)
//
class AnimPostBlendParamFromNode : public AnimPostBlendCtrl
{
  struct LocalData
  {
    attachment_uid_t lastRefUid;
    dag::Index16 nodeId;
  };

  int varId = -1;
  int slotId = -1;
  int destVarId = -1;
  int destVarWtId = -1;
  float defVal = 0;
  SimpleString nodeName;
  bool invertVal = false;

public:
  AnimPostBlendParamFromNode(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}
  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &) {}

  const char *class_name() const override { return "AnimPostBlendParamFromNode"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendParamFromNodeCID || AnimPostBlendCtrl::isSubOf(id); }

  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Compound align+rotate+shift controller
//
class AnimPostBlendCompoundRotateShift : public AnimPostBlendCtrl
{
  struct LocalData
  {
    dag::Index16 targetNode, alignAsNode, moveAlongNode;
  };
  struct VarId
  {
    int yaw = -1, pitch = -1, lean = -1, ofsX = -1, ofsY = -1, ofsZ = -1;
  } varId;
  struct Scales
  {
    float yaw = 1, pitch = 1, lean = 1, ofsX = 1, ofsY = 1, ofsZ = 1;
  } scale;
  int localVarId = -1;
  SimpleString targetNode, alignAsNode, moveAlongNode;
  mat33f tmRot[2];

public:
  AnimPostBlendCompoundRotateShift(AnimationGraph &g);

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}
  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &);

  const char *class_name() const override { return "AnimPostBlendParamFromNode"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendCompoundRotateShiftCID || AnimPostBlendCtrl::isSubOf(id); }

  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to set parameter directly (to local or attached animchar)
//
class AnimPostBlendSetParam : public AnimPostBlendCtrl
{
  int destSlotId = -1, destVarId = -1, srcVarId = -1;
  SimpleString destVarName;
  float val = 0, minWeight = 1e-4;

public:
  AnimPostBlendSetParam(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}
  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &) {}

  const char *class_name() const override { return "AnimPostBlendSetParam"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendSetParamCID || AnimPostBlendCtrl::isSubOf(id); }

  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

//
// Controller to compute twist nodes for skeleton
//
class AnimPostBlendTwistCtrl : public AnimPostBlendCtrl
{
  SimpleString node0, node1;
  Tab<SimpleString> twistNodes;
  int varId = -1;
  float angDiff = 0;
  bool optional = false;

public:
  AnimPostBlendTwistCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void reset(IPureAnimStateHolder &) {}
  virtual void init(IPureAnimStateHolder &, const GeomNodeTree &);

  const char *class_name() const override { return "AnimPostBlendTwistCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == AnimPostBlendTwistCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  virtual void process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};

} // end of namespace AnimV20
