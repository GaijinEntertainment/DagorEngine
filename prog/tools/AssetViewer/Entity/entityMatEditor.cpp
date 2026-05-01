// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "entityMatEditor.h"

#include <assets/assetExporter.h>
#include <assetsGui/av_assetTreeDragHandler.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <shaders/dag_shMaterialUtils.h>
#include <EASTL/algorithm.h>
#include <propPanel/control/container.h>
#include <shaders/dag_shaderCommon.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <de3_entityGetSceneLodsRes.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <memory/dag_framemem.h>
#include <de3_interface.h>
#include <de3_lodController.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/commonWindow/listDialog.h>
#include <propPanel/commonWindow/multiListDialog.h>
#include <propPanel/imguiHelper.h>
#include <libTools/util/strUtil.h>
#include <math/dag_mesh.h>
#include <util/dag_strUtil.h>
#include <util/dag_delayedAction.h>
#include <libTools/shaderResBuilder/processMat.h>
#include "../../../engine/shaders/scriptSElem.h"
#include "../../../engine/shaders/scriptSMat.h"
#include "../av_appwnd.h"
#include "entityMatEditorUndo.h"
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>

using hdpi::_pxScaled;
using PropPanel::DragAndDropResult;

namespace gamereshooks
{
int aux_game_res_handle_to_id(const char *resname, unsigned cid);
}

PropPanel::ContainerPropertyControl *get_material_editor_group();

enum
{
  // each shader variable consists of maximum three controls:
  // - ExtGroupPropertyControl (not each variable has a separate group but theoretically they could have)
  // - ExtensiblePropertyControl
  // - the variable itself
  CONTROLS_PER_MATERIAL_VAR = 3,

  MAX_MATERIAL_VAR_COUNT = 128,
  MAX_MATERIAL_TEXTURE_COUNT = 64,
  MAX_MATERIAL_EXTRA_CONTROLS_COUNT = 64,
  MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT =
    (CONTROLS_PER_MATERIAL_VAR * MAX_MATERIAL_VAR_COUNT) + MAX_MATERIAL_TEXTURE_COUNT + MAX_MATERIAL_EXTRA_CONTROLS_COUNT,

  MAX_LOD_MATERIAL_COUNT = 64,
  MAX_LOD_EXTRA_CONTROLS_COUNT = 64,
  MAX_LOD_GROUP_GUI_ELEMENTS_COUNT = MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT * MAX_LOD_MATERIAL_COUNT + MAX_LOD_EXTRA_CONTROLS_COUNT,
};

enum
{
  PID_MATERIALS_SHOW_TRIANGLE_COUNT,
  PID_LODS_MATERIAL_EDITORS_BASE
};

enum
{
  // Lod material editor local property IDs.
  LPID_LOD_MATERIALS_GROUP,
  LPID_LOD_MATERIAL_GROUPS_BASE
};

enum
{
  // Material group local property IDs.
  LPID_MATERIAL_GROUP = 1,
  LPID_TEXTURE_GROUP,
  LPID_SHADER_BUTTON,
  LPID_PROXYMAT_BUTTON,
  LPID_SAVE_PARAMS_BUTTON,
  LPID_COPY_PARAMS_BUTTON,
  LPID_PASTE_PARAMS_BUTTON,
  LPID_TRANSFER_CHANGES_TO_LODS,
  LPID_RESET_PARAMS_TO_DAG_BUTTON,
  LPID_ADD_SCRIPT_VARIABLES_BUTTON,
  LPID_REMOVE_SCRIPT_VARIABLES_BUTTON,
  LPID_SHADER_VARS_PARAMETER_GROUP_BASE,                                                             // for ExtGroupPropertyControl
  LPID_SHADER_VARS_EXTENSIBLE_BASE = LPID_SHADER_VARS_PARAMETER_GROUP_BASE + MAX_MATERIAL_VAR_COUNT, // for ExtensiblePropertyControl
  LPID_SHADER_VARS_BASE = LPID_SHADER_VARS_EXTENSIBLE_BASE + MAX_MATERIAL_VAR_COUNT,
  LPID_TEXTURE_LIST_BASE = LPID_SHADER_VARS_BASE + MAX_MATERIAL_VAR_COUNT,
};

G_STATIC_ASSERT(
  LPID_TEXTURE_LIST_BASE == (LPID_SHADER_VARS_PARAMETER_GROUP_BASE + (CONTROLS_PER_MATERIAL_VAR * MAX_MATERIAL_VAR_COUNT)));

#define PERFORM_FOR_ENTITY_LOD_SCENE(entity, action, lod, check)                                 \
  if (auto *getLodsRes = (entity)->queryInterface<IEntityGetDynSceneLodsRes>())                  \
  {                                                                                              \
    auto *sceneLodsRes = getLodsRes->getSceneLodsRes();                                          \
    if (sceneLodsRes && sceneLodsRes->lods.size() > lod)                                         \
      sceneLodsRes->lods[lod].scene->action;                                                     \
  }                                                                                              \
  else if (auto *getLodsRes = (entity)->queryInterface<IEntityGetRISceneLodsRes>())              \
  {                                                                                              \
    auto *sceneLodsRes = getLodsRes->getSceneLodsRes();                                          \
    if (sceneLodsRes && sceneLodsRes->lods.size() > lod && check(sceneLodsRes->lods[lod].scene)) \
      sceneLodsRes->lods[lod].scene->action;                                                     \
  }

G_STATIC_ASSERT(DAGTEXNUM == MAXMATTEXNUM);

static const char *materialSearchVarsToSkip[] = {"num_bones"};
static const char *missingResName = "--";
EntityMatProperties EntityMaterialEditor::propertiesCopyBuffer;
String EntityMaterialEditor::propertiesCopyBufferProxyMatName;

static String make_shader_button_name(const char *shclass_name) { return String(0, "Shader: %s", shclass_name); }

static String make_proxymat_button_name(const char *proxymat_name)
{
  return String(0, "ProxyMat: %s", *proxymat_name ? proxymat_name : missingResName);
}

static bool is_var_within_limits(const MatVarDesc &var, const ShaderVariableLimits *limits)
{
  if (!limits || !limits->hasMinMaxValue())
    return true;

  if (var.type == MAT_VAR_TYPE_INT)
  {
    const int minValue = (int)floor(limits->minValue);
    const int maxValue = (int)ceil(limits->maxValue);
    return var.value.i >= minValue && var.value.i <= limits->maxValue;
  }
  else if (var.type == MAT_VAR_TYPE_REAL)
  {
    return var.value.r >= limits->minValue && var.value.r <= limits->maxValue;
  }
  else if (var.type == MAT_VAR_TYPE_COLOR4)
  {
    return var.value.c[0] >= limits->minValue && var.value.c[0] <= limits->maxValue && var.value.c[1] >= limits->minValue &&
           var.value.c[1] <= limits->maxValue && var.value.c[2] >= limits->minValue && var.value.c[2] <= limits->maxValue &&
           var.value.c[3] >= limits->minValue && var.value.c[3] <= limits->maxValue;
  }

  return true;
}

static void set_up_shader_var_property_internal(PropPanel::ContainerPropertyControl &panel, int property_id, const char *variable_name,
  const char *description, const char *limits, bool within_limits, const char *default_value, bool value_is_default)
{
  static String tooltip;

  const bool hasLimits = limits && *limits;
  if (within_limits || !hasLimits)
  {
    tooltip = variable_name;

    if (description && *description)
    {
      tooltip += "\n\n";
      tooltip += description;
    }

    if (hasLimits)
    {
      tooltip += "\n\nExpected range: ";
      tooltip += limits;
    }

    if (default_value && *default_value)
    {
      tooltip += hasLimits ? "\n" : "\n\n";
      tooltip += "Default value: ";
      tooltip += default_value;
    }

    // According to the design the default values are highlighted.
    panel.setValueHighlightById(property_id,
      value_is_default ? PropPanel::ColorOverride::EDIT_BOX_NON_DEFAULT_VALUE_BACKGROUND : PropPanel::ColorOverride::NONE);
  }
  else
  {
    tooltip = "WARNING!\nExpected range: ";
    tooltip += limits;

    if (default_value && *default_value)
    {
      tooltip += "\nDefault value: ";
      tooltip += default_value;
    }

    panel.setValueHighlightById(property_id, PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND);
  }

  panel.setTooltipId(property_id, tooltip);
}

static void set_up_shader_var_property(PropPanel::ContainerPropertyControl &panel, int property_id, const MatVarDesc &var,
  const ShaderVariableLimits *limits)
{
  const bool withinLimits = is_var_within_limits(var, limits);

  String limitsText;
  if (var.type == MAT_VAR_TYPE_INT && limits && limits->hasMinMaxValue())
  {
    const int minValue = (int)floor(limits->minValue);
    const int maxValue = (int)ceil(limits->maxValue);
    limitsText.printf(0, "[%d, %d]", minValue, maxValue);
  }
  else if (var.type == MAT_VAR_TYPE_REAL && limits && limits->hasMinMaxValue())
  {
    limitsText.printf(0, "[%g, %g]", limits->minValue, limits->maxValue);
  }
  else if (var.type == MAT_VAR_TYPE_COLOR4 && limits && limits->hasMinMaxValue())
  {
    limitsText.printf(0, "[%g, %g] for all components", limits->minValue, limits->maxValue);
  }

  String defaultText;
  bool valueIsDefault = true;
  if (limits && limits->defaultValueType == var.type)
  {
    const ShaderMatData::VarValue &def = limits->defaultValue;

    switch (limits->defaultValueType)
    {
      case MAT_VAR_TYPE_BOOL:
        defaultText.printf(0, "%d", def.i == 0 ? 0 : 1);
        valueIsDefault = var.value.i == def.i;
        break;

      case MAT_VAR_TYPE_ENUM_LIGHTING:
        G_STATIC_ASSERT(LIGHTING_NONE == 0);
        G_STATIC_ASSERT(LIGHTING_TYPE_COUNT == 3);
        defaultText =
          (def.i >= LIGHTING_NONE && def.i < LIGHTING_TYPE_COUNT) ? lightingTypeStrs[def.i] : lightingTypeStrs[LIGHTING_NONE];
        valueIsDefault = var.value.i == def.i;
        break;

      case MAT_VAR_TYPE_INT:
        defaultText.printf(0, "%d", def.i);
        valueIsDefault = var.value.i == def.i;
        break;

      case MAT_VAR_TYPE_REAL:
        defaultText.printf(0, "%g", def.r);
        valueIsDefault = var.value.r == def.r;
        break;

      case MAT_VAR_TYPE_COLOR4:
        defaultText.printf(0, "%g, %g, %g, %g", def.c[0], def.c[1], def.c[2], def.c[3]);
        valueIsDefault = var.value.c4() == def.c4();
        break;
    }
  }

  set_up_shader_var_property_internal(panel, property_id, var.name.c_str(), limits ? limits->description.c_str() : nullptr,
    limitsText.c_str(), withinLimits, defaultText.c_str(), valueIsDefault);
}

static bool should_skip_var(const char *var_name)
{
  const char **skipVarsBegin = materialSearchVarsToSkip;
  const char **skipVarsEnd = materialSearchVarsToSkip + sizeof(materialSearchVarsToSkip) / sizeof(*materialSearchVarsToSkip);
  return eastl::find_if(skipVarsBegin, skipVarsEnd, [var_name](auto &skip_name) { return strcmp(skip_name, var_name) == 0; }) !=
         skipVarsEnd;
}

