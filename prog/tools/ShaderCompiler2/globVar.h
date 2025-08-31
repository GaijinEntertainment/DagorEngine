// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <math/integer/dag_IPoint4.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_sampler.h>
#include "varTypes.h"
#include "shaderSave.h"
#include "shVarVecTypes.h"
#include "intervals.h"
#include "hashStrings.h"

struct RaytraceTopAccelerationStructure;

namespace shc
{
class TargetContext;
}

namespace ShaderGlobal
{

union StVarValue
{
  shc::Col4 c4;
  shc::Int4 i4;
  real r;
  int i;
  struct
  {
    unsigned texId;
    BaseTexture *tex;
  };
  struct
  {
    unsigned bufId;
    Sbuffer *buf;
  };
  RaytraceTopAccelerationStructure *tlas;
  d3d::SamplerInfo samplerInfo;
  StVarValue() : i4{0, 0, 0, 0} {}
};

class Var
{
public:
  StVarValue value;
  NameId<VarMapAdapter> nameId;
  uint8_t type;
  bool isAlwaysReferenced, shouldIgnoreValue;
  bool isImplicitlyReferenced;
  bindump::string fileName;
  int array_size = 0;
  int index = 0;

  Var() : isAlwaysReferenced(false), shouldIgnoreValue(false), isImplicitlyReferenced(false) { memset(&value, 0, sizeof(value)); }

  bool operator==(const Var &right) const;
  bool operator!=(const Var &right) const { return !(operator==(right)); }
};

const char *get_var_name(const Var &variable, const shc::TargetContext &ctx);

class VarTable
{
  Tab<int> variableByNameId{midmem_ptr()};
  Tab<Var> variableList{midmem_ptr()};
  int variableByNameIdLastBuilt = -1;
  IntervalList intervals;
  const VarNameMap &varNameMap;
  const HashStrings &intervalNameMap;

public:
  VarTable(const VarNameMap &var_name_map, const HashStrings &interval_name_map) :
    varNameMap{var_name_map}, intervalNameMap{interval_name_map}
  {}

  void emplaceVarsAndIntervals(Tab<Var> &&variables, IntervalList &&intervals)
  {
    variableList = eastl::move(variables);
    intervals = eastl::move(intervals);
  }

  // return variable by it's index
  int getVarCount() const;
  Var &getVar(int internal_index);
  const Var &getVar(int internal_index) const { return const_cast<VarTable *>(this)->getVar(internal_index); }

  const char *getVarName(const Var &var) const { return varNameMap.getName(var.nameId); }
  const char *getVarName(int internal_index) const { return getVarName(getVar(internal_index)); }

  // find id by id
  int getVarInternalIndex(const int variable_id);

  // delete all variables
  void clear();

  // dump all variables && it's values to console
  void dumpToConsole() const;

  // set variable value from string
  bool setVarValue(int internal_index, const char *str);

  // get interval list
  const IntervalList &getIntervalList() const;
  const Tab<Var> &getVariableList() const;
  IntervalList &getMutableIntervalList();
  Tab<Var> &getMutableVariableList();

  void link(const Tab<Var> &variables, const IntervalList &interval_list, Tab<int> &global_var_link_table,
    Tab<ShaderVariant::ExtType> &interval_link_table);
};

}; // namespace ShaderGlobal
