// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animParamData.h"

#include <dag/dag_vector.h>
#include <propPanel/c_common.h>
#include <util/dag_safeArg.h>
#include <util/dag_string.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

// Services the animTree editor needs from the tool that hosts it.
class IAnimTreeEditorHost
{
public:
  virtual void noteFmt(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  // Root property panel of the host window, owner of the animTree trees.
  virtual PropPanel::ContainerPropertyControl *getMainPanel() = 0;
  // Tool dialogs: open file and question whether to create a new blk.
  // selectOpenFile: init_path and init_fn seed the dialog folder and file name.
  // Returns the picked path as #/<rel> under the project root; empty when cancelled
  // or when the pick does not contain the project root.
  virtual String selectOpenFile(const char *init_path, const char *init_fn) = 0;
  virtual bool confirmCreateNewBlk(const char *path) = 0;
  // Blend node a2d preview, played by the host dynModel.
  virtual void setSelectedA2dName(const char *name) = 0;
  virtual void resetSelectedA2dName() = 0;
  virtual void loadA2dResource(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    PropPanel::TLeafHandle leaf) = 0;
  virtual void animNodesFieldChanged(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
    const AnimParamData &param) = 0;
  // Play/stop button caption, matches the current playback state.
  virtual void updatePlayAnimationCaption(PropPanel::ContainerPropertyControl *panel) = 0;
  // DynModel selection dialog and playback toggle, backed by the host animPlayer.
  virtual void selectDynModel(PropPanel::ContainerPropertyControl *panel) = 0;
  virtual void changePlayingAnimation() = 0;
  // Tool dialogs: info and yes/no/cancel message boxes, used by dependent nodes checks.
  virtual void showInfoBox(const char *title, const char *text) = 0;
  // Returns PropPanel::DIALOG_ID_YES, DIALOG_ID_NO or DIALOG_ID_CANCEL.
  virtual int showYesNoCancelBox(const char *title, const char *text) = 0;

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void note, noteFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};
