//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <libTools/pageAsg/asg_anim_tree.h>

// various controllers
struct AnimObjCtrlFifo : public AnimObjCtrl
{
  String varname;

public:
  AnimObjCtrlFifo() { type = TYPE_Fifo; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlFifo *)source)->varname;
  }
};

struct AnimObjCtrlFifo3 : public AnimObjCtrlFifo
{
  AnimObjCtrlFifo3() { type = TYPE_Fifo3; }
};

struct AnimObjCtrlLinear : public AnimObjCtrl
{
  struct Rec
  {
    String node;
    real start, end;
  };
  Tab<Rec> list;
  String varname;

public:
  AnimObjCtrlLinear() : list(midmem) { type = TYPE_Linear; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 3; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlLinear *)source)->varname;
    list = ((AnimObjCtrlLinear *)source)->list;
  }
};

struct AnimObjCtrlLinearPoly : public AnimObjCtrl
{
  struct Rec
  {
    String node;
    real p0;
  };
  Tab<Rec> list;
  String varname;

public:
  AnimObjCtrlLinearPoly() : list(midmem) { type = TYPE_LinearPoly; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 2; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlLinearPoly *)source)->varname;
    list = ((AnimObjCtrlLinearPoly *)source)->list;
  }
};

struct AnimObjCtrlRandomSwitch : public AnimObjCtrl
{
  struct Rec
  {
    String node;
    real wt;
    int maxRepeat;
  };
  Tab<Rec> list;
  String varname;

public:
  AnimObjCtrlRandomSwitch() : list(midmem) { type = TYPE_RandomSwitch; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 3; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlRandomSwitch *)source)->varname;
    list = ((AnimObjCtrlRandomSwitch *)source)->list;
  }
};

struct AnimObjCtrlParametricSwitch : public AnimObjCtrl
{
  struct Rec
  {
    String node;
    real r0;
    real r1;
  };
  Tab<Rec> list;
  String varname;
  real morphTime = 0.0f;

public:
  AnimObjCtrlParametricSwitch() : list(midmem) { type = TYPE_ParamSwitch; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 3; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlParametricSwitch *)source)->varname;
    list = ((AnimObjCtrlParametricSwitch *)source)->list;
    morphTime = ((AnimObjCtrlParametricSwitch *)source)->morphTime;
  }
};

struct AnimObjCtrlParametricSwitchS : public AnimObjCtrlParametricSwitch
{
  AnimObjCtrlParametricSwitchS() { type = TYPE_ParamSwitchS; }
};

struct AnimObjCtrlHub : public AnimObjCtrl
{
  struct Rec
  {
    String node;
    real w;
    bool enabled;
  };
  Tab<Rec> list;
  bool _const = false;

public:
  AnimObjCtrlHub() : list(midmem) { type = TYPE_Hub; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 3; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    _const = ((AnimObjCtrlHub *)source)->_const;
    list = ((AnimObjCtrlHub *)source)->list;
  }
};

struct AnimObjCtrlDirectSync : public AnimObjCtrl
{
  struct Rec
  {
    String node, linkedTo;
    real dc, scale;
    real clamp0, clamp1;
  };
  Tab<Rec> list;
  String varname;

public:
  AnimObjCtrlDirectSync() : list(midmem) { type = TYPE_DirectSync; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  virtual int getListCount() const { return list.size(); }
  virtual int getRecParamsCount() const { return 6; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlDirectSync *)source)->varname;
    list = ((AnimObjCtrlDirectSync *)source)->list;
  }
};

struct AnimObjCtrlNull : public AnimObjCtrl
{
  AnimObjCtrlNull() { type = TYPE_Null; }
  virtual void load(const DataBlock &blk) {}
  void save(DataBlock &blk, const char *suffix) const override {}
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
};

struct AnimObjCtrlBlender : public AnimObjCtrl
{
  String varname;
  String node1, node2;
  real duration = 0;
  real morph = 0;

public:
  AnimObjCtrlBlender() { type = TYPE_Blender; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return 2; }
  virtual int getRecParamsCount() const { return 1; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlBlender *)source)->varname;
    node1 = ((AnimObjCtrlBlender *)source)->node1;
    node2 = ((AnimObjCtrlBlender *)source)->node2;
    duration = ((AnimObjCtrlBlender *)source)->duration;
    morph = ((AnimObjCtrlBlender *)source)->morph;
  }
};

