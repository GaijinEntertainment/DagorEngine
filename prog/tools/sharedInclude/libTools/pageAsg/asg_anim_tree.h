//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_math3d.h>
#include <libTools/pageAsg/asg_decl.h>
#include <ioSys/dag_dataBlock.h>


// blend node leaf object
struct AnimObjBnl
{
  enum
  {
    TYPE_Continuous,
    TYPE_Single,
    TYPE_Still,
    TYPE_Parametric,

    TYPE__NUM,
  };
  static const char *typeName[];

  String a2dfname;
  String name;
  String nodemask;
  int type;

  struct Label
  {
    String name;
    String keyStart, keyEnd;
    real relStart, relEnd;
  };

  struct Stripes
  {
    bool start;
    Tab<real> cp;

  public:
    Stripes() : start(true), cp(midmem) {}

    int findInd(real pos)
    {
      real dif = MAX_REAL;
      int ind = -1, i;
      for (i = 0; i < cp.size(); i++)
        if (fabs(cp[i] - pos) < dif)
        {
          dif = fabs(cp[i] - pos);
          ind = i;
        }
      return i;
    }

    void clear()
    {
      start = true;
      cp.clear();
    }

    int add(real pos)
    {
      append_items(cp, 1, &pos);
      sort();
      return findInd(pos);
    }

    void sort() { ::sort(cp, &realSort); }

    static int realSort(const real *p1, const real *p2)
    {
      if (*p1 > *p2)
        return 1;
      if (*p1 < *p2)
        return -1;
      return 0;
    }
    bool isDefault() const { return start && !cp.size(); }
  };

  String keyStart, keyEnd, syncTime, nodeMask;
  String varname, timeScaleParam;
  bool eoaIrq, ownTimer, disableOriginVel, additive, rewind;
  bool foreignAnimation;
  real duration;
  real moveDist;

  real p0, p1, mulK, addK;
  bool looping;
  bool updOnVarChange;

  Tab<Label> labels;
  DataBlock irqBlks;
  struct
  {
    real dirh, dirv, dist;
  } addMove;

public:
  AnimObjBnl() : labels(midmem) { fillDefaults(); }
  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;

  void fillDefaults();
  void applySettings();

protected:
  void loadNamedRanges(Tab<AnimObjBnl::Label> &labels, const DataBlock &blk);
  void saveNamedRanges(const Tab<AnimObjBnl::Label> &labels, DataBlock &blk) const;
  void loadStripes(Stripes &s, const DataBlock &blk);
  void saveStripes(const Stripes &s, DataBlock &blk) const;
  bool areKeysReducable() const;
};

// base blend node ctrl object
struct AnimObjCtrl
{
  enum
  {
    TYPE_Null,
    TYPE_Fifo,
    TYPE_Fifo3,
    TYPE_Linear,
    TYPE_RandomSwitch,
    TYPE_ParamSwitch,
    TYPE_ParamSwitchS,
    TYPE_Hub,
    TYPE_DirectSync,
    TYPE_Planar,
    TYPE_Blender,
    TYPE_BIS,
    TYPE_LinearPoly,
    TYPE_Stub,
    TYPE_AlignNode,
    TYPE_RotateNode,
    TYPE_MoveNode,
    TYPE_ParamsChange,
    TYPE_CondHideNode,
    TYPE_AnimateNode,

    TYPE__NUM,
  };
  static const char *typeName[];

  String name;
  int type;
  bool used;
  DataBlock props;

public:
  struct RecParamInfo
  {
    enum ParamType
    {
      STR_PARAM,
      INT_PARAM,
      REAL_PARAM,
      BOOL_PARAM,
      BNL_PARAM,
      CTRL_PARAM
    };
    ParamType type;
    String name;
    int w;
    RecParamInfo() : type(REAL_PARAM), name("Unknown"), w(5) {}
  };
  void fillDefaults();
  virtual void addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const = 0;
  virtual void load(const DataBlock &blk) { props = blk; }
  virtual void save(DataBlock &blk, const char *suffix) const { blk = props; }
  virtual ~AnimObjCtrl() {}

  // editing iface
  virtual bool canAdd() { return false; }
  virtual int getListCount() const { return 0; }
  virtual void insertRec(int i) {}
  virtual void eraseRec(int i) {}
  virtual int getRecParamsCount() const { return 0; }
  virtual RecParamInfo getRecParamInfo(int i) const { return RecParamInfo(); }
  virtual void getParamValueText(int param_no, int rec_no, String &text) const { text = "???"; }
  virtual void setParamValueText(int param_no, int rec_no, const char *text) {}
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const { val = 222; }
  virtual void setParamValueReal(int param_no, int rec_no, const real &val) {}
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    name = source->name;
    type = source->type;
    used = source->used;
    props = source->props;
  }
};

AnimObjCtrl *create_controller(int type);

// node mask list
struct AnimObjNodeMask
{
  NameMap nm;
  String name;

  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;
};

struct AnimObjExportContext
{
  struct BnlClone
  {
    const char *suffix;
    const char *nodemask;
  };
  Tab<BnlClone> clones;
  NameMap bnls;
  NameMap a2d;
  NameMap controllers;
  NameMap a2d_repl;

public:
  AnimObjExportContext() : clones(tmpmem) {}
};

// animation graph tree
struct AnimObjGraphTree
{
  NameMap a2dList;
  Tab<AnimObjNodeMask *> nodemask;
  Tab<AnimObjBnl *> bnlList;
  Tab<AnimObjCtrl *> ctrlList;
  DataBlock preview;
  DataBlock initAnimState;
  DataBlock stateDesc;
  String root;
  bool dontUseAnim;
  bool dontUseStates;
  bool forceNoStates;

  String sharedNodeMaskFName, sharedCtrlFName, sharedBnlFName;

public:
  AnimObjGraphTree() : nodemask(midmem), bnlList(midmem), ctrlList(midmem), forceNoStates(false) { fillDefaults(); }
  ~AnimObjGraphTree() { clear(); }
  void clear();
  void fillDefaults();
  void load(const DataBlock &blk);
  void save(DataBlock &blk, NameMap *a2d_map = NULL);

  // solid export of data
  void compact();
  void exportTree(DataBlock &blk, const AnimObjExportContext &ctx) const;
  void exportAnimCtrl(DataBlock &blk, const AnimObjExportContext &ctx) const;

  AnimObjBnl *findBnl(const char *name);
  AnimObjCtrl *findCtrl(const char *name);
  bool animNodeExists(const char *name);

protected:
  void loadBnl(const DataBlock &blk, const char *a2d);
  void loadBn(const DataBlock &blk, bool use);

  void loadNodemasks(const DataBlock &blk);
  void loadAnimCtrl(const DataBlock &blk);
  void loadAnimBnl(const DataBlock &blk);
  void saveNodemasks(DataBlock &blk) const;
  void saveAnimCtrl(DataBlock &blk) const;
  void saveAnimBnl(DataBlock &blk, NameMap *a2d_map);


  void addBnl(const char *name, const char *a2d, int type, const DataBlock &blk);
};