static inline int get_mat_group_panel_base_pid(int lod, int mat_id)
{
  return MAX_LOD_GROUP_GUI_ELEMENTS_COUNT * lod + PID_LODS_MATERIAL_EDITORS_BASE + MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT * mat_id +
         LPID_LOD_MATERIAL_GROUPS_BASE;
}

#define CHECK(res) (res->getMesh() && res->getMesh()->getMesh())

static ShaderMaterial *find_entity_shader_material(const IObjEntity *entity, const MaterialData &mat_data, int lod)
{
  const shaderbindump::ShaderClass *shaderClass = shBinDump().findShaderClass(mat_data.className);
  if (!shaderClass)
    return nullptr;

  ShaderMaterialProperties *shaderMatProps = ShaderMaterialProperties::create(shaderClass, mat_data);

  Tab<ShaderMaterial *> usedMats;
  PERFORM_FOR_ENTITY_LOD_SCENE(const_cast<IObjEntity *>(entity), gatherUsedMat(usedMats), lod, CHECK);

  ShaderMaterial *result = nullptr;
  for (ShaderMaterial *mat : usedMats)
  {
    for (int varId = 0; varId < shaderClass->localVars.size(); ++varId)
    {
      if (should_skip_var((const char *)shBinDump().varMap[shaderClass->localVars.v[varId].nameId]))
        shaderMatProps->stvar[varId].c4() = mat->native().props.stvar[varId].c4();
    }

    // We can't compare textures because atest texture is formed from diffuse's alpha channel.
    // So slot for it isn't set when you construct ShaderMaterialProperties.
    // It's set inside GenericTexSubstProcessMaterialData::processMaterial(),
    // on asset build. When trying to use it from here, texture names, acquired by TEXTUREID,
    // don't match the pattern and alpha texture isn't set therefore.
    bool matsCompatible = false;
    if ((matsCompatible = mat->native().props.isEqualWithoutTex(*shaderMatProps)))
    {
      for (int i = 0; i < MAXMATTEXNUM; ++i)
        matsCompatible &=
          shaderMatProps->textureId[i] == BAD_TEXTUREID || shaderMatProps->textureId[i] == mat->native().props.textureId[i];
    }
    if (matsCompatible)
    {
      result = mat;
      break;
    }
  }
  delete shaderMatProps;

  return result;
}

static int get_slot_with_alpha_tex(const TEXTUREID *texture_ids)
{
  for (int texSlotId = 0; texSlotId < MAXMATTEXNUM; ++texSlotId)
  {
    TEXTUREID texId = texture_ids[texSlotId];
    if (texId != BAD_TEXTUREID)
    {
      const char *texName = ::get_managed_texture_name(texId);
      if (strstr(texName, "_a*"))
        return texSlotId;
    }
  }
  return -1;
}

static DataBlock *find_asset_lod_props(DataBlock &asset_props, int lod)
{
  int lodNameId = asset_props.getNameId("lod"), lodCnt = 0;
  for (int i = 0; DataBlock *lodProps = asset_props.getBlock(i); ++i)
    if (lodProps->getBlockNameId() == lodNameId && lodCnt++ == lod)
      return lodProps;
  return nullptr;
}

static DataBlock *find_mat_override_props(DataBlock &material_overrides, const char *mat_name)
{
  for (int i = 0; DataBlock *matOverrideProps = material_overrides.getBlock(i); ++i)
  {
    const char *name = matOverrideProps->getStr("name");
    if (name && strcmp(name, mat_name) == 0)
      return matOverrideProps;
  }
  return nullptr;
}

static void notify_asset_changed(const DagorAsset *asset)
{
  get_app().getAssetMgr().callAssetBaseChangeNotifications(dag::ConstSpan<DagorAsset *>((DagorAsset *const *)&asset, 1),
    dag::ConstSpan<DagorAsset *>(), dag::ConstSpan<DagorAsset *>());
  get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());
}

static bool get_panel_property_address(int pcb_id, int &out_lod, int &out_mat_id, int &out_mat_local_prop_id)
{
  out_lod = -1;
  out_mat_id = -1;
  out_mat_local_prop_id = -1;
  if (pcb_id < PID_LODS_MATERIAL_EDITORS_BASE)
    return false;

  int lodLocalPropId = pcb_id - PID_LODS_MATERIAL_EDITORS_BASE;
  out_lod = lodLocalPropId / MAX_LOD_GROUP_GUI_ELEMENTS_COUNT;
  lodLocalPropId -= out_lod * MAX_LOD_GROUP_GUI_ELEMENTS_COUNT;
  if (lodLocalPropId < LPID_LOD_MATERIAL_GROUPS_BASE)
    return false;

  out_mat_local_prop_id = lodLocalPropId - LPID_LOD_MATERIAL_GROUPS_BASE;
  out_mat_id = out_mat_local_prop_id / MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT;
  out_mat_local_prop_id -= out_mat_id * MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT;

  return true;
}


static String get_asset_dag_file_path(const DagorAsset &asset, int lod)
{
  const int lodNameId = asset.props.getNameId("lod");
  int lodBlockIndex = 0;

  for (int i = 0; const DataBlock *blk = asset.props.getBlock(i); ++i)
  {
    if (blk->getBlockNameId() != lodNameId)
      continue;

    if (lodBlockIndex == lod)
    {
      const int paramIndex = blk->findParam("fname");
      const char *dagName = blk->getStr(paramIndex);
      if (dagName && *dagName)
        return String(0, "%s/%s", asset.getFolderPath(), dagName);

      break;
    }

    ++lodBlockIndex;
  }

  return String(0, "%s/%s.lod%02d.dag", asset.getFolderPath(), asset.getName(), lod);
}

namespace
{

struct ShaderParameterGroup
{
  SimpleString groupName;
  SimpleString groupDescription;
  dag::Vector<MatVarDesc> varsResult;
  dag::Vector<int> varIndices;
  dag::Vector<const ShaderVariableLimits *> varLimits;
};

struct ShaderParameterGroups
{
  void fillGroups(const ska::flat_hash_map<eastl::string, ShaderDescriptor> &shader_prop_separators,
    const ShaderSeparatorToPropsType &shader_separator_to_props, const dag::Vector<MatVarDesc> &vars)
  {
    // Global properties
    auto globalsIt = shader_prop_separators.find("_global_params");
    ShaderParameterGroup *group = nullptr;
    if (globalsIt != shader_prop_separators.end())
    {
      const ShaderSeparatorToPropsType &props = globalsIt->second.shaderSeparatorToProps;
      for (const auto &kv : props)
      {
        for (int varId = 0; varId < vars.size(); ++varId)
        {
          const MatVarDesc &var = vars[varId];
          if (!var.usedInMaterial)
            continue;
          auto propIt = kv.second.shaderVariables.find(var.name.c_str());
          if (propIt != kv.second.shaderVariables.end() && !isVarInProps(shader_separator_to_props, var.name))
          {
            if (!group)
            {
              group = &groups.push_back();
              group->groupName = "Global parameters";
            }

            group->varsResult.push_back(var);
            group->varIndices.push_back(varId);
            group->varLimits.push_back(&propIt->second);
          }
        }
      }
    }

    // Shader properties
    for (const auto &kv : shader_separator_to_props)
    {
      group = nullptr;
      for (int varId = 0; varId < vars.size(); ++varId)
      {
        const MatVarDesc &var = vars[varId];
        if (!var.usedInMaterial)
          continue;
        const ShaderDescriptorVariableGroup &descriptorGroup = kv.second;
        auto propIt = descriptorGroup.shaderVariables.find(var.name.c_str());
        if (propIt != descriptorGroup.shaderVariables.end())
        {
          if (!group)
          {
            group = &groups.push_back();
            group->groupName = kv.first.c_str();
            group->groupDescription = descriptorGroup.description.c_str();
          }

          group->varsResult.push_back(var);
          group->varIndices.push_back(varId);
          group->varLimits.push_back(&propIt->second);
        }
      }
    }

    // Gather shader vars not listed in dagorShaders.blk
    group = getGroupByName("");
    for (int varId = 0; varId < vars.size(); ++varId)
    {
      const MatVarDesc &var = vars[varId];
      if (!var.usedInMaterial)
        continue;

      if (globalsIt != shader_prop_separators.end() && isVarInProps(globalsIt->second.shaderSeparatorToProps, var.name))
        continue;

      if (isVarInProps(shader_separator_to_props, var.name))
        continue;

      if (!group)
        group = &groups.push_back();

      group->varsResult.push_back(var);
      group->varIndices.push_back(varId);
      group->varLimits.push_back(nullptr);
    }
  }

  ShaderParameterGroup *getGroupByName(const char *name)
  {
    for (ShaderParameterGroup &group : groups)
      if (strcmp(group.groupName.c_str(), name) == 0)
        return &group;
    return nullptr;
  }

  const ShaderParameterGroup *getGroupByVariableIndex(int variable_index)
  {
    for (const ShaderParameterGroup &group : groups)
      for (int index : group.varIndices)
        if (index == variable_index)
          return &group;
    return nullptr;
  }

  static bool isVarInProps(const ShaderSeparatorToPropsType &props, const SimpleString &var_name)
  {
    for (const auto &kv : props)
      if (kv.second.shaderVariables.find(var_name.c_str()) != kv.second.shaderVariables.end())
        return true;
    return false;
  }

  dag::Vector<ShaderParameterGroup> groups;
};

void set_up_shader_parameter_group_property(PropPanel::ContainerPropertyControl &panel, const ShaderParameterGroup &group)
{
  static String tooltip;

  int outOfLimitCount = 0;
  for (int i = 0; i < group.varIndices.size(); ++i)
  {
    const MatVarDesc &var = group.varsResult[i];
    const ShaderVariableLimits *variableLimits = group.varLimits.empty() ? nullptr : group.varLimits[i];
    if (!is_var_within_limits(var, variableLimits))
      ++outOfLimitCount;
  }

  if (outOfLimitCount > 1)
    tooltip.printf(0, "WARNING!\n%d variables are out of range.", outOfLimitCount);
  else if (outOfLimitCount == 1)
    tooltip = "WARNING!\nA variable is out of range.";
  else
  {
    tooltip = group.groupName.c_str();

    if (!group.groupDescription.empty())
    {
      tooltip += "\n\n";
      tooltip += group.groupDescription.c_str();
    }
  }

  panel.setValueHighlight(
    outOfLimitCount > 0 ? PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND : PropPanel::ColorOverride::NONE);
  panel.setTooltip(tooltip);
}

void update_shader_parameter_group_property(PropPanel::ContainerPropertyControl &panel, int pcb_id,
  const ska::flat_hash_map<eastl::string, ShaderDescriptor> &shader_prop_separators, const EntityMatProperties &mat_props,
  int variable_index)
{
  const auto shaderIt = shader_prop_separators.find(mat_props.shClassName.c_str());
  if (shaderIt == shader_prop_separators.end())
    return;

  const ShaderDescriptor &shaderDescriptor = shaderIt->second;
  ShaderParameterGroups shaderParameterGroups;
  shaderParameterGroups.fillGroups(shader_prop_separators, shaderDescriptor.shaderSeparatorToProps, mat_props.vars);

  PropPanel::PropertyControlBase *control = panel.getById(pcb_id);
  G_ASSERT(control);
  PropPanel::ContainerPropertyControl *controlParent = control->getParent(); // extensible
  G_ASSERT(controlParent);
  controlParent = controlParent->getParent(); // extensible group
  G_ASSERT(controlParent);

  const ShaderParameterGroup *group = shaderParameterGroups.getGroupByVariableIndex(variable_index);
  G_ASSERT(group);

  set_up_shader_parameter_group_property(*controlParent, *group);
}

class MaterialPasteMessageBoxDialog : public PropPanel::DialogWindow
{
public:
  MaterialPasteMessageBoxDialog(const char *message1, const char *message2, bool shaders_differ, bool shaders_differ_and_compatible) :
    PropPanel::DialogWindow(nullptr, hdpi::_pxActual(300), hdpi::_pxActual(100), "Paste options")
  {
    // The layout is like this because the buttons are long and more importantly, autoSize() currently does not handle
    // the buttons panel properly.

    String message(message1);
    if (message2 && *message2)
    {
      message += "\n\n";
      message += message2;
    }
    propertiesPanel->createStatic(PID_MESSAGE, message, true, true);

    propertiesPanel->createButton(PID_APPLY_ONLY_THE_PROPERTIES, "Paste only the properties");
    if (shaders_differ)
      propertiesPanel->createButton(PID_APPLY_THE_SHADER_AND_THE_PROPERTIES, "Paste the shader and the properties",
        shaders_differ_and_compatible);
    propertiesPanel->createButton(PID_CONVERT_TO_NON_PROXY_MAT, "Convert to non-ProxyMat and then paste");
    propertiesPanel->createButton(PID_SET_COPIED_PROXY_MAT, "Set copied ProxyMat");

    buttonsPanel->removeById(PropPanel::DIALOG_ID_OK);

    setInitialFocus(PID_APPLY_ONLY_THE_PROPERTIES);
    autoSize();
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_APPLY_ONLY_THE_PROPERTIES || pcb_id == PID_APPLY_THE_SHADER_AND_THE_PROPERTIES ||
        pcb_id == PID_CONVERT_TO_NON_PROXY_MAT || pcb_id == PID_SET_COPIED_PROXY_MAT)
      hide(pcb_id);
  }

