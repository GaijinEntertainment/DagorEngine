// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "entityMatEditor.h"

#include <assets/assetExporter.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <shaders/dag_shMaterialUtils.h>
#include <EASTL/algorithm.h>
#include <propPanel/control/container.h>
#include <shaders/dag_shaderCommon.h>
#include <obsolete/dag_cfg.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <de3_entityGetSceneLodsRes.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <memory/dag_framemem.h>
#include <de3_interface.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/commonWindow/listDialog.h>
#include <propPanel/commonWindow/multiListDialog.h>
#include <libTools/util/strUtil.h>
#include <math/dag_mesh.h>
#include <util/dag_strUtil.h>
#include <util/dag_delayedAction.h>
#include <libTools/shaderResBuilder/processMat.h>
#include "../../../engine/shaders/scriptSElem.h"
#include "../../../engine/shaders/scriptSMat.h"
#include "../av_appwnd.h"
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>

using hdpi::_pxScaled;

namespace gamereshooks
{
int aux_game_res_handle_to_id(GameResHandle h, unsigned cid);
}

enum
{
  MAX_MATERIAL_VAR_COUNT = 128,
  MAX_MATERIAL_TEXTURE_COUNT = 64,
  MAX_MATERIAL_EXTRA_CONTROLS_COUNT = 64,
  MAX_MATERIAL_GROUP_GUI_ELEMENTS_COUNT = MAX_MATERIAL_VAR_COUNT + MAX_MATERIAL_TEXTURE_COUNT + MAX_MATERIAL_EXTRA_CONTROLS_COUNT,

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
  LPID_SHADER_VARS_BASE,
  LPID_TEXTURE_LIST_BASE = LPID_SHADER_VARS_BASE + MAX_MATERIAL_VAR_COUNT
};

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

static String make_shader_button_name(const char *shclass_name) { return String(0, "Shader: %s", shclass_name); }

static String make_proxymat_button_name(const char *proxymat_name)
{
  return String(0, "ProxyMat: %s", *proxymat_name ? proxymat_name : missingResName);
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
  if (!asset)
    return;

  assetSrcFolderPath = asset->getFolderPath();
  assetName = asset->getName();

  if (appBlk.isEmpty())
  {
    appBlk = DataBlock(::get_app().getWorkspace().getAppPath());
    shaderPropSeparators = gatherParameterSeparators(appBlk, ::get_app().getWorkspace().getAppDir());
  }

  const DataBlock *curTypeBlk = appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx(
    DAEDITOR3.getAssetTypeName(asset_entity->getAssetTypeId()));
  shRemapBlk = curTypeBlk->getBlockByNameEx("remapShaders");

  matDataPerLod.clear();

  int lodCount = 0;
  for (int lod = 0;; ++lod)
  {
    const String dagFileName = get_asset_dag_file_path(*asset, lod);
    if (!dd_file_exists(dagFileName.c_str()))
    {
      lodCount = lod;
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
      case PropPanel::DIALOG_ID_CANCEL: return false;
      case PropPanel::DIALOG_ID_NO:
        for (int lod = 0; lod < matDataPerLod.size(); ++lod)
          clearMaterialOverrides(lod);
        notifyCurrentAssetNeedsReload();
        break;
      case PropPanel::DIALOG_ID_YES: saveAllChanges(); break;
    }
  }

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
  const dag::Vector<MatVarDesc> &vars, const dag::Vector<int> &var_indices)
{
  for (int varId = 0; varId < vars.size(); ++varId)
  {
    const MatVarDesc &var = vars[varId];
    if (!var.usedInMaterial)
      continue;
    int propVarId = var_indices.size() ? var_indices[varId] : varId;
    int propId = mat_gui_elements_baseid + LPID_SHADER_VARS_BASE + propVarId;
    switch (var.type)
    {
      case MAT_VAR_TYPE_BOOL: mat_panel.createCheckBox(propId, var.name.c_str(), var.value.i); break;
      case MAT_VAR_TYPE_ENUM_LIGHTING:
      {
        Tab<String> vals(tmpmem);
        vals.reserve(LIGHTING_TYPE_COUNT);
        for (int i = 0; i < LIGHTING_TYPE_COUNT; ++i)
          vals.push_back(String(lightingTypeStrs[i]));
        mat_panel.createCombo(propId, var.name.c_str(), vals, var.value.i);
        break;
      }
      case MAT_VAR_TYPE_INT: mat_panel.createEditInt(propId, var.name.c_str(), var.value.i); break;
      case MAT_VAR_TYPE_REAL: mat_panel.createEditFloat(propId, var.name.c_str(), var.value.r); break;
      case MAT_VAR_TYPE_COLOR4: mat_panel.createPoint4(propId, var.name.c_str(), Point4(var.value.c, Point4::CTOR_FROM_PTR)); break;
    }
  }
}

