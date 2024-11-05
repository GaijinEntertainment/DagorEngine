//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/pageAsg/asg_decl.h>
#include <libTools/pageAsg/asg_anim_tree.h>
#include <libTools/pageAsg/asg_node_mgr.h>
#include <math/dag_math3d.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <stdio.h>


// Branch condition
enum AnimGraphBranchType
{
  AGBT_EnterCondition,
  AGBT_CheckAtEnd,
  AGBT_CheckAlways,
};

struct AnimGraphCondition
{
  String targetName;
  String condition;
  bool customMorph;
  real customMorphTime;

public:
  AnimGraphCondition();

  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;
};


// Action that graph state can perform at specified timerange
struct AnimGraphTimedAction
{
  enum
  { // note: used in save/load
    AGTA_AT_Start,
    AGTA_AT_End,
    AGTA_AT_Time,
    AGTA_AT_Leave,
  };

  int performAt;     // type of "at time" representation
  bool relativeTime; // =true when preciseTime is in range (0..1) meaning start/end of anim; =false when time absolute
  bool cyclic;       // used when relativeTime==true, executes actions on each cycle of anim
  real preciseTime;  // time when action is to be performed (for AGTA_AT_Time)

  struct Action
  {
    enum
    { // note: used in save/load
      AGA_Nop,
      AGA_ClrFlag,
      AGA_SetFlag,
      AGA_GenIrq,
      AGA_SetParam,
    };
    int type;          // type from AGA_
    String subject;    // flag name, irq name or param name
    String expression; // expression for SetParam
    String p1;
    bool useParams;

    Action()
    {
      type = AGA_Nop;
      useParams = false;
    }
  };
  Tab<Action *> actions;

public:
  AnimGraphTimedAction();
  ~AnimGraphTimedAction();

  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;
};


// State
struct AnimGraphState
{
  // general state properties
  String name;
  Tab<AnimGraphCondition *> enterCond; // entrance conditions (generally only one condition)
  Tab<AnimGraphCondition *> endCond;   // conditions to be checked at state end
  Tab<AnimGraphCondition *> whileCond; // conditions to be checked while state is active
  Tab<String> animNode;                // anim node names for different anim channels
  NameMap classMarks;

  Tab<AnimGraphTimedAction *> actions; // actions to be performed during state

  int gt; // Graph thread idx
  struct
  {
    real dirh, dirv, vel;
  } addVel;

  bool undeletable, locked, useAnimForAllChannel, splitAnim, resumeAnim;
  bool animBasedActionTiming;

  // custom properties
  DataBlock *props; // can be NULL or can point to DataBlock inside main DataBlock - no destruction needed

  // editing and viewing properties
  struct
  {
    int belongsTo; // effective parent Id
    int headOf;    // effective self Id
  } groupId;

  struct
  {
    IPoint2 belongsTo; // position in group state belogs to
    IPoint2 headOf;    // position in group state is head of
  } nodePos;

  IPoint2 groupOrigin;


public:
  AnimGraphState(DataBlock *_props, int chan_num);
  ~AnimGraphState() { clear(); }

  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;

  void clear();

  void renameState(const char *old_name, const char *new_name);

  int getStateId() const { return groupId.headOf; }
  int getParentStateId() const { return groupId.belongsTo; }

  void addConditions(NameMap &list);

protected:
  void loadConditions(Tab<AnimGraphCondition *> &cond, const DataBlock &blk);
  void saveConditions(const Tab<AnimGraphCondition *> &cond, DataBlock &blk) const;
};

// Full scene (graph of states)
struct AsgStatesGraph : public IAsgGraphNodeManager
{
  AnimObjGraphTree tree;

  Tab<AnimGraphState *> states;
  DataBlock statePropsContainer;
  SmallTab<int, MidmemAlloc> stateToNode;

  IPoint2 rootGroupOrigin;
  float forcedStateMorphTime = 0.0;

  // conditions
  NameMap usedCondList;
  Tab<String> codeForCond;

  NameMap flagList;  // flags (may be used in conditions)
  NameMap classList; // classes of states
  NameMap irqList;   // irq names

  struct GraphThread
  {
    String name;
    String startNodeName;
  };
  Tab<GraphThread> gt;

  struct AnimChannel
  {
    String name;
    String fifoCtrl;
    // default suffix and nodemask for that suffix;
    // used to automatically generate BNLs
    String defSuffix;
    String defNodemask;
    float defMorphTime;
  };
  Tab<AnimChannel> chan;
  AsgStateGenParamGroup *rootScheme;