struct AnimObjCtrlBIS : public AnimObjCtrl
{
  String varname;
  String node1, node2, fifo;
  real morph1 = 0.0f, morph2 = 0.0f;
  int maskAnd = 0, maskEq = 0;

public:
  AnimObjCtrlBIS() { type = TYPE_BIS; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  virtual int getListCount() const { return 3; }
  virtual int getRecParamsCount() const { return 1; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    varname = ((AnimObjCtrlBIS *)source)->varname;
    node1 = ((AnimObjCtrlBIS *)source)->node1;
    node2 = ((AnimObjCtrlBIS *)source)->node2;
    fifo = ((AnimObjCtrlBIS *)source)->fifo;
    morph1 = ((AnimObjCtrlBIS *)source)->morph1;
    morph2 = ((AnimObjCtrlBIS *)source)->morph2;
    maskAnd = ((AnimObjCtrlBIS *)source)->maskAnd;
    maskEq = ((AnimObjCtrlBIS *)source)->maskEq;
  }
};

struct AnimObjCtrlAlignNode : public AnimObjCtrl
{
  String src, target;
  Point3 rot_euler;
  Point3 scale;
  bool useLocal = false;

public:
  AnimObjCtrlAlignNode() { type = TYPE_AlignNode; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  virtual int getListCount() const { return 1; }
  virtual int getRecParamsCount() const { return 2; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    src = ((AnimObjCtrlAlignNode *)source)->src;
    target = ((AnimObjCtrlAlignNode *)source)->target;
    rot_euler = ((AnimObjCtrlAlignNode *)source)->rot_euler;
    useLocal = ((AnimObjCtrlAlignNode *)source)->useLocal;
    scale = ((AnimObjCtrlAlignNode *)source)->scale;
  }
};

struct AnimObjCtrlRotateNode : public AnimObjCtrl
{
  struct NodeDesc
  {
    String name;
    float wt;
    NodeDesc() : wt(1.0f) {}
  };
  Tab<NodeDesc> target;
  String axisCourse, paramName;
  real kMul = 0, kAdd = 0, kCourseAdd = 0;
  Point3 rotAxis, dirAxis;
  real defWt = 0;
  bool leftTm = false;

public:
  AnimObjCtrlRotateNode() : target(uimem) { type = TYPE_RotateNode; }
  virtual void load(const DataBlock &blk);
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  virtual int getListCount() const { return target.size(); }
  virtual int getRecParamsCount() const { return 1; }
  virtual RecParamInfo getRecParamInfo(int i) const;
  virtual void insertRec(int i);
  virtual void eraseRec(int i);
  virtual void getParamValueText(int param_no, int rec_no, String &text) const;
  virtual void setParamValueText(int param_no, int rec_no, const char *text);
  virtual void getParamValueReal(int param_no, int rec_no, real &val) const;
  virtual void setParamValueReal(int param_no, int rec_no, const real &val);
  virtual bool canAdd() { return true; }
  virtual void virtualCopy(const AnimObjCtrl *source)
  {
    __super::virtualCopy(source);
    paramName = ((AnimObjCtrlRotateNode *)source)->paramName;
    target = ((AnimObjCtrlRotateNode *)source)->target;
    axisCourse = ((AnimObjCtrlRotateNode *)source)->axisCourse;
    kMul = ((AnimObjCtrlRotateNode *)source)->kMul;
    kAdd = ((AnimObjCtrlRotateNode *)source)->kAdd;
  }
};

struct AnimObjCtrlMoveNode : public AnimObjCtrl
{
public:
  AnimObjCtrlMoveNode() { type = TYPE_MoveNode; }
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
};

struct AnimObjCtrlParamsChange : public AnimObjCtrl
{
public:
  AnimObjCtrlParamsChange() { type = TYPE_ParamsChange; }
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
};

struct AnimObjCtrlHideNode : public AnimObjCtrl
{
public:
  AnimObjCtrlHideNode() { type = TYPE_CondHideNode; }
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
};

struct AnimObjAnimateNode : public AnimObjCtrl
{
public:
  AnimObjAnimateNode() { type = TYPE_AnimateNode; }
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
};
