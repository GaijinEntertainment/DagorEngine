// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "animTreeAnimationPlayer.h"
#include <animTreeEditor/animTreeEditorHost.h>
#include <animTreeEditor/animTreeEditor.h>

#include <EditorCore/ec_interface.h>

#include <assets/asset.h>
#include <math/dag_geomTree.h>
#include <anim/dag_simpleNodeAnim.h>

using PropPanel::TLeafHandle;

enum CtrlType;
enum class AnimStatesType;

class AnimTreePlugin : public IGenEditorPlugin, private IAnimTreeEditorHost, private AnimTreeEditor
{
public:
  AnimTreePlugin();
  ~AnimTreePlugin() override;

  const char *getInternalName() const override { return "animTree"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override;
  bool end() override;

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}
  bool reloadAsset(DagorAsset *asset) override { return true; }

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override;
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects(BaseTexture *) override {}

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

protected:
  AnimTreeAnimationPlayer animPlayer;

private:
  void noteFmt(const char *fmt, const DagorSafeArg *arg, int anum) override;
  PropPanel::ContainerPropertyControl *getMainPanel() override;
  String selectOpenFile(const char *init_path, const char *init_fn) override;
  bool confirmCreateNewBlk(const char *path) override;
  void showInfoBox(const char *title, const char *text) override;
  int showYesNoCancelBox(const char *title, const char *text) override;
  void setSelectedA2dName(const char *name) override;
  void resetSelectedA2dName() override;
  void loadA2dResource(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    PropPanel::TLeafHandle leaf) override;
  void animNodesFieldChanged(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    const AnimParamData &param) override;
  void updatePlayAnimationCaption(PropPanel::ContainerPropertyControl *panel) override;
  void changePlayingAnimation() override;
  void selectDynModel(PropPanel::ContainerPropertyControl *panel) override;
};