  struct PreviewAttach
  {
    String res;
    String tree;
    String node;
  };
  Tab<PreviewAttach> attachments;

public:
  AsgStatesGraph() : states(midmem), codeForCond(midmem), gt(midmem), chan(midmem), rootScheme(NULL), attachments(midmem) {}
  ~AsgStatesGraph() { clear(); }

  void clear();
  void clearWithDefaults();

  bool load(const DataBlock &blk, const DataBlock *blk_tree, const DataBlock *blk_sg, bool forceNoStates = false);

  bool save(DataBlock &blk, DataBlock *blk_tree, DataBlock *blk_sg);

  bool buildAndSave(const char *blk_fname, const char *rel_pdl, const char *res_name, int dbg_level, bool in_editor);

  // compacts (reorders items) and can optionally covert one prev id into new one
  int compact(int prev_group_id = -1);
  void complementCodeTab();

  bool checkValid();

  void addLink(int src_id, int dest_id, AnimGraphBranchType cond_type, const char *condition, bool customMorph, real customMorphTime);
  void addLink(int src_id, const char *dest_name, AnimGraphBranchType cond_type, const char *condition, bool customMorph,
    real customMorphTime);
  void delLinks(int src_id, int dest_id, AnimGraphBranchType cond_type);
  int getConditionsCount(int src_id, int dest_id, AnimGraphBranchType cond_type);

  void delOneLink(int state_id, int cond_idx, AnimGraphBranchType cond_type);
  void moveLink(int state_id, int cond_idx, AnimGraphBranchType cond_type, int dir);

  const char *getParentGroupName(AnimGraphState *s);

  void setWndPosition(int groupId, const IPoint2 &pos);
  void getWndPosition(int groupId, IPoint2 &pos) const;

  bool isValidId(int id) { return id >= 0 && id < states.size(); }
  Tab<AnimGraphCondition *> *getCond(int state_id, AnimGraphBranchType cond_type);

  bool exportCppCode(const char *fname, int dbg, AsgStateGenParamGroup *rs, const char *res_name);

  void incGtCount();
  void decGtCount();
  void incChanCount();
  void decChanCount();

  // notifications for related objects update
  void renameState(const char *old_name, const char *new_name);
  void renameAnimNode(const char *old_name, const char *new_name);
  void renameA2dFile(const char *old_name, const char *new_name);

  // IAsgGraphNodeManager interface
  virtual AnimGraphState *getState(int state_id);
  virtual int findState(const char *name);
  virtual int getNodeId(int state_id);

  void getBnlsNeeded(NameMap &bnls, NameMap &a2d, NameMap &ctrls);

protected:
  const char *translator = nullptr;
  unsigned gtMaskForMissingAnimWithAbsPos = 0;

  bool generateClassCode(const char *fname);

  void implement_canEnter(FILE *fp, int i, const AsgVarsList &vars);
  void implement_checkAlways(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id);
  void implement_checkAtEnd(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id);
  void implement_leave(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id);
  void implement_getStateExProps(FILE *fp, int i, AsgStateGenParamGroup *rs);
  void implement_go_checked(FILE *fp, int i, const AsgVarsList &vars);
  void implement_checkClass(FILE *fp, int i, const AsgVarsList &vars);
  void implement_checking(FILE *fp, int i, const Tab<AnimGraphCondition *> &cond, const AsgVarsList &vars, AsgLocalVarsList &locals,
    bool ret_bool);
  void implement_actionsCheckAndPerform(FILE *fp, int i, const AsgVarsList &vars, AsgLocalVarsList &locals,
    int context /*0=start, 1=always, 2=end, 3=leave*/, int anim_node_id);
  void implement_flagsDeclaration(FILE *fp);
  void implement_declareStaticVars(FILE *fp);
  int get_anim_node_id(int i, int *nodeMap);

  // performs splitting and reverse operation; used in buildAndSave
  void splitAnimChannels();
  void gatherBnlsNeeded(NameMap &bnls, NameMap &a2d, NameMap &ctrls);
  void restoreAnimChannels();

  bool checkAnimNodeExists(const char *name);

  static void loadNamemap(const DataBlock &blk, const char *varname, NameMap &nm);
  static void saveNamemap(DataBlock &blk, const char *varname, const NameMap &nm);
};
