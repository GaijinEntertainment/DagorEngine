// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "materials.h"
#include "mat_cm.h"
#include "mat_shader.h"
#include "mat_paramDesc.h"
#include "mat_param.h"
#include "mat_util.h"

#include <assetsGui/av_selObjDlg.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>

#include <EditorCore/ec_wndPublic.h>
#include <propPanel/control/container.h>

#include <3d/dag_materialData.h>

#include <stdlib.h>


//==================================================================================================
void MaterialsPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  panel.setEventHandler(this);

  Tab<String> shaders(tmpmem);
  getShadersNames(shaders);

  int sel = 0;
  for (int i = 0; i < shaders.size(); ++i)
    if (curMat->className == shaders[i])
    {
      sel = i;
      break;
    }
  panel.createCombo(PID_SHADER, "Shader:", shaders, sel);
  panel.createGroupBox(PID_SHADER_PANEL, "Shader parameters");

  fillShaderPanel(&panel);
}

//==================================================================================================

void MaterialsPlugin::fillShaderPanel(PropPanel::ContainerPropertyControl *panel)
{
  G_ASSERT(panel->getById(PID_SHADER_PANEL) && "MaterialsPlugin::fillPropPanel: No shader panel found!!!");

  PropPanel::ContainerPropertyControl *_shader_panel = panel->getById(PID_SHADER_PANEL)->getContainer();

  G_ASSERT(_shader_panel && "MaterialsPlugin::fillPropPanel: No shader panel found!!! (line 2)");

  const MatShader *shader = findShader(curMat->className);

  if (!shader)
    return;

  _shader_panel->clear();
  fillShaderParams(_shader_panel, *curMat, *shader);
}


void MaterialsPlugin::fillShaderParams(PropPanel::ContainerPropertyControl *panel, const MaterialRec &mat, const MatShader &shader)
{
  shaderParamsNames.clear();

  for (int i = 0; i < shader.getParamCount(); ++i)
  {
    const MatShader::ParamRec &param = shader.getParam(i);

    const MaterialParamDescr *descr = findParameterDescr(param.name);

    if (descr)
    {
      shaderParamsNames.push_back() = descr->getName();
      int _id = PID_FIRST_PARAM + (i * PID_PARAM_PIDS_COUNT);

      switch (descr->getType())
      {
        case MaterialParamDescr::PT_TEXTURE: addTexture(panel, mat, descr->getCaption(), _id, descr->getSlot()); break;

        case MaterialParamDescr::PT_TRIPLE_INT:
          addTripleInt(panel, descr->getCaption(), descr->getName(), descr->getIntConstains(), _id);
          break;

        case MaterialParamDescr::PT_TRIPLE_REAL:
          addTripleReal(panel, descr->getCaption(), descr->getName(), descr->getRealConstains(), _id);
          break;

        case MaterialParamDescr::PT_E3DCOLOR: addE3dColor(panel, descr->getCaption(), descr->getName(), descr->getSlot(), _id); break;

        case MaterialParamDescr::PT_COMBO:
          addCombo(panel, descr->getCaption(), descr->getName(), descr->getComboVals(), descr->getDefStrVal(), _id);
          break;

        case MaterialParamDescr::PT_CUSTOM: addCustom(panel, descr->getName(), _id); break;

        default: break;
      }
    }
  }
}


//==================================================================================================

void MaterialsPlugin::addTexture(PropPanel::ContainerPropertyControl *panel, const MaterialRec &mat, const char *caption, int ctrl_idx,
  int tex_idx) const
{
  const char *texCapt = mat.getTexRef(tex_idx);

  panel->createStatic(-1, caption);
  panel->createButton(ctrl_idx, texCapt);
}

//==================================================================================================

void MaterialsPlugin::addTripleInt(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param,
  const IPoint2 &constrains, int ctrl_idx) const
{
  String paramVal(::get_script_param(curMat->script, script_param));
  int val = paramVal.length() ? atoi(paramVal) : 0;

  panel->createCheckBox(ctrl_idx, "Use default", paramVal.length() ? false : true);
  panel->createTrackInt(ctrl_idx + 1, caption, val, constrains.x, constrains.y, 1, !panel->getBool(ctrl_idx));
}

//==================================================================================================