void EntityMaterialEditor::addShaderPropsCategories(int mat_gui_elements_baseid, PropPanel::ContainerPropertyControl &mat_panel,
  const dag::Vector<MatVarDesc> &vars, const ShaderSeparatorToPropsType &shaderSeparatorToProps)
{
  auto isVarInProps = [](const ShaderSeparatorToPropsType &props, const SimpleString &var_name) {
    for (const auto &kv : props)
      if (kv.second.find(var_name.c_str()) != kv.second.end())
        return true;
    return false;
  };

  // Global properties
  auto globalsIt = shaderPropSeparators.find("_global_params");
  if (globalsIt != shaderPropSeparators.end())
  {
    dag::Vector<MatVarDesc> varsResult;
    dag::Vector<int> varIndices;
    const ShaderSeparatorToPropsType &props = globalsIt->second;
    for (const auto &kv : props)
    {
      for (int varId = 0; varId < vars.size(); ++varId)
      {
        const MatVarDesc &var = vars[varId];
        if (!var.usedInMaterial)
          continue;
        if (kv.second.find(var.name.c_str()) != kv.second.end() && !isVarInProps(shaderSeparatorToProps, var.name))
        {
          varsResult.push_back(var);
          varIndices.push_back(varId);
        }
      }
      if (!varsResult.empty())
      {
        mat_panel.createStatic(0, "Global parameters");
        addShaderProps(mat_gui_elements_baseid, mat_panel, varsResult, varIndices);
      }
    }

    // Shader properties
    for (const auto &kv : shaderSeparatorToProps)
    {
      dag::Vector<MatVarDesc> varsResult;
      dag::Vector<int> varIndices;
      for (int varId = 0; varId < vars.size(); ++varId)
      {
        const MatVarDesc &var = vars[varId];
        if (!var.usedInMaterial)
          continue;
        if (kv.second.find(var.name.c_str()) != kv.second.end())
        {
          varsResult.push_back(var);
          varIndices.push_back(varId);
        }
      }
      if (!varsResult.empty())
      {
        mat_panel.createSeparator();
        if (!kv.first.empty())
          mat_panel.createStatic(0, kv.first.c_str());
        addShaderProps(mat_gui_elements_baseid, mat_panel, varsResult, varIndices);
      }
    }

    // Gather shader vars not listed in dagorShaders.cfg
    dag::Vector<MatVarDesc> unlistedVars;
    varIndices.clear();
    for (int varId = 0; varId < vars.size(); ++varId)
    {
      const MatVarDesc &var = vars[varId];
      if (!var.usedInMaterial)
        continue;
      const ShaderSeparatorToPropsType &props = globalsIt->second;
      if (!isVarInProps(props, var.name) && !isVarInProps(shaderSeparatorToProps, var.name))
      {
        unlistedVars.push_back(var);
        varIndices.push_back(varId);
      }
    }

    mat_panel.createSeparator();
    addShaderProps(mat_gui_elements_baseid, mat_panel, unlistedVars, varIndices);
  }
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
  SimpleString proxyMatName = matDataPerLod[lod].fileRes->getCurProxyMatName(matProps.dagMatId);
  mat_panel.createButton(matGuiElementsBaseId + LPID_PROXYMAT_BUTTON, make_proxymat_button_name(proxyMatName.c_str()));

  const auto it = shaderPropSeparators.find(matShaderName);
  if (it == eastl::end(shaderPropSeparators))
    addShaderProps(matGuiElementsBaseId, mat_panel, matProps.vars);
  else
    addShaderPropsCategories(matGuiElementsBaseId, mat_panel, matProps.vars, it->second);

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

  if (entity->getAssetTypeId() == DAEDITOR3.getAssetTypeId("rendInst"))
  {
    int resIdx = rendinst::addRIGenExtraResIdx(assetName.c_str(), -1, -1, rendinst::AddRIFlag::UseShadow);
    RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(resIdx);
    res->updateShaderElems();
    rendinst::render::reinitOnShadersReload();
  }
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
    lodMatData.fileRes->switchToProxyMat(matProps.dagMatId, newProxyMatName);

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

  return newProxyMatName;
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

void EntityMaterialEditor::copyMatProperties(int lod, int mat_id) { propertiesCopyBuffer = matDataPerLod[lod].matProperties[mat_id]; }

void EntityMaterialEditor::pasteMatProperties(int lod, int mat_id)
{
  EntityMatProperties &matProperties = matDataPerLod[lod].matProperties[mat_id];

  // Replace shader class first, so we will be pasting values into
  // list of vars for the new class instead of previous.
  bool shaderClassReplaced = false;
  if (!propertiesCopyBuffer.shClassName.empty() && matProperties.shClassName != propertiesCopyBuffer.shClassName)
  {
    if (wingw::message_box(wingw::MBS_YESNO, "Shaders difference",
          "The copied shader '%s' is different from the current one. Do you want to paste it?",
          propertiesCopyBuffer.shClassName.c_str()) == PropPanel::DIALOG_ID_YES)
    {
      if (find_value_idx(getAvailableShaderClasses(), String(propertiesCopyBuffer.shClassName.c_str())) >= 0)
      {
        replaceMatShaderClass(lod, mat_id, propertiesCopyBuffer.shClassName.c_str());
        shaderClassReplaced = true;
      }
      else
      {
        wingw::message_box(wingw::MBS_OK, "Shaders difference", "Shader '%s' is incompatible with asset type '%s' and can't be pasted",
          propertiesCopyBuffer.shClassName.c_str(), DAEDITOR3.getAssetTypeName(entity->getAssetTypeId()));
      }
    }
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
    MatVarDesc &var = matDataPerLod[lod].matProperties[matId].vars[varId];
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
        Point4 val = panel->getPoint4(pcb_id);
        var.value.c4() = Color4(val.x, val.y, val.z, val.w);
        break;
      }
    }
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
          panel->setText(pcb_id, make_shader_button_name(newShaderClassName));
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
              PropPanel::DIALOG_ID_OK)
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
        pasteMatProperties(lod, matId);
        refillMatPropPanel(lod, matId, *panel);
        break;
      case LPID_RESET_PARAMS_TO_DAG_BUTTON:
        if (lodMatData.isMatChanged(matId) &&
            wingw::message_box(wingw::MBS_OKCANCEL, "Reset parameters", "This will discard your changes!") == PropPanel::DIALOG_ID_OK)
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
      const GameResHandle resHandle = GAMERES_HANDLE_FROM_STRING(asset->getName());
      const int resId = gamereshooks::aux_game_res_handle_to_id(resHandle, RendInstGameResClassId);
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
