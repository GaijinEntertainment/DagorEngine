// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTree.h"

#include "../av_appwnd.h"
#include <animTreeEditor/animTreePanelPids.h>

#include <assetsGui/av_selObjDlg.h>
#include <de3_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

static void update_play_animation_text(PropPanel::ContainerPropertyControl *panel, bool is_anim_in_progress)
{
  if (!panel)
    return;

  if (is_anim_in_progress)
    panel->setCaption(PID_NODES_SETTINGS_PLAY_ANIMATION, "Stop playing animation");
  else
    panel->setCaption(PID_NODES_SETTINGS_PLAY_ANIMATION, "Play animation");
}

// Caption of the PID_SELECT_DYN_MODEL button, one owner of the empty-name fallback.
static const char *dyn_model_caption(const String &model_name) { return model_name.empty() ? "Select model" : model_name.c_str(); }

AnimTreePlugin::AnimTreePlugin() :
  AnimTreeEditor(static_cast<IAnimTreeEditorHost &>(*this)),
  animPlayer{get_app().getAssetMgr().getAssetTypeId("dynModel"), get_app().getAssetMgr()}
{}

AnimTreePlugin::~AnimTreePlugin() { AnimTreePlugin::end(); }

bool AnimTreePlugin::begin(DagorAsset *asset)
{
  AnimTreeEditor::begin(*asset);
  animPlayer.init(*asset);

  return true;
}

bool AnimTreePlugin::end()
{
  animPlayer.clear();
  AnimTreeEditor::end(getPluginPanel());
  return true;
}

bool AnimTreePlugin::getSelectionBox(BBox3 &box) const { return animPlayer.getSelectionBox(box); }

void AnimTreePlugin::actObjects(float dt)
{
  const bool oldState = animPlayer.isAnimInProgress();
  animPlayer.act(dt);
  const bool curState = animPlayer.isAnimInProgress();
  if (oldState != curState)
    update_play_animation_text(getPluginPanel(), curState);
}

bool AnimTreePlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "animTree") == 0; }

void AnimTreePlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  fillMainPanel(panel, dyn_model_caption(animPlayer.getModelName()));
}

void AnimTreePlugin::selectDynModel(PropPanel::ContainerPropertyControl *panel)
{
  SelectAssetDlgOptions dlgOptions;
  dlgOptions.selectableAssetTypes.push_back(animPlayer.getDynModelId());
  dlgOptions.initiallySelectedAsset = animPlayer.getModelName();
  const char *selectCaption = "Select dynModel";
  dlgOptions.dialogCaption = selectCaption;
  dlgOptions.selectButtonCaption = selectCaption;
  dlgOptions.resetButtonCaption = "Reset dynModel";
  dlgOptions.positionBesideWindow = "Properties";

  const eastl::optional<String> result = DAEDITOR3.selectAsset(dlgOptions);
  if (!result.has_value())
    return;

  const String &modelName = result.value();
  animPlayer.setDynModel(modelName.c_str(), *curAsset);
  panel->setCaption(PID_SELECT_DYN_MODEL, dyn_model_caption(modelName));
}

void AnimTreePlugin::noteFmt(const char *fmt, const DagorSafeArg *arg, int anum) { DAEDITOR3.conNoteV(fmt, arg, anum); }

PropPanel::ContainerPropertyControl *AnimTreePlugin::getMainPanel() { return getPluginPanel(); }

String AnimTreePlugin::selectOpenFile(const char *init_path, const char *init_fn)
{
  String result =
    wingw::file_open_dlg(getMainPanel()->getRootParent()->getWindowHandle(), "Open file...", "All|*.*||", "", init_path, init_fn);
  if (result.empty())
    return result;

  // Easy way get absolute project path
  String projectPath("#");
  DataBlock::resolveIncludePath(projectPath);
  simplify_fname(projectPath);
  if (!strstr(result, projectPath))
  {
    logerr("Selected path <%s> don't contain project path <%s>. Edit field with path and filename directly", result, projectPath);
    return String();
  }
  return String(0, "#/%s", &result[projectPath.length()]);
}

bool AnimTreePlugin::confirmCreateNewBlk(const char *path)
{
  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Include change",
    "File with path: %s doesn't exist.\nDo you want to create new blk?", path);

  return dialogResult == wingw::MB_ID_YES;
}

void AnimTreePlugin::showInfoBox(const char *title, const char *text) { wingw::message_box(wingw::MBS_INFO, title, text); }

int AnimTreePlugin::showYesNoCancelBox(const char *title, const char *text)
{
  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, title, text);
  if (dialogResult == wingw::MB_ID_YES)
    return PropPanel::DIALOG_ID_YES;
  if (dialogResult == wingw::MB_ID_NO)
    return PropPanel::DIALOG_ID_NO;
  return PropPanel::DIALOG_ID_CANCEL;
}

void AnimTreePlugin::setSelectedA2dName(const char *name) { animPlayer.setSelectedA2dName(name); }

void AnimTreePlugin::resetSelectedA2dName() { animPlayer.resetSelectedA2dName(); }

void AnimTreePlugin::loadA2dResource(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
  PropPanel::TLeafHandle leaf)
{
  animPlayer.loadA2dResource(panel, params, leaf);
}

void AnimTreePlugin::animNodesFieldChanged(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
  const AnimParamData &param)
{
  animPlayer.animNodesFieldChanged(panel, params, param);
}

void AnimTreePlugin::updatePlayAnimationCaption(PropPanel::ContainerPropertyControl *panel)
{
  update_play_animation_text(panel, animPlayer.isAnimInProgress());
}

void AnimTreePlugin::changePlayingAnimation() { animPlayer.changePlayingAnimation(); }
