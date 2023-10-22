#pragma once

#include <util/dag_simpleString.h>
#include <dag/dag_vector.h>
#include <EASTL/array.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderMatData.h>
#include <3d/dag_materialData.h>
#include <ioSys/dag_dataBlock.h>

enum MatVarType
{
  MAT_VAR_TYPE_NONE = -1,
  MAT_VAR_TYPE_INT = SHVT_INT,
  MAT_VAR_TYPE_REAL = SHVT_REAL,
  MAT_VAR_TYPE_COLOR4 = SHVT_COLOR4,
  MAT_VAR_TYPE_BOOL,
  MAT_VAR_TYPE_ENUM_LIGHTING
};

enum LightingType
{
  LIGHTING_NONE,
  LIGHTING_LIGHTMAP,
  LIGHTING_VLTMAP,
  LIGHTING_TYPE_COUNT
};

enum
{
  MAT_VAR_REAL_TWO_SIDED,
  MAT_VAR_LIGHTING,
  MAT_VAR_TWOSIDED,
  MAT_SPECIAL_VAR_COUNT
};

extern const char *lightingTypeStrs[LIGHTING_TYPE_COUNT];

// A list of vars is constructed of all vars of a shader (+ special vars) to simplify work with them.
// So the ones used in a material are marked.
struct MatVarDesc
{
  SimpleString name;
  MatVarType type;
  ShaderMatData::VarValue value;
  bool usedInMaterial;

  bool operator==(const MatVarDesc &var) const;
};

struct EntityMatProperties
{
  int dagMatId = -1;
  SimpleString matName;
  SimpleString shClassName;
  dag::Vector<MatVarDesc> vars;
  eastl::array<SimpleString, MAXMATTEXNUM> textures;
};

void override_mat_vars_with_script(dag::Vector<MatVarDesc> &vars, const char *mat_script);
dag::Vector<MatVarDesc> make_mat_vars_from_script(const char *mat_script, const char *shclass);
SimpleString make_mat_script_from_vars(const dag::Vector<MatVarDesc> &vars);
dag::Vector<MatVarDesc> get_shclass_vars_default(const char *shclass);

void replace_mat_shclass_in_props_blk(DataBlock &props, const SimpleString &shclass);
void replace_mat_vars_in_props_blk(DataBlock &props, const dag::Vector<MatVarDesc> &vars);
void replace_mat_textures_in_props_blk(DataBlock &props, const eastl::array<SimpleString, MAXMATTEXNUM> &textures);