  enum
  {
    PID_MESSAGE = PropPanel::DIALOG_ID_FIRST_FREE,
    PID_APPLY_ONLY_THE_PROPERTIES,
    PID_APPLY_THE_SHADER_AND_THE_PROPERTIES,
    PID_CONVERT_TO_NON_PROXY_MAT,
    PID_SET_COPIED_PROXY_MAT,
  };
};

} // namespace

const ShaderVariableLimits *EntityMaterialEditor::getShaderVariableLimits(const EntityMatProperties &mat_props,
  const MatVarDesc &var) const
{
  const char *matShaderName = mat_props.shClassName.c_str();
  const auto shaderIt = shaderPropSeparators.find(matShaderName);
  if (shaderIt == shaderPropSeparators.end())
    return nullptr;

  // Shader properties
  const ShaderSeparatorToPropsType &shaderSeparatorToProps = shaderIt->second.shaderSeparatorToProps;
  for (const auto &kv : shaderSeparatorToProps)
  {
    auto propIt = kv.second.shaderVariables.find(var.name.c_str());
    if (propIt != kv.second.shaderVariables.end())
      return &propIt->second;
  }

  // Global properties
  auto globalsIt = shaderPropSeparators.find("_global_params");
  if (globalsIt != shaderPropSeparators.end())
  {
    const ShaderSeparatorToPropsType &props = globalsIt->second.shaderSeparatorToProps;
    for (const auto &kv : props)
    {
      auto propIt = kv.second.shaderVariables.find(var.name.c_str());
      if (propIt != kv.second.shaderVariables.end())
        return &propIt->second;
    }
  }

  return nullptr;
}

int EntityMaterialEditor::getMaterialIndexByName(const DagMatFileResourcesHandler &file_res, const char *name)
{
  const int count = file_res.getMaterialCount();
  for (int i = 0; i < count; ++i)
    if (file_res.getMaterialName(i) == name)
      return i;

  return -1;
}

void EntityMaterialEditor::createSubMaterialToMaterialIndexMap(const char *dag_name, const Node &node,
  const DagMatFileResourcesHandler &file_res, dag::Vector<int> &map)
{
  // node.mat is never null due to the condition in countMaterialUsage.
  G_ASSERT(node.mat);
  if (!node.mat)
    return;

  const int subMatCount = node.mat->subMatCount();
  map.resize(subMatCount, -1);

  for (int i = 0; i < subMatCount; ++i)
  {
    const MaterialData *subMaterialData = node.mat->getSubMat(i);
    if (!subMaterialData)
      continue;

    const char *subMatName = subMaterialData->matName.c_str();
    const int materialIndex = getMaterialIndexByName(file_res, subMatName);
    if (materialIndex >= 0)
      map[i] = materialIndex;
    else
      logwarn("Sub-material '%s' of node '%s' cannot be found in the main material list in file '%s'.", subMatName, node.name.c_str(),
        dag_name);
  }
}

void EntityMaterialEditor::countMaterialUsage(const char *dag_name, const Node &node, const DagMatFileResourcesHandler &file_res,
  dag::Span<int> materials)
{
  for (const Node *child : node.child)
    if (child)
      countMaterialUsage(dag_name, *child, file_res, materials);

  if ((node.flags & NODEFLG_RENDERABLE) == 0 || !node.mat || node.mat->subMatCount() == 0 || !node.obj ||
      !node.obj->isSubOf(OCID_MESHHOLDER))
    return;

  const MeshHolderObj &meshHolder = static_cast<const MeshHolderObj &>(*node.obj);
  if (!meshHolder.mesh)
    return;

  dag::Vector<int> subMaterialToMaterialIndexMap;
  createSubMaterialToMaterialIndexMap(dag_name, node, file_res, subMaterialToMaterialIndexMap);
  G_ASSERT(!subMaterialToMaterialIndexMap.empty()); // This is always true due to the "node.mat->subMatCount() == 0" condition above.

  for (const Face &face : meshHolder.mesh->face)
  {
    // Bad sub-material indices are simply clamped. (See collapse_nodes in shaderResBuilder/collapseData.h.)
    int subMaterialIndex = face.mat;
    if (subMaterialIndex >= subMaterialToMaterialIndexMap.size())
      subMaterialIndex = subMaterialToMaterialIndexMap.size() - 1;

    const int materialIndex = subMaterialToMaterialIndexMap[subMaterialIndex];
    if (materialIndex >= 0)
      ++materials[materialIndex];
  }
}

void EntityMaterialEditor::loadSettings(const DataBlock &settings)
{
  showTriangleCount = settings.getBool("showTriangleCount", showTriangleCount);
}

void EntityMaterialEditor::saveSettings(DataBlock &settings) const { settings.addBool("showTriangleCount", showTriangleCount); }

void EntityMaterialEditor::begin(DagorAsset *asset, IObjEntity *asset_entity)
{
  entity = asset_entity;

  if (isReloading)
  {
    isReloading = false;
    onReloaded();
    return;
  }

  end();
  if (!asset || !entity)
    return;

  assetSrcFolderPath = asset->getFolderPath();
  assetName = asset->getName();

  if (appBlk.isEmpty())
  {
    appBlk.load(::get_app().getWorkspace().getAppBlkPath());
    shaderPropSeparators = gatherParameterSeparators(appBlk, ::get_app().getWorkspace().getAppDir());

    for (auto &shClass : shBinDump().classes)
    {
      const char *shaderName = (const char *)shClass.name;
      set_defaults_from_shader_bin_dump(shaderName, shaderPropSeparators[shaderName]);
    }
  }

  proxymatDropHandler.reset(new ProxymatAssetDragAndDropHandler(this));
  textureDropHandler.reset(new TextureAssetDragAndDropHandler(this));

  const DataBlock *curTypeBlk = appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx(
    DAEDITOR3.getAssetTypeName(asset_entity->getAssetTypeId()));
  shRemapBlk = curTypeBlk->getBlockByNameEx("remapShaders");

  matDataPerLod.clear();

  ILodController *lodController = entity ? entity->queryInterface<ILodController>() : nullptr;
  if (lodController && dd_stricmp(asset->getTypeStr(), "rendInst") != 0 && dd_stricmp(asset->getTypeStr(), "dynModel") != 0)
    lodController = nullptr;
  const int transition_lods = asset->props.getBlockByName("transition_lod") && asset->props.getBlockByName("impostor") ? 1 : 0;
  const int lodCount = lodController ? lodController->getLodCount() - transition_lods : 0;

  for (int lod = 0; lod < lodCount; ++lod)
  {
    const String dagFileName = get_asset_dag_file_path(*asset, lod);
    if (!dd_file_exists(dagFileName.c_str()))
    {
      logwarn("Expected %d LODs but '%s' cannot be found.", lodCount, dagFileName.c_str());
      break;
    }

    EntityLodMatData &lodMatData = matDataPerLod.push_back();
    lodMatData.fileRes = eastl::make_unique<DagMatFileResourcesHandler>(dagFileName, DAEDITOR3.getAssetByName(assetName.c_str()));

    dag::Vector<int> materialUsage(lodMatData.fileRes->getMaterialCount(), 0);
    if (showTriangleCount)
    {
      AScene scene;
      if (load_ascene(dagFileName, scene, LASF_MATNAMES | LASF_NOSPLINES | LASF_NOLIGHTS, false) && scene.root)
        countMaterialUsage(dagFileName, *scene.root, *lodMatData.fileRes, make_span(materialUsage));
    }

    for (int dagMatId = 0; dagMatId < lodMatData.fileRes->getMaterialCount(); ++dagMatId)
    {
      SimpleString shClassName = lodMatData.fileRes->getShaderClass(dagMatId);
      if (shClassName.empty())
        continue;

      EntityMatProperties &matProps = lodMatData.matProperties.push_back();
      matProps.dagMatId = dagMatId;
      matProps.facesUsingMaterial = materialUsage[dagMatId];
      matProps.matName = lodMatData.fileRes->getMaterialName(dagMatId);
      matProps.shClassName = shClassName;
      matProps.vars = lodMatData.fileRes->getVars(dagMatId);
      matProps.textures = lodMatData.fileRes->getTextureNames(dagMatId);

      MaterialData shaderSearchMatData;
      override_material_with_props(shaderSearchMatData, lodMatData.fileRes->getPropertiesBlk(dagMatId), assetName);
      ShaderMaterial *shader = find_entity_shader_material(entity, shaderSearchMatData, lod);
      lodMatData.matShaders.push_back(shader);
    }
  }

  active = matDataPerLod.size() > 0;
}

bool EntityMaterialEditor::end()
{
  if (!active || isReloading)
    return true;

  if (hasChanges())
  {
    int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Material properties",
      "You have changed material properties. Do you want to save the changes to the physical resources?");
    switch (dialogResult)
    {
      case wingw::MB_ID_CANCEL: return false;
      case wingw::MB_ID_NO:
        for (int lod = 0; lod < matDataPerLod.size(); ++lod)
          clearMaterialOverrides(lod);
        notifyCurrentAssetNeedsReload();
        break;
      case wingw::MB_ID_YES: saveAllChanges(); break;
    }
  }

  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);
  undoSystem->clear();

  active = false;
  return true;
}

void EntityMaterialEditor::saveAllChanges()
{
  for (int lod = 0; lod < matDataPerLod.size(); ++lod)
  {
    clearMaterialOverrides(lod);
    matDataPerLod[lod].save(matDataPerLod[lod].getChangedMatIds());
  }
}

