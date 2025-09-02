// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globVar.h"
#include "shTargetContext.h"
#include "intervals.h"
#include "varMap.h"
#include <shaders/dag_shaderCommon.h>

#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include "shLog.h"

namespace ShaderGlobal
{

const char *get_var_name(const Var &variable, const shc::TargetContext &ctx) { return ctx.varNameMap().getName(variable.nameId); }

Tab<Var> &VarTable::getMutableVariableList() { return variableList; }
IntervalList &VarTable::getMutableIntervalList() { return intervals; }

const Tab<Var> &VarTable::getVariableList() const { return variableList; }

// get interval list
const IntervalList &VarTable::getIntervalList() const { return intervals; }

// return variable by ID
int VarTable::getVarCount() const { return variableList.size(); }

Var &VarTable::getVar(int internal_index) { return variableList[internal_index]; }

// delete all variables
void VarTable::clear()
{
  clear_and_shrink(variableList);
  intervals.clear();
  clear_and_shrink(variableByNameId);
  variableByNameIdLastBuilt = 0;
}

// find id by id
int VarTable::getVarInternalIndex(const int variable_id)
{
  if (variable_id == VarNameMap::BAD_ID)
    return -1;
  if (variable_id < variableByNameId.size())
    return variableByNameId[variable_id];

  if (variableList.size() == variableByNameIdLastBuilt)
    return -1;

  for (int i = variableByNameIdLastBuilt; i < variableList.size(); ++i)
  {
    int nameId = variableList[i].nameId;
    if (variableByNameId.size() <= nameId)
    {
      int addCnt = nameId - variableByNameId.size() + 1;
      int at = append_items(variableByNameId, addCnt);
      memset(&variableByNameId[at], 0xFF, sizeof(variableByNameId[at]) * addCnt);
    }

    variableByNameId[nameId] = i;
  }

  variableByNameIdLastBuilt = variableList.size();

  return getVarInternalIndex(variable_id);
}

// set variable value from string
bool VarTable::setVarValue(int internal_index, const char *str)
{
  if (!tabutils::isCorrectIndex(variableList, internal_index) || !str)
    return false;

  Var &variable = variableList[internal_index];

  switch (variable.type)
  {
    case SHVT_INT:
      if (sscanf(str, "%d", &variable.value.i) != 1)
        return false;
      break;
    case SHVT_INT4:
      if (sscanf(str, "%d %d %d %d", &variable.value.i4.x, &variable.value.i4.y, &variable.value.i4.z, &variable.value.i4.w) < 3)
        return false;
      break;
    case SHVT_REAL:
      if (sscanf(str, "%f", &variable.value.r) != 1)
        return false;
      break;
    case SHVT_COLOR4:
      if (sscanf(str, "%f %f %f %f", &variable.value.c4.r, &variable.value.c4.g, &variable.value.c4.b, &variable.value.c4.a) < 3)
        return false;
      break;
    case SHVT_TEXTURE: return false; break;
    default: return false;
  }

  return true;
}

bool Var::operator==(const Var &right) const
{
  G_ASSERT(type == right.type);
  if (shouldIgnoreValue || right.shouldIgnoreValue)
    return true;
  switch (type)
  {
    case SHVT_INT: return value.i == right.value.i;
    case SHVT_INT4: return memcmp(&value.i4, &right.value.i4, sizeof(value.i4)) == 0;
    case SHVT_REAL: return value.r == right.value.r;
    case SHVT_COLOR4: return memcmp(&value.c4, &right.value.c4, sizeof(value.c4)) == 0;
    case SHVT_FLOAT4X4: return true;
    case SHVT_BUFFER: return value.bufId == right.value.bufId;
    case SHVT_TLAS: return value.tlas == right.value.tlas;
    case SHVT_TEXTURE: return value.texId == right.value.texId;
    case SHVT_SAMPLER: return memcmp(&value.samplerInfo, &right.value.samplerInfo, sizeof(value.samplerInfo)) == 0;
    default: G_ASSERT(0);
  }
  return false;
}

void VarTable::link(const Tab<ShaderGlobal::Var> &variables, const IntervalList &intervalsFromFile, Tab<int> &global_var_link_table,
  Tab<ShaderVariant::ExtType> &interval_link_table)
{
  // Global vars.
  int varsNum = variables.size();
  global_var_link_table.resize(varsNum);
  for (unsigned int varNo = 0; varNo < varsNum; varNo++)
  {
    const Var &var = variables[varNo];

    bool exists = false;
    for (int existingVarId = 0; existingVarId < variableList.size(); existingVarId++)
    {
      Var &existingVar = variableList[existingVarId];
      if (var.nameId == existingVar.nameId)
      {
        exists = true;
        global_var_link_table[varNo] = existingVarId;

        if (var.type != existingVar.type)
          sh_debug(SHLOG_FATAL, "Different variable types: '%s'", varNameMap.getName(var.nameId));

        if (var != existingVar)
          sh_debug(SHLOG_FATAL, "Different variable values: '%s'", varNameMap.getName(var.nameId));

        if (var.isAlwaysReferenced != existingVar.isAlwaysReferenced)
          sh_debug(SHLOG_FATAL, "Different variable always_referenced states: '%s'", varNameMap.getName(var.nameId));

        if (var.isImplicitlyReferenced && !existingVar.isImplicitlyReferenced)
          existingVar.isImplicitlyReferenced = true;
      }
    }

    if (!exists)
    {
      global_var_link_table[varNo] = variableList.size();
      variableList.push_back(var);
    }
  }

  // Intervals.
  interval_link_table.resize(intervalsFromFile.getCount());
  for (unsigned int intervalNo = 0; intervalNo < intervalsFromFile.getCount(); intervalNo++)
  {
    if (intervalsFromFile.getInterval(intervalNo))
    {
      bool res = intervals.addInterval(*intervalsFromFile.getInterval(intervalNo));
      if (!res)
        sh_debug(SHLOG_FATAL, "Different intervals: '%s'",
          intervalNameMap.getName(intervalsFromFile.getInterval(intervalNo)->getNameId()));

      interval_link_table[intervalNo] = intervals.getIntervalIndex(intervalsFromFile.getInterval(intervalNo)->getNameId());
    }
    else
    {
      interval_link_table[intervalNo] = INTERVAL_NOT_INIT;
    }
  }
}

} // namespace ShaderGlobal