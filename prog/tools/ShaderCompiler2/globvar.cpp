#include "globVar.h"
#include "intervals.h"
#include "varMap.h"
#include <shaders/dag_shaderCommon.h>

#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include "shLog.h"
namespace ShaderGlobal
{
static Tab<int> variable_by_nameid(midmem_ptr());
static Tab<Var> variable_list(midmem_ptr());
static int variable_by_nameid_last_built = -1;
static IntervalList intervals;

Tab<Var> &getMutableVariableList() { return variable_list; }
IntervalList &getMutableIntervalList() { return intervals; }

// get interval list
const IntervalList &getIntervalList() { return intervals; }

// return variable by ID
int get_var_count() { return variable_list.size(); }

Var &get_var(int internal_index) { return variable_list[internal_index]; }

// delete all variables
void clear()
{
  clear_and_shrink(variable_list);
  intervals.clear();
  clear_and_shrink(variable_by_nameid);
  variable_by_nameid_last_built = 0;
}

// find id by id
int get_var_internal_index(const int variable_id)
{
  if (variable_id == VarMap::BAD_ID)
    return -1;
  if (variable_id < variable_by_nameid.size())
    return variable_by_nameid[variable_id];

  if (variable_list.size() == variable_by_nameid_last_built)
    return -1;

  for (int i = variable_by_nameid_last_built; i < variable_list.size(); ++i)
  {
    int nameId = variable_list[i].nameId;
    if (variable_by_nameid.size() <= nameId)
    {
      int addCnt = nameId - variable_by_nameid.size() + 1;
      int at = append_items(variable_by_nameid, addCnt);
      memset(&variable_by_nameid[at], 0xFF, sizeof(variable_by_nameid[at]) * addCnt);
    }

    variable_by_nameid[nameId] = i;
  }

  variable_by_nameid_last_built = variable_list.size();

  return get_var_internal_index(variable_id);
}

// set variable value from string
bool set_var_value(int internal_index, const char *str)
{
  if (!tabutils::isCorrectIndex(variable_list, internal_index) || !str)
    return false;

  Var &variable = variable_list[internal_index];

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
    case SHVT_INT4: return value.i4 == right.value.i4;
    case SHVT_REAL: return value.r == right.value.r;
    case SHVT_COLOR4: return value.c4 == right.value.c4;
    case SHVT_FLOAT4X4: return true;
    case SHVT_BUFFER: return value.bufId == right.value.bufId;
    case SHVT_TEXTURE: return value.texId == right.value.texId;
    default: G_ASSERT(0);
  }
  return false;
}

void link(const Tab<ShaderGlobal::Var> &variables, const IntervalList &intervalsFromFile, Tab<int> &global_var_link_table,
  Tab<ShaderVariant::ExtType> &interval_link_table)
{
  // Global vars.
  int varsNum = variables.size();
  global_var_link_table.resize(varsNum);
  for (unsigned int varNo = 0; varNo < varsNum; varNo++)
  {
    const Var &var = variables[varNo];

    bool exists = false;
    for (unsigned int existingVar = 0; existingVar < variable_list.size(); existingVar++)
    {
      if (var.nameId == variable_list[existingVar].nameId)
      {
        exists = true;
        global_var_link_table[varNo] = existingVar;

        if (var.type != variable_list[existingVar].type)
          fatal("Different variable types: '%s'", var.getName());

        if (var != variable_list[existingVar])
          fatal("Different variable values: '%s'", var.getName());
      }
    }

    if (!exists)
    {
      global_var_link_table[varNo] = variable_list.size();
      variable_list.push_back(var);
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
        fatal("Different intervals: '%s'", intervalsFromFile.getInterval(intervalNo)->getNameStr());

      interval_link_table[intervalNo] = intervals.getIntervalIndex(intervalsFromFile.getInterval(intervalNo)->getNameId());
    }
    else
    {
      interval_link_table[intervalNo] = INTERVAL_NOT_INIT;
    }
  }
}
} // namespace ShaderGlobal