void EntityMaterialEditor::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  if (!active)
    return;
  panel.disableFillAutoResize();
  panel.setEventHandler(this);
  panel.clear();

  panel.createCheckBox(PID_MATERIALS_SHOW_TRIANGLE_COUNT, "Show triangle count", showTriangleCount);

  for (int lod = 0; lod < matDataPerLod.size(); ++lod)
  {
    PropPanel::ContainerPropertyControl *grpLodMatList =
      panel.createGroup(lod * MAX_LOD_GROUP_GUI_ELEMENTS_COUNT + LPID_MATERIAL_GROUP, String(0, "Materials (Lod%d)", lod).c_str());
    if (!grpLodMatList)
      continue;

    for (int matId = 0; matId < matDataPerLod[lod].matProperties.size(); ++matId)
    {
      PropPanel::ContainerPropertyControl *grpMat =
        grpLodMatList->createGroup(get_mat_group_panel_base_pid(lod, matId) + LPID_MATERIAL_GROUP, "");
      if (grpMat)
      {
        fillMatPropPanel(lod, matId, *grpMat);
        grpMat->setBoolValue(true); // Minimize the group.
      }
    }
    grpLodMatList->setBoolValue(true);
  }

  panel.restoreFillAutoResize();
}

void EntityMaterialEditor::addShaderProps(int mat_gui_elements_baseid, PropPanel::ContainerPropertyControl &mat_panel,
  const dag::Vector<MatVarDesc> &vars, const dag::Vector<int> &var_indices, dag::ConstSpan<const ShaderVariableLimits *> var_limits)
{
  auto createExtensible = [&mat_panel](int id) {
    PropPanel::ContainerPropertyControl *extensible = mat_panel.createExtensible(id, true, "delete", "Remove parameter");
    extensible->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);
    return extensible;
  };

  for (int varId = 0; varId < vars.size(); ++varId)
  {
    const MatVarDesc &var = vars[varId];
    if (!var.usedInMaterial)
      continue;
    const int propVarId = var_indices.size() ? var_indices[varId] : varId;
    const int extensiblePropId = mat_gui_elements_baseid + LPID_SHADER_VARS_EXTENSIBLE_BASE + propVarId;
    const int propId = mat_gui_elements_baseid + LPID_SHADER_VARS_BASE + propVarId;
    G_ASSERT(propVarId >= 0);
    G_ASSERT(propVarId < MAX_MATERIAL_VAR_COUNT);
    G_ASSERT(propId == (extensiblePropId + MAX_MATERIAL_VAR_COUNT));
    switch (var.type)
    {
      case MAT_VAR_TYPE_BOOL:
      {
        PropPanel::ContainerPropertyControl *extensible = createExtensible(extensiblePropId);
        extensible->createCheckBox(propId, var.name.c_str(), var.value.i);
        set_up_shader_var_property(*extensible, propId, var, var_limits.empty() ? nullptr : var_limits[varId]);
        break;
      }

      case MAT_VAR_TYPE_ENUM_LIGHTING:
      {
        PropPanel::ContainerPropertyControl *extensible = createExtensible(extensiblePropId);

        Tab<String> vals(tmpmem);
        vals.reserve(LIGHTING_TYPE_COUNT);
        for (int i = 0; i < LIGHTING_TYPE_COUNT; ++i)
          vals.push_back(String(lightingTypeStrs[i]));
        extensible->createCombo(propId, var.name.c_str(), vals, var.value.i);
        set_up_shader_var_property(*extensible, propId, var, var_limits.empty() ? nullptr : var_limits[varId]);
        break;
      }

      case MAT_VAR_TYPE_INT:
      {
        PropPanel::ContainerPropertyControl *extensible = createExtensible(extensiblePropId);
        extensible->createEditInt(propId, var.name.c_str(), var.value.i);
        set_up_shader_var_property(*extensible, propId, var, var_limits.empty() ? nullptr : var_limits[varId]);
        break;
      }

      case MAT_VAR_TYPE_REAL:
      {
        PropPanel::ContainerPropertyControl *extensible = createExtensible(extensiblePropId);
        extensible->createEditFloat(propId, var.name.c_str(), var.value.r);
        set_up_shader_var_property(*extensible, propId, var, var_limits.empty() ? nullptr : var_limits[varId]);
        break;
      }

      case MAT_VAR_TYPE_COLOR4:
      {
        PropPanel::ContainerPropertyControl *extensible = createExtensible(extensiblePropId);

        const ShaderVariableLimits *limits = var_limits.empty() ? nullptr : var_limits[varId];
        if (limits && limits->editAsColor)
          extensible->createColorBox(propId, var.name.c_str(), e3dcolor(var.value.c4()), true, true, false);
        else
          extensible->createPoint4(propId, var.name.c_str(), Point4(var.value.c, Point4::CTOR_FROM_PTR));

        set_up_shader_var_property(*extensible, propId, var, limits);
        break;
      }

      default: break;
    }
  }
}

void EntityMaterialEditor::addShaderPropsCategories(int mat_gui_elements_baseid, PropPanel::ContainerPropertyControl &mat_panel,
  const dag::Vector<MatVarDesc> &vars, const ShaderSeparatorToPropsType &shaderSeparatorToProps)
{
  ShaderParameterGroups shaderParameterGroups;
  shaderParameterGroups.fillGroups(shaderPropSeparators, shaderSeparatorToProps, vars);

  eastl::sort(shaderParameterGroups.groups.begin(), shaderParameterGroups.groups.end(),
    [](const ShaderParameterGroup &a, const ShaderParameterGroup &b) {
      if (a.groupName.empty() || b.groupName.empty())
        return a.groupName.empty() < b.groupName.empty();

      return strcmp(a.groupName, b.groupName) < 0;
    });

  for (const ShaderParameterGroup &group : shaderParameterGroups.groups)
  {
    const int groupId = mat_gui_elements_baseid + LPID_SHADER_VARS_PARAMETER_GROUP_BASE + group.varIndices[0];
    const char *groupName = group.groupName.empty() ? (shaderParameterGroups.groups.size() == 1 ? "Parameters" : "Other parameters")
                                                    : group.groupName.c_str();
    PropPanel::ContainerPropertyControl *groupContainer =
      mat_panel.createExtGroup(groupId, groupName, "delete", "Remove all parameters in this group");
    groupContainer->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);

    addShaderProps(mat_gui_elements_baseid, *groupContainer, group.varsResult, group.varIndices, group.varLimits);
    set_up_shader_parameter_group_property(*groupContainer, group);
  }
}

DragAndDropResult ProxymatAssetDragAndDropHandler::handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  int lod, matId, matLocalPropId;
  if (!get_panel_property_address(pcb_id, lod, matId, matLocalPropId))
    return DragAndDropResult::NONE;

  const int matGuiElementsBaseId = get_mat_group_panel_base_pid(lod, matId);
  if (pcb_id != (matGuiElementsBaseId + LPID_PROXYMAT_BUTTON))
    return DragAndDropResult::NONE;

  const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery(ASSET_DRAG_AND_DROP_TYPE);
  if (!dragAndDropPayload)
    return DragAndDropResult::NONE;

  AssetDragData dragData;
  PropPanel::getDragAndDropPayloadData<AssetDragData>(dragAndDropPayload, &dragData);

  // Only allow dropping into the same asset type!
  DagorAsset *asset = dragData.asset;
  if (asset == nullptr || strcmp(asset->getTypeStr(), "proxymat") != 0)
    return DragAndDropResult::NOT_ALLOWED;

  const char *newProxyMatName = asset->getName();
  if (!newProxyMatName)
    return DragAndDropResult::NONE;

  if (dragAndDropPayload->IsDelivery())
  {
    matEditor->performSwitchToNewProxyMat(lod, matId, newProxyMatName);

    PropPanel::ContainerPropertyControl *editor_panel = panel;
    auto lod_panel = panel->getParent();
    if (lod_panel != nullptr)
    {
      editor_panel = lod_panel;
      auto material_editor_panel = lod_panel->getParent();
      if (material_editor_panel != nullptr)
        editor_panel = material_editor_panel;
    }
    matEditor->refillMatPropPanel(lod, matId, *editor_panel);

    return DragAndDropResult::ACCEPTED;
  }

  return DragAndDropResult::NONE;
}

DragAndDropResult TextureAssetDragAndDropHandler::handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  int lod, matId, matLocalPropId;
  if (!get_panel_property_address(pcb_id, lod, matId, matLocalPropId))
    return DragAndDropResult::NONE;

  const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery(ASSET_DRAG_AND_DROP_TYPE);
  if (!dragAndDropPayload)
    return DragAndDropResult::NONE;

  AssetDragData dragData;
  PropPanel::getDragAndDropPayloadData<AssetDragData>(dragAndDropPayload, &dragData);

  // Only allow dropping into the same asset type!
  DagorAsset *asset = dragData.asset;
  if (asset == nullptr || asset->getType() != asset->getMgr().getTexAssetTypeId())
    return DragAndDropResult::NOT_ALLOWED;

  const char *newMatTexName = asset->getName();
  if (!newMatTexName)
    return DragAndDropResult::NONE;

  if (dragAndDropPayload->IsDelivery())
  {
    const int matGuiElementsBaseId = get_mat_group_panel_base_pid(lod, matId);
    int texSlot = pcb_id - (matGuiElementsBaseId + LPID_TEXTURE_LIST_BASE);
    matEditor->matDataPerLod[lod].matProperties[matId].textures[texSlot] = newMatTexName;
    matEditor->updateAssetShaderMaterial(lod, matId);
    panel->setText(pcb_id, newMatTexName);

    return DragAndDropResult::ACCEPTED;
  }

  return DragAndDropResult::NONE;
}

