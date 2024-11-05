// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  local variable support
/************************************************************************/

#include <math/dag_color.h>
#include <generic/dag_tab.h>
#include "shExpr.h"
#include "shVarVecTypes.h"

//************************************************************************
//* forwards
//************************************************************************

class ScriptedShaderElement;

/*********************************
 *
 * class LocalVar
 *
 *********************************/
struct LocalVar
{
public:
  union ConstValue
  {
    shc::Col4 c;
    real r;
    ConstValue() : c{0, 0, 0, 0} {}
    ConstValue(float r, float g, float b, float a) : c{r, g, b, a} {}
  };

  ConstValue cv;
  int32_t reg : 28 = -1;
  bool isConst : 1 = false;
  bool isDynamic : 1 = false;
  bool isInteger : 1 = false;
  bool dependsOnDynVarsAndMaterialParams : 1 = false;
  const int varNameId = -1;
  shexpr::ValueType valueType;
  LocalVar() = default;

  inline LocalVar(int _varNameId, shexpr::ValueType val_type, bool is_integer) :
    varNameId(_varNameId), valueType(val_type), isInteger(is_integer)
  {}
}; // class LocalVar
//

/*********************************
 *
 * class LocalVarTable
 *
 *********************************/
class LocalVarTable
{
public:
  // ctor/dtor
  LocalVarTable();
  ~LocalVarTable();

  // clear all table
  void clear();

  // add new local variable (return variable ID)
  int addVariable(const LocalVar &var);

  // return local variable ID by name (-1, if failed)
  int getVariableId(int var_name_id) const;

  //  // return local variable ID by ptr (-1, if failed)
  //  int getVariableId(LocalVar* var);

  // return local variable by name (NULL, if failed)
  LocalVar *getVariableByName(int var_name_id) { return getVariableById(getVariableId(var_name_id)); }
  const LocalVar *getVariableByName(int var_name_id) const
  {
    return const_cast<LocalVarTable *>(this)->getVariableByName(var_name_id);
  }


  // return local variable by ID (NULL, if failed)
  LocalVar *getVariableById(int var_id) { return (var_id < 0 || var_id >= varTable.size()) ? NULL : &varTable[var_id]; }
  const LocalVar *getVariableById(int var_id) const { return const_cast<LocalVarTable *>(this)->getVariableById(var_id); }


  // create copy
  void copyFrom(const LocalVarTable &c);

  inline dag::ConstSpan<LocalVar> getVars() const { return varTable; }
  inline int getCount() const { return varTable.size(); }

private:
  Tab<LocalVar> varTable;

private:
  LocalVarTable(const LocalVarTable &other) : varTable(midmem) { operator=(other); }

  LocalVarTable &operator=(const LocalVarTable &other) { return *this; }

  void addBuiltinConstants();
}; // class LocalVarTable
