// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "materialRec.h"

#include <util/dag_string.h>
#include <util/dag_globDef.h>


class MaterialParamDescr;
class DataBlock;


class MatParam
{
public:
  MatParam(const MaterialParamDescr &descr) : paramDescr(descr) {}
  virtual ~MatParam() {}

  const char *getParamName() const;

  virtual void setScript(MaterialRec &mat) {}

protected:
  const MaterialParamDescr &paramDescr;
};


class MatTexture : public MatParam
{
public:
  String texAsset;

  MatTexture(const MaterialParamDescr &descr) : MatParam(descr) {}

  int getSlot() const;
};


class MatTripleInt : public MatParam
{
public:
  bool useDef;
  int val;

  MatTripleInt(const MaterialParamDescr &descr) : MatParam(descr), useDef(true), val(0) {}

  virtual void setScript(MaterialRec &mat);
};


class MatTripleReal : public MatParam
{
public:
  bool useDef;
  real val;

  MatTripleReal(const MaterialParamDescr &descr) : MatParam(descr), useDef(true), val(0) {}

  virtual void setScript(MaterialRec &mat);
};


class MatE3dColor : public MatParam
{
public:
  bool useDef;
  E3DCOLOR val;

  MatE3dColor(const MaterialParamDescr &descr) : MatParam(descr), useDef(true), val(0) {}

  virtual void setScript(MaterialRec &mat);
};


class MatCombo : public MatParam
{
public:
  String val;

  MatCombo(const MaterialParamDescr &descr) : MatParam(descr) {}

  virtual void setScript(MaterialRec &mat);
};


class MatCustom : public MatParam
{
public:
  enum CustomType
  {
    CUSTOM_2SIDED,
  };

  MatCustom(const MaterialParamDescr &descr) : MatParam(descr) {}

  virtual CustomType getCustomType() const = 0;
};


class Mat2Sided : public MatCustom
{
public:
  enum TwoSided
  {
    TWOSIDED_NONE,
    TWOSIDED,
    TWOSIDED_REAL,
  };

  TwoSided val;

  Mat2Sided(const MaterialParamDescr &descr) : MatCustom(descr), val(TWOSIDED_NONE) {}

  CustomType getCustomType() const { return CUSTOM_2SIDED; }

  virtual void setScript(MaterialRec &mat);
};