void EntityMaterialEditor::fillMatPropPanel(int lod, int mat_id, PropPanel::ContainerPropertyControl &mat_panel)
{
  const EntityMatProperties &matProps = matDataPerLod[lod].matProperties[mat_id];
  const char *matShaderName = matProps.shClassName.c_str();
  const int matGuiElementsBaseId = get_mat_group_panel_base_pid(lod, mat_id);
  mat_panel.disableFillAutoResize();
  mat_panel.clear();

  if (showTriangleCount)
    mat_panel.setCaptionValue(String(64, "%s (%d tri)", matProps.matName.c_str(), matProps.facesUsingMaterial));
  else
    mat_panel.setCaptionValue(matProps.matName.c_str());

  mat_panel.createButton(matGuiElementsBaseId + LPID_SHADER_BUTTON, make_shader_button_name(matShaderName));

  const auto it = shaderPropSeparators.find(matShaderName);
  const ShaderDescriptor *shaderDescriptor = it == shaderPropSeparators.end() ? nullptr : &it->second;
  if (shaderDescriptor && !shaderDescriptor->description.empty())
    mat_panel.setTooltipId(matGuiElementsBaseId + LPID_SHADER_BUTTON, shaderDescriptor->description.c_str());

  SimpleString proxyMatName = matDataPerLod[lod].fileRes->getCurProxyMatName(matProps.dagMatId);
  const int proxymatButtonId = matGuiElementsBaseId + LPID_PROXYMAT_BUTTON;
  mat_panel.createButton(proxymatButtonId, make_proxymat_button_name(proxyMatName.c_str()));
  mat_panel.setDropTargetHandler(proxymatDropHandler.get());

  if (!shaderDescriptor)
    addShaderProps(matGuiElementsBaseId, mat_panel, matProps.vars);
  else
    addShaderPropsCategories(matGuiElementsBaseId, mat_panel, matProps.vars, shaderDescriptor->shaderSeparatorToProps);

  mat_panel.createSeparator();
  mat_panel.createButton(matGuiElementsBaseId + LPID_SAVE_PARAMS_BUTTON, "Save parameters");
  mat_panel.createButton(matGuiElementsBaseId + LPID_RESET_PARAMS_TO_DAG_BUTTON, "Reset parameters to DAG's");
  mat_panel.createButton(matGuiElementsBaseId + LPID_ADD_SCRIPT_VARIABLES_BUTTON, "Add params");
  mat_panel.createButton(matGuiElementsBaseId + LPID_REMOVE_SCRIPT_VARIABLES_BUTTON, "Remove params", true, false);
  mat_panel.createSeparator();
  mat_panel.createButton(matGuiElementsBaseId + LPID_COPY_PARAMS_BUTTON, "Copy params");
  mat_panel.createButton(matGuiElementsBaseId + LPID_PASTE_PARAMS_BUTTON, "Paste params", true, false);
  mat_panel.createButton(matGuiElementsBaseId + LPID_TRANSFER_CHANGES_TO_LODS, "Transfer changes to lods");

  mat_panel.createSeparator();
  PropPanel::ContainerPropertyControl *grpTextures = mat_panel.createGroup(matGuiElementsBaseId + LPID_TEXTURE_GROUP, "Textures");
  if (grpTextures)
  {
    unsigned usedTexMask = get_shclass_used_tex_mask(matShaderName);
    for (int texSlot = 0; texSlot < matProps.textures.size(); ++texSlot)
    {
      if (!(usedTexMask & (1 << texSlot)))
        continue;

      int elementId = matGuiElementsBaseId + LPID_TEXTURE_LIST_BASE + texSlot;
      grpTextures->createStatic(0, String(0, "Texture slot %d:", texSlot).c_str());
      const char *label = !matProps.textures[texSlot].empty() ? matProps.textures[texSlot].c_str() : missingResName;
      grpTextures->createButton(matGuiElementsBaseId + LPID_TEXTURE_LIST_BASE + texSlot, label);
    }
    grpTextures->setDropTargetHandler(textureDropHandler.get());
    grpTextures->setBoolValue(true);
  }

  mat_panel.restoreFillAutoResize();
}

void EntityMaterialEditor::refillMatPropPanel(int lod, int mat_id, PropPanel::ContainerPropertyControl &editor_panel)
{
  PropPanel::ContainerPropertyControl *grpMat =
    editor_panel.getContainerById(get_mat_group_panel_base_pid(lod, mat_id) + LPID_MATERIAL_GROUP);
  if (grpMat)
    fillMatPropPanel(lod, mat_id, *grpMat);
}

int EntityMaterialEditor::swapTexture(int for_lod, const char *old_tex_name, const DagorAsset *new_tex,
  PropPanel::ContainerPropertyControl &panel)
{
  const char *newMatTexName = new_tex->getName();
  if (!newMatTexName)
    return 0;

  int swapped = 0;
  for (int lod = 0; lod < matDataPerLod.size(); ++lod)
  {
    if (for_lod > -1 && lod != for_lod)
      continue;

    PropPanel::ContainerPropertyControl *grpLodMatList =
      panel.getContainerById(lod * MAX_LOD_GROUP_GUI_ELEMENTS_COUNT + LPID_MATERIAL_GROUP);
    if (!grpLodMatList)
      continue;

    for (int matId = 0; matId < matDataPerLod[lod].matProperties.size(); ++matId)
    {
      PropPanel::ContainerPropertyControl *grpMat =
        grpLodMatList->getContainerById(get_mat_group_panel_base_pid(lod, matId) + LPID_MATERIAL_GROUP);
      if (!grpMat)
        continue;

      EntityMatProperties &matProps = matDataPerLod[lod].matProperties[matId];
      const char *matShaderName = matProps.shClassName.c_str();
      const int matGuiElementsBaseId = get_mat_group_panel_base_pid(lod, matId);
      PropPanel::ContainerPropertyControl *grpTextures = grpMat->getContainerById(matGuiElementsBaseId + LPID_TEXTURE_GROUP);
      if (!grpTextures)
        continue;

      bool updateMaterial = false;
      unsigned usedTexMask = get_shclass_used_tex_mask(matShaderName);
      for (int texSlot = 0; texSlot < matProps.textures.size(); ++texSlot)
      {
        if (!(usedTexMask & (1 << texSlot)))
          continue;

        SimpleString &texName = matProps.textures[texSlot];
        if (texName == old_tex_name)
        {
          texName = newMatTexName;
          grpTextures->setText(matGuiElementsBaseId + LPID_TEXTURE_LIST_BASE + texSlot, newMatTexName);
          updateMaterial = true;
          swapped++;
        }
      }
      if (updateMaterial)
        updateAssetShaderMaterial(lod, matId);
    }
  }
  return swapped;
}

void EntityMaterialEditor::updateAssetShaderMaterialInternal(int lod, int mat_id)
{
  const EntityMatProperties &matProps = matDataPerLod[lod].matProperties[mat_id];
  ShaderMaterial *&curMatShader = matDataPerLod[lod].matShaders[mat_id];

  Tab<ShaderMaterial *> oldMat(framemem_ptr()), newMat(framemem_ptr());
  // Duplicate materials to recreate shader elems so the changes will appear.
  PERFORM_FOR_ENTITY_LOD_SCENE(entity, duplicateMat(curMatShader, oldMat, newMat), lod, CHECK);
  curMatShader = newMat.size() ? newMat.back() : nullptr;
  if (!curMatShader)
    return;

  for (int varId = 0; varId < (int)matProps.vars.size() - MAT_SPECIAL_VAR_COUNT; ++varId)
  {
    MatVarDesc curVar = matProps.vars[varId + MAT_SPECIAL_VAR_COUNT];
    if (should_skip_var(curVar.name.c_str()))
      continue;

    int shaderVarId = ::get_shader_variable_id(curVar.name.c_str(), true);
    if (shaderVarId < 0)
      continue;

    switch (curVar.type)
    {
      case MAT_VAR_TYPE_INT: curMatShader->set_int_param(shaderVarId, curVar.value.i); break;
      case MAT_VAR_TYPE_REAL: curMatShader->set_real_param(shaderVarId, curVar.value.r); break;
      case MAT_VAR_TYPE_COLOR4: curMatShader->set_color4_param(shaderVarId, curVar.value.c4()); break;
      default: break;
    }
  }
  bool realTwoSided = matProps.vars[MAT_VAR_REAL_TWO_SIDED].usedInMaterial && matProps.vars[MAT_VAR_REAL_TWO_SIDED].value.i;
  curMatShader->set_flags(realTwoSided ? SHFLG_REAL2SIDED : 0, SHFLG_REAL2SIDED);

  const bool twoSided = matProps.vars[MAT_VAR_TWOSIDED].usedInMaterial && matProps.vars[MAT_VAR_TWOSIDED].value.i;
  curMatShader->set_flags(twoSided ? SHFLG_2SIDED : 0, SHFLG_2SIDED);

  int alphaTexSlotId = get_slot_with_alpha_tex(curMatShader->native().props.textureId);
  unsigned usedTexMask = get_shclass_used_tex_mask(curMatShader->native().props.sclass);
  bool textureChanged = false;
  for (int texSlot = 0; texSlot < matProps.textures.size(); ++texSlot)
  {
    if (alphaTexSlotId == texSlot || !(usedTexMask & (1 << texSlot)))
      continue;

    String tempString;
    String texName(replace_asset_name(matProps.textures[texSlot].c_str(), tempString, assetName));
    if (!texName.empty())
      texName += "*";

    bool changed = setMatTexture(lod, mat_id, texSlot, texName.c_str());
    // Diffuse texture requires additional handling because
    // separate alpha texture is formed from its alpha channel in case we use alpha split shader.
    if (changed && str_ends_with_c(texName.c_str(), "_d*", texName.length(), 3))
    {
      if (alphaTexSlotId >= 0)
      {
        String alphaTexName = texName;
        alphaTexName.replace("_d*", "_a*");
        setMatTexture(lod, mat_id, alphaTexSlotId, alphaTexName.c_str());
      }
    }

    textureChanged |= changed;
  }

  if (textureChanged)
  {
    for (ShaderMaterial *material : newMat)
      if (ShaderElement *shaderElement = material->native().getElem())
        shaderElement->acquireTexRefs();

    for (ShaderMaterial *material : oldMat)
      if (ShaderElement *shaderElement = material->native().getElem())
        shaderElement->releaseTexRefs();
  }

  PERFORM_FOR_ENTITY_LOD_SCENE(entity, updateShaderElems(), lod, CHECK);
  if (entity->getAssetTypeId() == DAEDITOR3.getAssetTypeId("rendInst"))
    rendinst::render::reinitOnShadersReload();
}

void EntityMaterialEditor::updateAssetShaderMaterial(int lod, int mat_id)
{
  // Keep the original materials to prevent duplicateMat from releasing them, they might be used in the
  // "if (textureChanged)" block in updateAssetShaderMaterialInternal.
  Tab<ShaderMaterial *> usedMaterials;
  PERFORM_FOR_ENTITY_LOD_SCENE(entity, gatherUsedMat(usedMaterials), lod, CHECK);

  for (ShaderMaterial *material : usedMaterials)
    material->addRef();

  updateAssetShaderMaterialInternal(lod, mat_id);

  for (ShaderMaterial *material : usedMaterials)
    material->delRef();
}

void EntityMaterialEditor::performAddOrRemoveScriptVars(int lod, int mat_id, bool is_to_add)
{
  Tab<String> availableVars;
  dag::Vector<MatVarDesc> &matVars = matDataPerLod[lod].matProperties[mat_id].vars;
  for (auto &var : matVars)
    if (var.usedInMaterial != is_to_add)
      availableVars.push_back(String(var.name.c_str()));
  Tab<String> selectedVars;
  PropPanel::MultiListDialog selectVars("List of parameters", _pxScaled(300), _pxScaled(400), availableVars, selectedVars);
  if (selectVars.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    for (auto &selVarName : selectedVars)
      eastl::find_if(matVars.begin(), matVars.end(),
        // Safe, because 'selVarName' is originally taken from matVars and present there.
        [&](auto &var) { return String(var.name) == selVarName; })
        ->usedInMaterial = is_to_add;
  }
}

const char *EntityMaterialEditor::performTextureSelection(int lod, int mat_id, int tex_slot)
{
  SimpleString &texName = matDataPerLod[lod].matProperties[mat_id].textures[tex_slot];
  String texAssetName(texName.c_str());
  if (texAssetName.suffix("*"))
    texAssetName.pop_back();
  const char *newMatTexName = DAEDITOR3.selectAssetX(texAssetName.c_str(), "Select texture", "tex");
  if (newMatTexName)
  {
    if (DagorAsset *texAsset = DAEDITOR3.getAssetByName(newMatTexName))
    {
      texName = newMatTexName;
    }
    else
    {
      texName = "";
      newMatTexName = missingResName;
    }
    updateAssetShaderMaterial(lod, mat_id);
  }
  return newMatTexName;
}

