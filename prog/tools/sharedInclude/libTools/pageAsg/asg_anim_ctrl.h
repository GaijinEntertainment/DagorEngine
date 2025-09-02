//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/pageAsg/asg_anim_tree.h>

// various controllers
struct AnimObjCtrlFifo : public AnimObjCtrl
{
  String varname;

public:
  AnimObjCtrlFifo() { type = TYPE_Fifo; }
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 3; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 2; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 3; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 3; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 3; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  int getListCount() const override { return list.size(); }
  int getRecParamsCount() const override { return 6; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
    varname = ((AnimObjCtrlDirectSync *)source)->varname;
    list = ((AnimObjCtrlDirectSync *)source)->list;
  }
};

struct AnimObjCtrlNull : public AnimObjCtrl
{
  AnimObjCtrlNull() { type = TYPE_Null; }
  void load(const DataBlock &blk) override {}
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return 2; }
  int getRecParamsCount() const override { return 1; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override;
  int getListCount() const override { return 3; }
  int getRecParamsCount() const override { return 1; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  int getListCount() const override { return 1; }
  int getRecParamsCount() const override { return 2; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk, const char *suffix) const override;
  void addNeededBnls(NameMap &b, NameMap &a2d, AnimObjGraphTree &t, const char *suf) const override {}
  int getListCount() const override { return target.size(); }
  int getRecParamsCount() const override { return 1; }
  RecParamInfo getRecParamInfo(int i) const override;
  void insertRec(int i) override;
  void eraseRec(int i) override;
  void getParamValueText(int param_no, int rec_no, String &text) const override;
  void setParamValueText(int param_no, int rec_no, const char *text) override;
  void getParamValueReal(int param_no, int rec_no, real &val) const override;
  void setParamValueReal(int param_no, int rec_no, const real &val) override;
  bool canAdd() override { return true; }
  void virtualCopy(const AnimObjCtrl *source) override
  {
    AnimObjCtrl::virtualCopy(source);
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
