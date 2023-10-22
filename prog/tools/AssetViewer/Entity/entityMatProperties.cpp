#include "entityMatProperties.h"

#include <obsolete/dag_cfg.h>
#include <shaders/dag_shMaterialUtils.h>
#include <de3_interface.h>
#include <assets/asset.h>
#include <libTools/dagFileRW/dagFileFormat.h>

const char *lightingTypeStrs[LIGHTING_TYPE_COUNT] = {"none", "lightmap", "vltmap"};

static String make_mat_var_str(const MatVarDesc &var)
{
  switch (var.type)
  {
    case MAT_VAR_TYPE_BOOL: return String(0, "%s=%s", var.name, var.value.i ? "yes" : "no");
    case MAT_VAR_TYPE_ENUM_LIGHTING: return String(0, "%s=%s", var.name, lightingTypeStrs[var.value.i]);
    case MAT_VAR_TYPE_INT: return String(0, "%s=%d", var.name, var.value.i);
    case MAT_VAR_TYPE_REAL: return String(0, "%s=%f", var.name, var.value.r);
    case MAT_VAR_TYPE_COLOR4:
      return String(0, "%s=%f,%f,%f,%f", var.name, var.value.c[0], var.value.c[1], var.value.c[2], var.value.c[3]);
  }
  return String{};
}

bool MatVarDesc::operator==(const MatVarDesc &var) const
{
  // Have to compare strings because if to convert a var into a string and then back,
  // it's value can be altered because of rounding errors (it's an issue when saving a material to a file
  // and then checking for difference with this saved data).
  // Since material data is stored as strings, it's correct to compare vars this way.
  return usedInMaterial == var.usedInMaterial && make_mat_var_str(*this) == make_mat_var_str(var);
}

void override_mat_vars_with_script(dag::Vector<MatVarDesc> &vars, const char *mat_script)
{
  CfgReader matScriptReader;
  matScriptReader.getdiv_text(String(0, "[q]\r\n%s\r\n", mat_script), "q");

  for (MatVarDesc &var : vars)
  {
    if (!matScriptReader.getstr(var.name.c_str(), nullptr)) // If the var is not in the script.
      continue;

    switch (var.type)
    {
      case MAT_VAR_TYPE_BOOL: var.value.i = matScriptReader.getbool(var.name.c_str(), false); break;
      case MAT_VAR_TYPE_ENUM_LIGHTING:
      {
        const char *valStr = matScriptReader.getstr(var.name.c_str(), lightingTypeStrs[LIGHTING_NONE]);
        int val = LIGHTING_NONE;
        if (strcmp(valStr, lightingTypeStrs[LIGHTING_LIGHTMAP]) == 0)
          val = LIGHTING_LIGHTMAP;
        else if (strcmp(valStr, lightingTypeStrs[LIGHTING_VLTMAP]) == 0)
          val = LIGHTING_VLTMAP;
        var.value.i = val;
      }
      break;
      case MAT_VAR_TYPE_INT: var.value.i = matScriptReader.getint(var.name.c_str(), -1); break;
      case MAT_VAR_TYPE_REAL: var.value.r = matScriptReader.getreal(var.name.c_str(), 0.0f); break;
      case MAT_VAR_TYPE_COLOR4: var.value.c4() = matScriptReader.getcolor4(var.name.c_str(), E3DCOLOR(0)); break;
    }
    var.usedInMaterial = true;
  }
}

dag::Vector<MatVarDesc> make_mat_vars_from_script(const char *mat_script, const char *shclass)
{
  dag::Vector<MatVarDesc> vars = get_shclass_vars_default(shclass);
  override_mat_vars_with_script(vars, mat_script);
  return vars;
}

SimpleString make_mat_script_from_vars(const dag::Vector<MatVarDesc> &vars)
{
  String script;
  for (auto &var : vars)
  {
    if (var.usedInMaterial)
      script += make_mat_var_str(var) + "\r\n";
  }
  return SimpleString(script.c_str());
}

dag::Vector<MatVarDesc> get_shclass_vars_default(const char *shclass)
{
  dag::Vector<ShaderVarDesc> shClassVarList = get_shclass_script_vars_list(shclass);

  dag::Vector<MatVarDesc> vars;
  vars.resize(MAT_SPECIAL_VAR_COUNT + shClassVarList.size());

  vars[MAT_VAR_REAL_TWO_SIDED].name = "real_two_sided";
  vars[MAT_VAR_REAL_TWO_SIDED].type = MAT_VAR_TYPE_BOOL;
  vars[MAT_VAR_REAL_TWO_SIDED].value.i = 0;
  vars[MAT_VAR_REAL_TWO_SIDED].usedInMaterial = false;

  vars[MAT_VAR_LIGHTING].name = "lighting";
  vars[MAT_VAR_LIGHTING].type = MAT_VAR_TYPE_ENUM_LIGHTING;
  vars[MAT_VAR_LIGHTING].value.i = LIGHTING_NONE;
  vars[MAT_VAR_LIGHTING].usedInMaterial = false;

  vars[MAT_VAR_TWOSIDED].name = "twosided";
  vars[MAT_VAR_TWOSIDED].type = MAT_VAR_TYPE_BOOL;
  vars[MAT_VAR_TWOSIDED].value.i = 0;
  vars[MAT_VAR_TWOSIDED].usedInMaterial = false;

  for (int varId = MAT_SPECIAL_VAR_COUNT; varId < vars.size(); ++varId)
  {
    int shVarId = varId - MAT_SPECIAL_VAR_COUNT;
    vars[varId].name = shClassVarList[shVarId].name;
    vars[varId].type = MatVarType(shClassVarList[shVarId].type);
    vars[varId].value = shClassVarList[shVarId].value;
    vars[varId].usedInMaterial = false;
  }

  return vars;
}

void replace_mat_shclass_in_props_blk(DataBlock &props, const SimpleString &shclass) { props.setStr("class", shclass.c_str()); }

void replace_mat_vars_in_props_blk(DataBlock &props, const dag::Vector<MatVarDesc> &vars)
{
  while (props.removeParam("script"))
    ;
  for (auto &var : vars)
  {
    if (var.usedInMaterial)
      props.addStr("script", make_mat_var_str(var));
  }
}

void replace_mat_textures_in_props_blk(DataBlock &props, const eastl::array<SimpleString, MAXMATTEXNUM> &textures)
{
  for (int texSlot = 0; texSlot < DAGTEXNUM; ++texSlot)
  {
    String texParamName = String(0, "tex%d", texSlot);
    if (DagorAsset *texAsset = DAEDITOR3.getAssetByName(textures[texSlot]))
      props.setStr(texParamName, texAsset->getTargetFilePath());
    else
      props.removeParam(texParamName);
  }
}