const char *EntityMaterialEditor::performShaderClassSelection(int lod, int mat_id)
{
  Tab<String> availableShClasses = getAvailableShaderClasses();
  EntityLodMatData &lodMatData = matDataPerLod[lod];
  EntityMatProperties &matProperties = lodMatData.matProperties[mat_id];

  PropPanel::ListDialog selectShClass("Select shader", availableShClasses, _pxScaled(300), _pxScaled(600));
  int curSelectId = find_value_idx(availableShClasses, String(matProperties.shClassName.c_str()));
  selectShClass.setSelectedIndex(curSelectId);
  if (selectShClass.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    const char *newShaderClassName = selectShClass.getSelectedText();
    if (newShaderClassName && newShaderClassName != matProperties.shClassName)
    {
      replaceMatShaderClass(lod, mat_id, newShaderClassName);
      if (applyMaterialOverride(lod, mat_id))
        reloadCurrentAsset();
    }
  }
  return matDataPerLod[lod].matProperties[mat_id].shClassName.c_str();
}

const char *EntityMaterialEditor::performProxyMatSelection(int lod, int mat_id)
{
  EntityLodMatData &lodMatData = matDataPerLod[lod];
  EntityMatProperties &matProps = lodMatData.matProperties[mat_id];
  SimpleString curProxyMatName = lodMatData.fileRes->getCurProxyMatName(matProps.dagMatId);

  const char *newProxyMatName = DAEDITOR3.selectAssetX(curProxyMatName.c_str(), "Select proxymat", "proxymat");
  if (newProxyMatName)
  {
    performSwitchToNewProxyMat(lod, mat_id, newProxyMatName);
  }

  return newProxyMatName;
}

void EntityMaterialEditor::performSwitchToNewProxyMat(int lod, int mat_id, const char *new_proxy_mat_name)
{
  EntityLodMatData &lodMatData = matDataPerLod[lod];
  EntityMatProperties &matProps = lodMatData.matProperties[mat_id];

  lodMatData.fileRes->switchToProxyMat(matProps.dagMatId, new_proxy_mat_name);

  SimpleString oldShClassName = matProps.shClassName;
  matProps.shClassName = lodMatData.fileRes->getShaderClass(matProps.dagMatId);
  matProps.vars = lodMatData.fileRes->getVars(matProps.dagMatId);
  matProps.textures = lodMatData.fileRes->getTextureNames(matProps.dagMatId);

  if (oldShClassName != matProps.shClassName)
  {
    if (applyMaterialOverride(lod, mat_id))
      reloadCurrentAsset();
  }
  else
  {
    updateAssetShaderMaterial(lod, mat_id);
  }
}

bool EntityMaterialEditor::setMatTexture(int lod, int mat_id, int tex_slot, const char *tex_name)
{
  TEXTUREID texId = ::add_managed_texture(tex_name);
  if (matDataPerLod[lod].matShaders[mat_id]->native().props.textureId[tex_slot] != texId)
  {
    matDataPerLod[lod].matShaders[mat_id]->native().props.textureId[tex_slot] = texId;
    matDataPerLod[lod].matShaders[mat_id]->native().props.execInitCode();
    matDataPerLod[lod].matShaders[mat_id]->native().recreateMat();
    return true;
  }
  return false;
}

void EntityMaterialEditor::replaceMatShaderClass(int lod, int mat_id, const char *new_shclass_name)
{
  EntityLodMatData &lodMatData = matDataPerLod[lod];
  EntityMatProperties &matProperties = lodMatData.matProperties[mat_id];
  dag::Vector<MatVarDesc> prevMatVars = lodMatData.matProperties[mat_id].vars;
  matProperties.shClassName = new_shclass_name;
  matProperties.vars = get_shclass_vars_default(new_shclass_name);
  applyToCommonVars(matProperties.vars, prevMatVars, [](MatVarDesc &var, const MatVarDesc &prev_var) {
    var.value = prev_var.value;
    var.usedInMaterial = prev_var.usedInMaterial;
  });
}

void EntityMaterialEditor::copyMatProperties(int lod, int mat_id)
{
  const EntityLodMatData &srcLodMatData = matDataPerLod[lod];
  const EntityMatProperties &srcMatProperties = srcLodMatData.matProperties[mat_id];

  propertiesCopyBuffer = srcMatProperties;
  propertiesCopyBufferProxyMatName = srcLodMatData.fileRes->getCurProxyMatName(srcMatProperties.dagMatId);
}

void EntityMaterialEditor::pasteMatProperties(int lod, int mat_id, PropPanel::ContainerPropertyControl &editor_panel)
{
  EntityMatProperties &matProperties = matDataPerLod[lod].matProperties[mat_id];
  const SimpleString dstProxyMatName = matDataPerLod[lod].fileRes->getCurProxyMatName(matProperties.dagMatId);
  const bool srcIsProxyMat = !propertiesCopyBufferProxyMatName.empty();
  const bool dstIsProxyMat = !dstProxyMatName.empty();
  const bool shadersDiffer =
    !propertiesCopyBuffer.shClassName.empty() && matProperties.shClassName != propertiesCopyBuffer.shClassName;
  const bool shadersDifferAndCompatible =
    shadersDiffer && find_value_idx(getAvailableShaderClasses(), String(propertiesCopyBuffer.shClassName.c_str())) >= 0;

  String shaderMessage;
  if (shadersDiffer)
  {
    if (shadersDifferAndCompatible)
      shaderMessage.printf(0, "The copied shader '%s' is different from the current one.", propertiesCopyBuffer.shClassName.c_str());
    else
      shaderMessage.printf(0,
        "The copied shader '%s' is different from the current one,\nand is incompatible with asset type '%s' and can't be pasted.",
        propertiesCopyBuffer.shClassName.c_str(), DAEDITOR3.getAssetTypeName(entity->getAssetTypeId()));
  }

  int dialogResult = MaterialPasteMessageBoxDialog::PID_APPLY_ONLY_THE_PROPERTIES;

  if (!srcIsProxyMat && !dstIsProxyMat && shadersDiffer)
  {
    MaterialPasteMessageBoxDialog dialog(shaderMessage, nullptr, shadersDiffer, shadersDifferAndCompatible);
    dialog.getPanel()->removeById(MaterialPasteMessageBoxDialog::PID_CONVERT_TO_NON_PROXY_MAT);
    dialog.getPanel()->removeById(MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT);
    dialogResult = dialog.showDialog();
  }
  else if (!srcIsProxyMat && dstIsProxyMat)
  {
    MaterialPasteMessageBoxDialog dialog("The target material uses ProxyMat.", shaderMessage, shadersDiffer,
      shadersDifferAndCompatible);
    dialog.getPanel()->setCaption(MaterialPasteMessageBoxDialog::PID_APPLY_ONLY_THE_PROPERTIES,
      "Paste only the properties into current ProxyMat");
    dialog.getPanel()->setCaption(MaterialPasteMessageBoxDialog::PID_APPLY_THE_SHADER_AND_THE_PROPERTIES,
      "Paste the shader and the properties into current ProxyMat");
    dialog.getPanel()->setEnabledById(MaterialPasteMessageBoxDialog::PID_CONVERT_TO_NON_PROXY_MAT,
      !shadersDiffer || shadersDifferAndCompatible);
    dialog.getPanel()->removeById(MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT);
    dialogResult = dialog.showDialog();
  }
  else if (srcIsProxyMat && !dstIsProxyMat)
  {
    MaterialPasteMessageBoxDialog dialog("The copied material uses ProxyMat.", shaderMessage, shadersDiffer,
      shadersDifferAndCompatible);
    dialog.getPanel()->removeById(MaterialPasteMessageBoxDialog::PID_CONVERT_TO_NON_PROXY_MAT);
    dialog.getPanel()->setCaption(MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT, "Convert to ProxyMat and then paste");
    dialog.getPanel()->setEnabledById(MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT,
      !shadersDiffer || shadersDifferAndCompatible);
    dialogResult = dialog.showDialog();
  }
  else if (srcIsProxyMat && dstIsProxyMat && propertiesCopyBufferProxyMatName != dstProxyMatName.c_str())
  {
    MaterialPasteMessageBoxDialog dialog("Both the copied and the target material use ProxyMats.", shaderMessage, shadersDiffer,
      shadersDifferAndCompatible);
    dialog.getPanel()->setCaption(MaterialPasteMessageBoxDialog::PID_APPLY_ONLY_THE_PROPERTIES,
      "Paste only the properties into current ProxyMat");
    dialog.getPanel()->setCaption(MaterialPasteMessageBoxDialog::PID_APPLY_THE_SHADER_AND_THE_PROPERTIES,
      "Paste the shader and the properties into current ProxyMat");
    dialog.getPanel()->removeById(MaterialPasteMessageBoxDialog::PID_CONVERT_TO_NON_PROXY_MAT);
    dialog.getPanel()->setEnabledById(MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT,
      !shadersDiffer || shadersDifferAndCompatible);
    dialogResult = dialog.showDialog();
  }

  bool shaderClassReplaced = false;

  if (dialogResult == MaterialPasteMessageBoxDialog::PID_APPLY_ONLY_THE_PROPERTIES)
  {
    // There is nothing extra to do when only applying to the properties.
  }
  else if (dialogResult == MaterialPasteMessageBoxDialog::PID_APPLY_THE_SHADER_AND_THE_PROPERTIES)
  {
    G_ASSERT(shadersDiffer && shadersDifferAndCompatible);

    // Replace shader class first, so we will be pasting values into list of vars for the new class instead of previous.
    replaceMatShaderClass(lod, mat_id, propertiesCopyBuffer.shClassName.c_str());
    shaderClassReplaced = true;
  }
  else if (dialogResult == MaterialPasteMessageBoxDialog::PID_CONVERT_TO_NON_PROXY_MAT)
  {
    G_ASSERT(!shadersDiffer || shadersDifferAndCompatible);

    performSwitchToNewProxyMat(lod, mat_id, "");
    if (shadersDiffer)
      replaceMatShaderClass(lod, mat_id, propertiesCopyBuffer.shClassName.c_str());
    refillMatPropPanel(lod, mat_id, editor_panel);
  }
  else if (dialogResult == MaterialPasteMessageBoxDialog::PID_SET_COPIED_PROXY_MAT)
  {
    G_ASSERT(!shadersDiffer || shadersDifferAndCompatible);

    performSwitchToNewProxyMat(lod, mat_id, propertiesCopyBufferProxyMatName);
    refillMatPropPanel(lod, mat_id, editor_panel);
  }
  else // Cancel
  {
    return;
  }

  applyToCommonVars(matProperties.vars, propertiesCopyBuffer.vars, [](auto &var, const auto &copied_var) {
    if (strcmp(copied_var.name, "twosided") == 0)
    {
      // Copy twosided regardless of used or not, because it is special.
      // Twosided is not a real script value, so after saving a twosided = false value usedInMaterial will be false too.
      // How twosided is interpreted when copied to its real place, the material flag:
      // usedInMaterial: false, value: false -> false
      // usedInMaterial: false, value: true  -> false
      // usedInMaterial: true,  value: false -> false
      // usedInMaterial: true,  value: true  -> true
      var.value = copied_var.value;
      var.usedInMaterial = copied_var.usedInMaterial;
    }
    else if (copied_var.usedInMaterial)
    {
      var.value = copied_var.value;
      var.usedInMaterial = true;
    }
  });

  matProperties.textures = propertiesCopyBuffer.textures;

  updateAssetShaderMaterial(lod, mat_id);

  if (shaderClassReplaced && applyMaterialOverride(lod, mat_id))
    reloadCurrentAsset();
}