void MaterialsPlugin::addTripleReal(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param,
  const Point2 &constrains, int ctrl_idx) const
{
  String paramVal(::get_script_param(curMat->script, script_param));
  real val = getCorrectRealVal(paramVal.length() ? atof(paramVal) : 0, constrains);

  panel->createCheckBox(ctrl_idx, "Use default", paramVal.length() ? false : true);
  panel->createEditFloat(ctrl_idx + 1, caption, val, 2, !panel->getBool(ctrl_idx));
  panel->setMinMaxStep(ctrl_idx + 1, constrains.x, constrains.y, 0.01);
}

//==================================================================================================

void MaterialsPlugin::addE3dColor(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param, int slot,
  int ctrl_idx) const
{
  if (slot == -1)
    return;

  Color4 val;
  if (slot == 0)
    val = curMat->mat.diff;
  else if (slot == 1)
    val = curMat->mat.amb;
  else if (slot == 2)
    val = curMat->mat.spec;
  else if (slot == 3)
    val = curMat->mat.emis;
  bool def = (curMat->defMatCol & (1 << slot)) ? true : false;

  panel->createCheckBox(ctrl_idx, "Use default", def);
  panel->createColorBox(ctrl_idx + 1, caption, e3dcolor(val), !def);
}

//==================================================================================================

void MaterialsPlugin::addCombo(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param,
  const Tab<String> &vals, const char *def, int ctrl_idx)
{
  String paramVal(::get_script_param(curMat->script, script_param));

  if (!paramVal.length())
  {
    ::set_script_param(curMat->script, script_param, def);
    paramVal = def;

    updatePreview();
  }

  int _sel = -1;
  for (int i = 0; i < vals.size(); ++i)
  {
    if (paramVal == vals[i])
      _sel = i;
  }

  panel->createCombo(ctrl_idx, caption, vals, _sel);
}

//==================================================================================================

void MaterialsPlugin::addCustom(PropPanel::ContainerPropertyControl *panel, const char *script_param, int ctrl_idx) const
{
  if (!stricmp(script_param, "two_sided"))
  {
    Tab<String> vals(tmpmem);
    vals.resize(3);
    vals[0] = "None";
    vals[1] = "Two sided";
    vals[2] = "Two sided real";

    int _sel = 0;

    if (curMat->flags & MATF_2SIDED)
      _sel = 1;

    String paramVal(::get_script_param(curMat->script, "real_two_sided"));
    if (paramVal.length() &&
        (!stricmp(paramVal, "yes") || !stricmp(paramVal, "true") || !stricmp(paramVal, "1") || !stricmp(paramVal, "on")))
      _sel = 2;

    panel->createCombo(ctrl_idx, "Two sided:", vals, _sel);
  }
}

//==================================================================================================

IPoint2 MaterialsPlugin::getParamIdxFromPid(int pid) const
{
  IPoint2 ret;
  ret.x = (pid - PID_FIRST_PARAM) / PID_PARAM_PIDS_COUNT;
  ret.y = pid - PID_FIRST_PARAM - ret.x * PID_PARAM_PIDS_COUNT;

  return ret;
}


//==================================================================================================

void MaterialsPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id >= PID_FIRST_PARAM)
    onShaderParamChange(pcb_id, panel);
  else if (pcb_id == PID_SHADER)
  {
    curMat->className = panel->getText(pcb_id);

    fillShaderPanel(panel);
  }

  updatePreview();
}


//==================================================================================================

void MaterialsPlugin::onShaderParamChange(int pid, PropPanel::ContainerPropertyControl *panel)
{
  IPoint2 paramIdx = getParamIdxFromPid(pid);

  G_ASSERT(paramIdx.x >= 0 && paramIdx.x < shaderParamsNames.size());

  const MaterialParamDescr *desc = findParameterDescr(shaderParamsNames[paramIdx.x]);
  G_ASSERT(desc);

  MatParam &param = findMatParam(desc->getName());

  switch (desc->getType())
  {
    case MaterialParamDescr::PT_TEXTURE: onTextureChange(panel, paramIdx, (MatTexture &)param); break;

    case MaterialParamDescr::PT_TRIPLE_INT: onTripleIntChange(panel, paramIdx, (MatTripleInt &)param); break;

    case MaterialParamDescr::PT_TRIPLE_REAL: onTripleRealChange(panel, paramIdx, (MatTripleReal &)param); break;

    case MaterialParamDescr::PT_E3DCOLOR: onE3dColorChange(panel, paramIdx, (MatE3dColor &)param); break;

    case MaterialParamDescr::PT_COMBO: onComboChange(panel, paramIdx, (MatCombo &)param); break;

    case MaterialParamDescr::PT_CUSTOM: onCustomChange(panel, paramIdx, (MatCustom &)param); break;

    case MaterialParamDescr::PT_UNKNOWN: break; // to prevent the unhandled switch case error
  }

  param.setScript(*curMat);
}

