//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/************************************************************************
  simple param block
************************************************************************/

// PBlock & DataBlock selective crossing
// indexing with four-cc codes

#include <util/dag_globDef.h>
#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>

// forward declarations of external classes
class String;
class DataBlock;


#define MAKE_PARAM(x) (SimplePBlock::ParamId) _MAKE4C(x)

class SimplePBlock
{
public:
  typedef unsigned ParamId;

  SimplePBlock();

  // init block from blk (only from current level, no subblocks are supported)
  SimplePBlock(const DataBlock &blk);
  void loadFromBlk(const DataBlock &blk);

  // clear all aprams
  void clear();

  // get params
  real getReal(ParamId param, real def = 0.0f) const;
  int getInt(ParamId param, int def = 0) const;
  bool getBool(ParamId param, bool def = false) const;
  E3DCOLOR getE3dColor(ParamId param, const E3DCOLOR def = E3DCOLOR(255, 255, 255)) const;
  const Point3 &getPoint3(ParamId param, const Point3 &def = Point3(0, 0, 0)) const;
  const char *getString(ParamId param, const char *def = NULL) const;

  // set params
  void setReal(ParamId param, real v);
  void setInt(ParamId param, int v);
  void setBool(ParamId param, bool v);
  void setE3dColor(ParamId param, const E3DCOLOR v);
  void setPoint3(ParamId param, const Point3 &v);
  void setString(ParamId param, const char *v);

  // get param id from string
  ParamId getParamId(const char *s) const;

  // assigment
  SimplePBlock(const SimplePBlock &other);
  SimplePBlock &operator=(const SimplePBlock &other);

  // debugging
  void dump(String &ret) const;

private:
  struct ParamDesc
  {
    ParamId id;
    int type;
    union Value
    {
      real r;
      int i;
      bool b;
      int c;
      float p[3];
      char *s;
    } val;

    void setString(const char *s);
    ParamDesc();
    ~ParamDesc();
  };

  Tab<ParamDesc> params;

  int getParamIndex(ParamId param) const;
};
DAG_DECLARE_RELOCATABLE(SimplePBlock::ParamDesc);