void EntityMaterialEditor::reloadCurrentAsset()
{
  isReloading = notifyCurrentAssetNeedsReload();
  if (isReloading)
  {
    for (auto &lodData : matDataPerLod)
      for (auto &shader : lodData.matShaders)
        shader = nullptr;
  }
}

bool EntityMaterialEditor::notifyCurrentAssetNeedsReload()
{
  DagorAsset *curAsset = DAEDITOR3.getAssetByName(assetName.c_str());
  if (!curAsset)
    return false;
  add_delayed_callback((delayed_callback)&notify_asset_changed, curAsset);
  return true;
}

void EntityMaterialEditor::onReloaded()
{
  for (int lod = 0; lod < matDataPerLod.size(); ++lod)
  {
    EntityLodMatData &lodMatData = matDataPerLod[lod];
    for (int matId = 0; matId < lodMatData.matProperties.size(); ++matId)
    {
      MaterialData shaderSearchMatData;
      override_material_with_props(shaderSearchMatData, makeCurMatPropertiesBlk(lod, matId), assetName);
      lodMatData.matShaders[matId] = find_entity_shader_material(entity, shaderSearchMatData, lod);
    }
  }
}

dag::Vector<int> EntityMaterialEditor::EntityLodMatData::getChangedMatIds() const
{
  dag::Vector<int> changedIds;
  for (int matId = 0; matId < matProperties.size(); ++matId)
  {
    if (isMatChanged(matId))
      changedIds.push_back(matId);
  }
  return changedIds;
}

bool EntityMaterialEditor::EntityLodMatData::isMatChanged(int mat_id) const
{
  const EntityMatProperties &props = matProperties[mat_id];
  if (fileRes->getCurProxyMatName(props.dagMatId) != fileRes->getOrigProxyMatName(props.dagMatId) ||
      props.shClassName != fileRes->getShaderClass(props.dagMatId) || props.textures != fileRes->getTextureNames(props.dagMatId))
    return true;

  dag::Vector<MatVarDesc> fileResVars = fileRes->getVars(props.dagMatId);
  if (props.vars.size() != fileResVars.size())
    return true;

  for (int varId = 0; varId < props.vars.size(); ++varId)
  {
    // Twosided needs special handling, see the comment in EntityMaterialEditor::pasteMatProperties.
    if (varId == MAT_VAR_TWOSIDED)
    {
      const bool propVarTwoSided = props.vars[varId].usedInMaterial && props.vars[varId].value.i != 0;
      const bool fileResVarTwoSided = fileResVars[varId].usedInMaterial && fileResVars[varId].value.i != 0;
      if (propVarTwoSided != fileResVarTwoSided)
        return true;
      continue;
    }

    if (!(props.vars[varId] == fileResVars[varId]))
      return true;
  }

  return false;
}

void EntityMaterialEditor::EntityLodMatData::save(const dag::Vector<int> &mats_to_save_ids)
{
  dag::Vector<const EntityMatProperties *> matPropsToSave(mats_to_save_ids.size());
  for (int i = 0; i < mats_to_save_ids.size(); ++i)
    matPropsToSave[i] = &matProperties[mats_to_save_ids[i]];
  fileRes->setDataAndSave(matPropsToSave);
}

bool EntityMaterialEditor::hasChanges() const
{
  for (auto &lodMats : matDataPerLod)
    if (lodMats.getChangedMatIds().size())
      return true;
  return false;
}

Tab<String> EntityMaterialEditor::getAvailableShaderClasses() const
{
  FastNameMap typeShClasses;
  for (int paramId = 0, n = shRemapBlk->paramCount(); paramId < n; ++paramId)
  {
    if (shRemapBlk->getParamType(paramId) == DataBlock::TYPE_STRING)
      typeShClasses.addNameId(shRemapBlk->getStr(paramId));
  }

  Tab<String> availableShClasses;
  for (auto &shClass : shBinDump().classes)
  {
    if (typeShClasses.getNameId((const char *)shClass.name) >= 0)
      availableShClasses.push_back(String((const char *)shClass.name));
  }

  return availableShClasses;
}

DataBlock EntityMaterialEditor::makeCurMatPropertiesBlk(int lod, int mat_id) const
{
  const EntityMatProperties &matProps = matDataPerLod[lod].matProperties[mat_id];
  DataBlock propsBlk = matDataPerLod[lod].fileRes->getPropertiesBlk(matProps.dagMatId);
  propsBlk.setStr("name", matProps.matName.c_str());
  replace_mat_shclass_in_props_blk(propsBlk, matProps.shClassName);
  replace_mat_vars_in_props_blk(propsBlk, matProps.vars);
  replace_mat_textures_in_props_blk(propsBlk, matProps.textures);
  return propsBlk;
}

void EntityMaterialEditor::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  int lod, matId, matLocalPropId;
  if (!get_panel_property_address(pcb_id, lod, matId, matLocalPropId))
  {
    if (pcb_id == PID_MATERIALS_SHOW_TRIANGLE_COUNT)
    {
      showTriangleCount = panel->getBool(pcb_id);
      reloadCurrentAsset();
      isReloading = false; // Do a full reload.
    }

    return;
  }

  if (matLocalPropId < LPID_TEXTURE_LIST_BASE && matLocalPropId >= LPID_SHADER_VARS_BASE)
  {
    int varId = matLocalPropId - LPID_SHADER_VARS_BASE;
    EntityMatProperties &matProps = matDataPerLod[lod].matProperties[matId];
    MatVarDesc &var = matProps.vars[varId];
    const ShaderVariableLimits *limits = getShaderVariableLimits(matProps, var);
    switch (var.type)
    {
      case MAT_VAR_TYPE_BOOL: var.value.i = panel->getBool(pcb_id); break;

      case MAT_VAR_TYPE_ENUM_LIGHTING:
      {
        SimpleString valStr = panel->getText(pcb_id);
        int val = LIGHTING_NONE;
        if (strcmp(valStr.c_str(), lightingTypeStrs[LIGHTING_LIGHTMAP]) == 0)
          val = LIGHTING_LIGHTMAP;
        else if (strcmp(valStr.c_str(), lightingTypeStrs[LIGHTING_VLTMAP]) == 0)
          val = LIGHTING_VLTMAP;
        var.value.i = val;
        break;
      }

      case MAT_VAR_TYPE_INT: var.value.i = panel->getInt(pcb_id); break;

      case MAT_VAR_TYPE_REAL: var.value.r = panel->getFloat(pcb_id); break;

      case MAT_VAR_TYPE_COLOR4:
      {
        if (limits && limits->editAsColor)
        {
          E3DCOLOR color = panel->getColor(pcb_id);
          var.value.c4() = Color4(color);
        }
        else
        {
          Point4 val = panel->getPoint4(pcb_id);
          var.value.c4() = Color4(val.x, val.y, val.z, val.w);
        }
        break;
      }

      case MAT_VAR_TYPE_NONE: break; // to prevent the unhandled switch case error
    }
    set_up_shader_var_property(*panel, pcb_id, var, limits);
    update_shader_parameter_group_property(*panel, pcb_id, shaderPropSeparators, matProps, varId);
    updateAssetShaderMaterial(lod, matId);
  }
}

bool EntityMaterialEditor::applyMaterialOverride(int lod, int mat_id)
{
  DagorAsset *curAsset = DAEDITOR3.getAssetByName(assetName.c_str());
  if (!curAsset)
    return false;

  DataBlock *lodProps = find_asset_lod_props(curAsset->props, lod);
  if (!lodProps)
    return false;

  const EntityMatProperties &matProps = matDataPerLod[lod].matProperties[mat_id];

  DataBlock *materialOverrides = lodProps->addBlock("materialOverrides");
  DataBlock *matOverrideProps = find_mat_override_props(*materialOverrides, matProps.matName.c_str());
  if (!matOverrideProps)
    matOverrideProps = materialOverrides->addNewBlock("material");

  DataBlock propsBlk = makeCurMatPropertiesBlk(lod, mat_id);
  matOverrideProps->setFrom(&propsBlk);

  return true;
}

void EntityMaterialEditor::clearMaterialOverrides(int lod)
{
  DagorAsset *curAsset = DAEDITOR3.getAssetByName(assetName.c_str());
  if (!curAsset)
    return;

  DataBlock *lodProps = find_asset_lod_props(curAsset->props, lod);
  if (lodProps)
    lodProps->removeBlock("materialOverrides");
}

void EntityMaterialEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  int lod, matId, matLocalPropId;
  if (!get_panel_property_address(pcb_id, lod, matId, matLocalPropId))
    return;

  if (matLocalPropId >= LPID_TEXTURE_LIST_BASE)
  {
    int texSlot = matLocalPropId - LPID_TEXTURE_LIST_BASE;
    const char *newMatTexName = performTextureSelection(lod, matId, texSlot);
    if (newMatTexName)
      panel->setText(pcb_id, newMatTexName);
  }
  else if (matLocalPropId >= LPID_SHADER_VARS_PARAMETER_GROUP_BASE && matLocalPropId < LPID_SHADER_VARS_EXTENSIBLE_BASE)
  {
    const int groupFirstVarId = matLocalPropId - LPID_SHADER_VARS_PARAMETER_GROUP_BASE;
    const EntityMatProperties &matProps = matDataPerLod[lod].matProperties[matId];
    const char *matShaderName = matProps.shClassName.c_str();
    const auto shaderIt = shaderPropSeparators.find(matShaderName);
    const ShaderDescriptor *shaderDescriptor = shaderIt == shaderPropSeparators.end() ? nullptr : &shaderIt->second;

    if (shaderDescriptor)
    {
      beginUndo(lod, matId, matProps);

      ShaderParameterGroups shaderParameterGroups;
      shaderParameterGroups.fillGroups(shaderPropSeparators, shaderDescriptor->shaderSeparatorToProps, matProps.vars);

      if (const ShaderParameterGroup *group = shaderParameterGroups.getGroupByVariableIndex(groupFirstVarId))
        for (int varId : group->varIndices)
          matDataPerLod[lod].matProperties[matId].vars[varId].usedInMaterial = false;

      updateAssetShaderMaterial(lod, matId);
      refillMatPropPanel(lod, matId, *panel);

      endUndo("Removing parameter group", true);
    }
  }
  else if (matLocalPropId >= LPID_SHADER_VARS_EXTENSIBLE_BASE && matLocalPropId < LPID_SHADER_VARS_BASE)
  {
    EntityMatProperties &matProps = matDataPerLod[lod].matProperties[matId];
    const int varId = matLocalPropId - LPID_SHADER_VARS_EXTENSIBLE_BASE;
    MatVarDesc &var = matProps.vars[varId];

    beginUndo(lod, matId, matProps);

    var.usedInMaterial = false;
    updateAssetShaderMaterial(lod, matId);
    refillMatPropPanel(lod, matId, *panel);

    endUndo("Removing parameter", true);
  }
  else
  {
    EntityLodMatData &lodMatData = matDataPerLod[lod];
    EntityMatProperties &matProps = lodMatData.matProperties[matId];
    switch (matLocalPropId)
    {
      case LPID_SHADER_BUTTON:
      {
        const char *newShaderClassName = performShaderClassSelection(lod, matId);
        if (newShaderClassName)
        {
          panel->setText(pcb_id, make_shader_button_name(newShaderClassName));

          const auto it = shaderPropSeparators.find(newShaderClassName);
          const ShaderDescriptor *shaderDescriptor = it == shaderPropSeparators.end() ? nullptr : &it->second;
          panel->setTooltipId(pcb_id, shaderDescriptor ? shaderDescriptor->description.c_str() : "");
        }
        break;
      }
      case LPID_PROXYMAT_BUTTON:
      {
        const char *newProxyMatName = performProxyMatSelection(lod, matId);
        if (newProxyMatName)
          refillMatPropPanel(lod, matId, *panel);
        break;
      }
      case LPID_ADD_SCRIPT_VARIABLES_BUTTON:
      case LPID_REMOVE_SCRIPT_VARIABLES_BUTTON:
        performAddOrRemoveScriptVars(lod, matId, matLocalPropId == LPID_ADD_SCRIPT_VARIABLES_BUTTON);
        updateAssetShaderMaterial(lod, matId);
        refillMatPropPanel(lod, matId, *panel);
        break;
      case LPID_SAVE_PARAMS_BUTTON:
        if (lodMatData.isMatChanged(matId) &&
            wingw::message_box(wingw::MBS_OKCANCEL, "Save parameters",
              lodMatData.fileRes->getCurProxyMatName(matProps.dagMatId).empty()
                ? "This will overwrite current LOD's DAG"
                : "This will overwrite proxymat used by the material. All assets using this proxymat will be affected.") ==
              wingw::MB_ID_OK)
        {
          clearMaterialOverrides(lod);
          lodMatData.save({matId});

          const SimpleString proxyMatName = lodMatData.fileRes->getCurProxyMatName(matProps.dagMatId);
          if (!proxyMatName.empty())
            modifyAllAffectedLoadedAssets(proxyMatName);
        }
        break;
      case LPID_COPY_PARAMS_BUTTON: copyMatProperties(lod, matId); break;
      case LPID_PASTE_PARAMS_BUTTON:
        pasteMatProperties(lod, matId, *panel);
        refillMatPropPanel(lod, matId, *panel);
        break;
      case LPID_RESET_PARAMS_TO_DAG_BUTTON:
        if (lodMatData.isMatChanged(matId) &&
            wingw::message_box(wingw::MBS_OKCANCEL, "Reset parameters", "This will discard your changes!") == wingw::MB_ID_OK)
        {
          DagorAsset *curAsset = DAEDITOR3.getAssetByName(assetName.c_str());
          if (!curAsset)
            return;
          EntityMatProperties &matProps = lodMatData.matProperties[matId];
          matProps.matName = lodMatData.fileRes->getMaterialName(matProps.dagMatId);
          matProps.shClassName = lodMatData.fileRes->getShaderClass(matProps.dagMatId);
          matProps.vars = lodMatData.fileRes->getVars(matProps.dagMatId);
          matProps.textures = lodMatData.fileRes->getTextureNames(matProps.dagMatId);
          updateAssetShaderMaterial(lod, matId);
          refillMatPropPanel(lod, matId, *panel);
        }
        break;
      case LPID_TRANSFER_CHANGES_TO_LODS: transferChangesToLods(lod, matId, panel); break;
    }
  }
}