//==================================================================================================

void MaterialsPlugin::onTripleIntChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx,
  MatTripleInt &param) const
{
  const int checkPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;
  const int trackPid = checkPid + 1;

  param.useDef = panel->getBool(checkPid);
  param.val = panel->getInt(trackPid);

  panel->setEnabledById(trackPid, !param.useDef);
}


//==================================================================================================

void MaterialsPlugin::onTripleRealChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx,
  MatTripleReal &param) const
{
  const int checkPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;
  const int realPid = checkPid + 1;

  param.useDef = panel->getBool(checkPid);
  param.val = panel->getFloat(realPid);

  panel->setEnabledById(realPid, !param.useDef);
}

//==================================================================================================

void MaterialsPlugin::onE3dColorChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatE3dColor &param) const
{
  const int checkPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;
  const int realPid = checkPid + 1;

  param.useDef = panel->getBool(checkPid);
  param.val = panel->getColor(realPid);

  panel->setEnabledById(realPid, !param.useDef);
}

//==================================================================================================

void MaterialsPlugin::onComboChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatCombo &param) const
{
  const int comboPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;
  param.val = panel->getText(comboPid);
}

//==================================================================================================

void MaterialsPlugin::onTextureChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatTexture &param) const
{
  int type = curAsset->getMgr().getTexAssetTypeId();

  SelectAssetDlg dlg(0, &curAsset->getMgr(), "Select texture", "Select texture", "Reset texture", make_span_const(&type, 1));
  dlg.selectObj(curMat->getTexRef(param.getSlot()));
  dlg.setManualModalSizingEnabled();
  if (!dlg.hasEverBeenShown())
    dlg.positionLeftToWindow("Properties", true);

  int ret = dlg.showDialog();
  if (ret == PropPanel::DIALOG_ID_CLOSE)
    return;

  if (ret == PropPanel::DIALOG_ID_OK)
    param.texAsset = dlg.getSelObjName();
  else
    param.texAsset = "";

  if (param.getSlot() >= curMat->textures.size())
  {
    int start = curMat->textures.size();
    curMat->textures.resize(param.getSlot() + 1);

    for (int i = start; i < curMat->textures.size(); ++i)
      curMat->textures[i] = NULL;
  }

  curMat->textures[param.getSlot()] = curAsset->getMgr().findAsset(param.texAsset, type);
  const int btnPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;
  panel->setCaption(btnPid, param.texAsset.str());
}

//==================================================================================================

void MaterialsPlugin::onCustomChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatCustom &param) const
{
  switch (param.getCustomType())
  {
    case MatCustom::CUSTOM_2SIDED: onTwoSidedChange(panel, param_idx, (Mat2Sided &)param); break;
  }
}

//==================================================================================================

void MaterialsPlugin::onTwoSidedChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, Mat2Sided &param) const
{
  const int comboPid = PID_FIRST_PARAM + param_idx.x * PID_PARAM_PIDS_COUNT;

  int val = panel->getInt(comboPid);

  switch (val)
  {
    case 0: param.val = Mat2Sided::TWOSIDED_NONE; break;

    case 1: param.val = Mat2Sided::TWOSIDED; break;

    case 2: param.val = Mat2Sided::TWOSIDED_REAL; break;
  }
}

//==================================================================================================

void MaterialsPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id >= PID_FIRST_PARAM)
    onShaderParamChange(pcb_id, panel);

  updatePreview();
}

//==================================================================================================

real MaterialsPlugin::getCorrectRealVal(real val, const Point2 &constrains) const
{
  if (val < constrains.x)
    val = constrains.x;
  else if (val > constrains.y)
    val = constrains.y;

  return val;
}
