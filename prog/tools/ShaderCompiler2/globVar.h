#pragma once
#include <math/dag_color.h>
#include <math/integer/dag_IPoint4.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>
#include "varTypes.h"
#include "varMap.h"
#include "shaderSave.h"

class IntervalList;

namespace ShaderGlobal
{
union StVarValue
{
  struct
  {
    Color4 c4;
  };
  struct
  {
    IPoint4 i4;
  };
  real r;
  int i;
  struct
  {
    TEXTUREID texId;
    BaseTexture *tex;
  };
  struct
  {
    D3DRESID bufId;
    Sbuffer *buf;
  };
  StVarValue() {}
};

class Var
{
public:
  StVarValue value;
  NameId<VarMapAdapter> nameId;
  uint8_t type;
  bool isAlwaysReferenced, shouldIgnoreValue;
  bindump::string fileName;
  int array_size = 0;
  int index = 0;

  Var() : isAlwaysReferenced(false), shouldIgnoreValue(false) { memset(&value, 0, sizeof(value)); }

  inline const char *getName() const { return VarMap::getName(nameId); }

  bool operator==(const Var &right) const;
  bool operator!=(const Var &right) const { return !(operator==(right)); }
};

// return variable by it's index
int get_var_count();
Var &get_var(int internal_index);

// find id by id
int get_var_internal_index(const int variable_id);

// delete all variables
void clear();

// dump all variables && it's values to console
void dump_to_console();

// output variable to console
static void dump_variable(const Var &variable);

// set variable value from string
bool set_var_value(int internal_index, const char *str);

// get interval list
const IntervalList &getIntervalList();
IntervalList &getMutableIntervalList();
Tab<Var> &getMutableVariableList();

void link(const Tab<ShaderGlobal::Var> &variables, const IntervalList &interval_list, Tab<int> &global_var_link_table,
  Tab<ShaderVariant::ExtType> &interval_link_table);
}; // namespace ShaderGlobal