void EntityMaterialEditor::applyToCommonVars(dag::Vector<MatVarDesc> &dst_vars, const dag::Vector<MatVarDesc> &src_vars,
  eastl::function<void(MatVarDesc &, const MatVarDesc &)> common_var_cb)
{
  for (auto &dstVar : dst_vars)
  {
    auto pSrcVar = eastl::find_if(src_vars.begin(), src_vars.end(), [&dstVar](auto &src_var) { return src_var.name == dstVar.name; });
    if (pSrcVar != src_vars.end())
      common_var_cb(dstVar, *pSrcVar);
  }
}

void EntityMaterialEditor::transferChangesToLods(int lod, int mat_id, PropPanel::ContainerPropertyControl *panel)
{
  const EntityMatProperties &matPropsSrc = matDataPerLod[lod].matProperties[mat_id];
  const dag::Vector<MatVarDesc> &matVarsSrc = matPropsSrc.vars;
  const SimpleString &matNameSrc = matDataPerLod[lod].fileRes->getMaterialName(matPropsSrc.dagMatId);

  Tab<String> vals;
  Tab<int> lods;
  Tab<int> matIndices;
  for (int i = 0; i < matDataPerLod.size(); ++i)
  {
    if (i == lod)
      continue;
    for (int j = 0; j < matDataPerLod[i].matProperties.size(); ++j)
    {
      const EntityMatProperties &matProps = matDataPerLod[i].matProperties[j];
      const SimpleString &matName = matDataPerLod[i].fileRes->getMaterialName(matProps.dagMatId);
      if (matName == matNameSrc)
      {
        vals.push_back(String(0, "lod%02d", i));
        lods.push_back(i);
        matIndices.push_back(j);
        break;
      }
    }
  }
  Tab<String> sels;
  PropPanel::MultiListDialog dlg("List of lods", _pxScaled(300), _pxScaled(400), vals, sels);
  Tab<int> selIndices;
  dlg.setSelectionTab(&selIndices);
  if (dlg.showDialog() != PropPanel::DIALOG_ID_OK)
    return;
  for (int sel : selIndices)
  {
    int dstLod = lods[sel];
    int dstMatId = matIndices[sel];
    EntityMatProperties &matPropsDst = matDataPerLod[dstLod].matProperties[dstMatId];
    applyToCommonVars(matPropsDst.vars, matVarsSrc, [](MatVarDesc &var, const MatVarDesc &src_var) {
      var.value = src_var.value;
      var.usedInMaterial = src_var.usedInMaterial;
    });
    refillMatPropPanel(dstLod, dstMatId, *panel);
    updateAssetShaderMaterial(dstLod, dstMatId);
  }
}

dag::Vector<DagorAsset *> EntityMaterialEditor::getLoadedRendInstAssets(const DagorAssetMgr &assetMgr)
{
  dag::Vector<DagorAsset *> rendInstAssets;

  const int rendInstAssetType = assetMgr.getAssetTypeId("rendInst");
  GameResourceFactory *rendInstGameResFactory = find_gameres_factory(RendInstGameResClassId);
  dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();

  for (DagorAsset *asset : assets)
    if (asset->getType() == rendInstAssetType)
    {
      const int resId = gamereshooks::aux_game_res_handle_to_id(asset->getName(), RendInstGameResClassId);
      if (rendInstGameResFactory && rendInstGameResFactory->isResLoaded(resId))
        rendInstAssets.push_back(asset);
    }

  return rendInstAssets;
}

bool EntityMaterialEditor::isAssetDependsOnProxyMat(const DagorAssetMgr &assetMgr, const DagorAsset &asset,
  const char *proxy_mat_asset_file_name)
{
  IDagorAssetExporter *exporter = assetMgr.getAssetExporter(asset.getType());
  if (!exporter)
    return false;

  Tab<SimpleString> files;
  exporter->gatherSrcDataFiles(asset, files);
  for (const SimpleString &srcFile : files)
    if (stricmp(dd_get_fname(srcFile), proxy_mat_asset_file_name) == 0)
      return true;

  return false;
}

void EntityMaterialEditor::modifyAllAffectedLoadedAssets(const char *proxy_mat_Name)
{
  const DagorAsset *proxyMatAsset = DAEDITOR3.getAssetByName(proxy_mat_Name);
  if (!proxyMatAsset)
    return;

  const String proxyMatAssetFileName(proxyMatAsset->getSrcFileName());
  const DagorAssetMgr &assetMgr = get_app().getAssetMgr();
  const dag::Vector<DagorAsset *> assetsToReload = getLoadedRendInstAssets(assetMgr);

  for (DagorAsset *asset : assetsToReload)
  {
    // Those assets that are in the same directory where the proxy material is, are
    // handled by DagorAssetMgr::trackChangesContinuous in assetMgrTrackChanges.cpp.
    if (asset->getFolderIndex() == proxyMatAsset->getFolderIndex())
      continue;

    if (!isAssetDependsOnProxyMat(assetMgr, *asset, proxyMatAssetFileName))
      continue;

    // Using delayed callback here because if the asset is the current asset then
    // reloading it would destroy the notifier control's instance, and cause a crash
    // in WindowBaseHandler::controlProc.
    add_delayed_callback((delayed_callback)&notify_asset_changed, asset);
  }
}

void EntityMaterialEditor::beginUndo(int lod, int material_index, const EntityMatProperties &material_properties)
{
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);

  undoSystem->begin();
  undoSystem->put(new MaterialEditorUndoObject(*this, lod, material_index));
}

void EntityMaterialEditor::endUndo(const char *operation_name, bool accept)
{
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);

  if (accept)
    undoSystem->accept(operation_name);
  else
    undoSystem->cancel();
}

void EntityMaterialEditor::saveForUndo(int lod, int material_index, EntityMatProperties &undo_material_properties)
{
  if (lod < 0 || lod >= matDataPerLod.size())
  {
    logdbg("EntityMaterialEditor: invalid LOD index (%d/%d). Undo will not saved.", lod, matDataPerLod.size());
    return;
  }

  EntityLodMatData &lodMaterialData = matDataPerLod[lod];

  if (material_index < 0 || material_index >= lodMaterialData.matProperties.size())
  {
    logdbg("EntityMaterialEditor: invalid material index (%d/%d). Undo will not be saved.", material_index,
      lodMaterialData.matProperties.size());
    return;
  }

  undo_material_properties = matDataPerLod[lod].matProperties[material_index];
}

void EntityMaterialEditor::loadFromUndo(int lod, int material_index, const EntityMatProperties &undo_material_properties)
{
  if (!active)
    return;

  PropPanel::ContainerPropertyControl *materialEditorGroup = get_material_editor_group();
  G_ASSERT(materialEditorGroup);

  if (lod < 0 || lod >= matDataPerLod.size())
  {
    logdbg("EntityMaterialEditor: invalid LOD index in the undo object (%d/%d). Undo will not be applied.", lod, matDataPerLod.size());
    return;
  }

  EntityLodMatData &lodMaterialData = matDataPerLod[lod];

  if (material_index < 0 || material_index >= lodMaterialData.matProperties.size())
  {
    logdbg("EntityMaterialEditor: invalid material index in the undo object (%d/%d). Undo will not be applied.", material_index,
      lodMaterialData.matProperties.size());
    return;
  }

  EntityMatProperties &materialProperties = lodMaterialData.matProperties[material_index];

  if (undo_material_properties.vars.size() != materialProperties.vars.size())
  {
    logdbg("EntityMaterialEditor: different variable count in the undo object (%d) and in the editor (%d). Undo will not be applied.",
      undo_material_properties.vars.size(), materialProperties.vars.size());
    return;
  }

  for (int variableIndex = 0; variableIndex < materialProperties.vars.size(); ++variableIndex)
  {
    const MatVarDesc &undoVar = undo_material_properties.vars[variableIndex];
    const MatVarDesc &var = materialProperties.vars[variableIndex];

    if (strcmp(undoVar.name, var.name) != 0)
    {
      logdbg("EntityMaterialEditor: different variable order in the undo object (%d, %s) and in the editor (%d, %s). Undo will not be "
             "applied.",
        variableIndex, undoVar.name.c_str(), variableIndex, var.name.c_str());
      return;
    }
  }

  for (int variableIndex = 0; variableIndex < undo_material_properties.vars.size(); ++variableIndex)
  {
    const MatVarDesc &undoVar = undo_material_properties.vars[variableIndex];
    MatVarDesc &var = materialProperties.vars[variableIndex];
    var.usedInMaterial = undoVar.usedInMaterial;
  }

  updateAssetShaderMaterial(lod, material_index);
  refillMatPropPanel(lod, material_index, *materialEditorGroup);
}
