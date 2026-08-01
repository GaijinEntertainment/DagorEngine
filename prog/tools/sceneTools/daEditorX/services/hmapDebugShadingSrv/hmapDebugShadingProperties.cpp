// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmapDebugShadingProperties.h"

#include <de3_hmapDebugShadingService.h>

#include <EditorCore/ec_interface.h>
#include <ioSys/dag_fileIo.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_files.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <time.h>

namespace
{

static constexpr int MAXIMUM_GRADIENT_KEY_COUNT = 64;

enum
{
  LPID_GRADIENT_LAND_GROUP,
  LPID_GRADIENT_LAND_SHOW,
  LPID_GRADIENT_LAND_GRADIENT_SELECTOR,
  LPID_GRADIENT_LAND_GRADIENT_CREATE,
  LPID_GRADIENT_LAND_GRADIENT_CLONE,
  LPID_GRADIENT_LAND_GRADIENT_DELETE,
  LPID_GRADIENT_LAND_GRADIENT,
  LPID_GRADIENT_LAND_ANCHOR_EXTENSIBLE_GROUP,

  LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START,
  LPID_GRADIENT_LAND_GRADIENT_KEY_EXTENSIBLE_GROUP = 0,
  LPID_GRADIENT_LAND_GRADIENT_KEY_COLOR,
  LPID_GRADIENT_LAND_GRADIENT_KEY_HEIGHT,
  LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT,
  LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_END =
    LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START + (LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT * MAXIMUM_GRADIENT_KEY_COUNT),

  LPID_TOTAL_USED_IDS,
};

} // namespace

G_STATIC_ASSERT(IHmapDebugShadingService::REQUIRED_PROPERTY_IDS == LPID_TOTAL_USED_IDS);

eastl::optional<int64_t> HeightmapDebugShadingProperties::getLastModificationTime(const char *path)
{
  DagorStat stat{};
  if (df_stat(path, &stat) == -1)
    return {};
  else
    return stat.mtime;
}

void HeightmapDebugShadingProperties::loadGradientsFile(const char *path)
{
  FullFileLoadCB crd(path, DF_READ | DF_IGNORE_MISSING);
  if (!crd.fileHandle || !lastLoadedColorGradientsBlk.loadFromStream(crd, path, crd.getTargetDataSize()))
    lastLoadedColorGradientsBlk.reset();

  colorGradients.load(lastLoadedColorGradientsBlk);
  lastLoadedColorGradientsBlkModificationTime = getLastModificationTime(path);
}

const char *HeightmapDebugShadingProperties::getOverwriteProtectedSavePath(const char *path) const
{
  static String savePath;

  // One level of overwrite protection is enough. (Do not check if the timestamped save path has been modified from the outside.)
  if (!savePath.empty())
    return savePath;

  if (lastLoadedColorGradientsBlkModificationTime.has_value())
  {
    const eastl::optional<int64_t> currentModificationTime = getLastModificationTime(path);
    if (currentModificationTime.has_value() && currentModificationTime.value() != lastLoadedColorGradientsBlkModificationTime.value())
    {
      time_t epochTime;
      time(&epochTime);
      const tm *t = localtime(&epochTime);
      savePath.printf(0, "%s.%04d-%02d-%02d %02d.%02d.%02d.blk", path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
        t->tm_min, t->tm_sec);

      logerr("The heightmap color gradients file \"%s\" has been modified from the outside. Saving new changes to \"%s\". The file "
             "will not be automatically loaded.",
        path, savePath);

      return savePath;
    }
  }

  return path;
}

void HeightmapDebugShadingProperties::saveGradientsFile(const char *path)
{
  DataBlock newColorGradientsBlk;
  colorGradients.save(newColorGradientsBlk);
  if (newColorGradientsBlk == lastLoadedColorGradientsBlk)
    return;

  path = getOverwriteProtectedSavePath(path);
  if (newColorGradientsBlk.saveToTextFile(path))
  {
    logdbg("Saved heightmap color gradients to \"%s\".", path);
    lastLoadedColorGradientsBlk = newColorGradientsBlk;
    lastLoadedColorGradientsBlkModificationTime = getLastModificationTime(path);
  }
  else
  {
    logerr("Error saving heightmap color gradients to \"%s\".", path);
  }
}

void HeightmapDebugShadingProperties::loadSettings(const DataBlock &blk) { selectedGradientName = blk.getStr("gradient", ""); }

void HeightmapDebugShadingProperties::saveSettings(DataBlock &blk) const { blk.addStr("gradient", selectedGradientName); }

void HeightmapDebugShadingProperties::updateGradientSelectorButtons(PropPanel::ContainerPropertyControl &panel, int pid_start,
  const HeightmapColorGradient *gradient)
{
  panel.setEnabledById(pid_start + LPID_GRADIENT_LAND_GRADIENT_CLONE, gradient != nullptr);
  panel.setEnabledById(pid_start + LPID_GRADIENT_LAND_GRADIENT_DELETE, gradient != nullptr);
}

void HeightmapDebugShadingProperties::updateGradientSelector(PropPanel::ContainerPropertyControl &panel, int pid_start,
  const HeightmapColorGradient *gradient)
{
  Tab<String> gradientNames(tmpmem);
  for (const HeightmapColorGradient &gradient : colorGradients.gradients)
    gradientNames.push_back(gradient.name);

  panel.setStrings(pid_start + LPID_GRADIENT_LAND_GRADIENT_SELECTOR, gradientNames);
  panel.setText(pid_start + LPID_GRADIENT_LAND_GRADIENT_SELECTOR, gradient ? gradient->name : "");

  updateGradientSelectorButtons(panel, pid_start, gradient);
}

void HeightmapDebugShadingProperties::updateGradientControl(PropPanel::ContainerPropertyControl &panel, int pid_start,
  const HeightmapColorGradient *gradient)
{
  panel.setEnabledById(pid_start + LPID_GRADIENT_LAND_GRADIENT, gradient != nullptr);

  if (!gradient)
  {
    panel.resetById(pid_start + LPID_GRADIENT_LAND_GRADIENT);
    return;
  }

  const float minHeight = gradient->keys.empty() ? 0.0f : gradient->keys.front().height;
  const float maxHeight = gradient->keys.empty() ? 0.0f : gradient->keys.back().height;
  panel.setMinMaxStep(pid_start + LPID_GRADIENT_LAND_GRADIENT, minHeight, maxHeight, 1.0f);

  PropPanel::Gradient panelGradient;
  for (int keyIndex = 0; keyIndex < gradient->keys.size(); ++keyIndex)
  {
    const HeightmapColorGradient::Key &gradientKey = gradient->keys[keyIndex];
    panelGradient.emplace_back(gradientKey.height, gradientKey.color);
  }
  panel.setGradient(pid_start + LPID_GRADIENT_LAND_GRADIENT, &panelGradient);
}

void HeightmapDebugShadingProperties::updateGradientKeyControlLimits(PropPanel::ContainerPropertyControl &panel, int pid_start,
  const HeightmapColorGradient *gradient)
{
  if (!gradient)
    return;

  int keyCountToShow = gradient->keys.size();
  if (keyCountToShow > MAXIMUM_GRADIENT_KEY_COUNT)
  {
    logerr("Gradient \"%s\" has more than %d anchors, but only %d is suported.", gradient->name, keyCountToShow,
      MAXIMUM_GRADIENT_KEY_COUNT);
    keyCountToShow = MAXIMUM_GRADIENT_KEY_COUNT;
  }

  for (int keyIndex = 0; keyIndex < keyCountToShow; ++keyIndex)
  {
    const int keyBasePid =
      LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START + (keyIndex * LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT);
    const float lowerLimit = keyIndex > 0 ? gradient->keys[keyIndex - 1].height : -10000.0f;
    const float upperLimit = (keyIndex + 1) < gradient->keys.size() ? gradient->keys[keyIndex + 1].height : 10000.0f;
    panel.setMinMaxStep(pid_start + keyBasePid + LPID_GRADIENT_LAND_GRADIENT_KEY_HEIGHT, lowerLimit, upperLimit, 1.0f);
  }
}

void HeightmapDebugShadingProperties::updateGradientKeyControls(PropPanel::ContainerPropertyControl &panel, int pid_start,
  const HeightmapColorGradient *gradient)
{
  PropPanel::ContainerPropertyControl *gradientLandGroup = panel.getContainerById(pid_start + LPID_GRADIENT_LAND_GROUP);
  G_ASSERT(gradientLandGroup);

  for (int keyIndex = 0; keyIndex < MAXIMUM_GRADIENT_KEY_COUNT; ++keyIndex)
  {
    const int keyBasePid =
      LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START + (keyIndex * LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT);
    gradientLandGroup->removeById(pid_start + keyBasePid + LPID_GRADIENT_LAND_GRADIENT_KEY_EXTENSIBLE_GROUP);
  }

  if (gradient && gradient->keys.size() < MAXIMUM_GRADIENT_KEY_COUNT)
    gradientLandGroup->setInt(pid_start + LPID_GRADIENT_LAND_ANCHOR_EXTENSIBLE_GROUP, 1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);
  else
    gradientLandGroup->setInt(pid_start + LPID_GRADIENT_LAND_ANCHOR_EXTENSIBLE_GROUP, 0);

  if (!gradient)
    return;

  int keyCountToShow = gradient->keys.size();
  if (keyCountToShow > MAXIMUM_GRADIENT_KEY_COUNT)
  {
    logerr("Gradient \"%s\" has more than %d anchors, but only %d is suported.", gradient->name, keyCountToShow,
      MAXIMUM_GRADIENT_KEY_COUNT);
    keyCountToShow = MAXIMUM_GRADIENT_KEY_COUNT;
  }

  String tempString;
  for (int keyIndex = 0; keyIndex < keyCountToShow; ++keyIndex)
  {
    const int keyBasePid =
      LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START + (keyIndex * LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT);
    const HeightmapColorGradient::Key &key = gradient->keys[keyIndex];

    const bool canRemoveAnchor = gradient->keys.size() > 2;
    const char *removeAnchorTooltip =
      canRemoveAnchor ? "Remove anchor"
                      : "Remove anchor\n\nBoundary anchors are required and cannot be\nremoved, but can be adjusted.";

    PropPanel::ContainerPropertyControl *keyExtensibleGroup = gradientLandGroup->createExtensible(
      pid_start + keyBasePid + LPID_GRADIENT_LAND_GRADIENT_KEY_EXTENSIBLE_GROUP, true, "delete", removeAnchorTooltip);
    if (canRemoveAnchor)
      keyExtensibleGroup->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);

    keyExtensibleGroup->setUseFixedWidthColumns();

    // This is only needed because ExtensiblePropertyControl sets ImGuiStyleVar_CellPadding to 0.
    // Ideally we would use ImGui::GetStyle().ItemInnerSpacing.x, but because this method sets the padding, it would be twice as big.
    keyExtensibleGroup->setHorizontalSpaceBetweenControls(hdpi::_pxScaled(1));

    keyExtensibleGroup->createSimpleColor(pid_start + keyBasePid + LPID_GRADIENT_LAND_GRADIENT_KEY_COLOR, "", key.color, true, true,
      PropPanel::Constants::SIMPLE_COLOR_BUTTON_FRAME_SIZE);

    keyExtensibleGroup->createEditFloat(pid_start + keyBasePid + LPID_GRADIENT_LAND_GRADIENT_KEY_HEIGHT, "", key.height, 2, true,
      false);
  }

  updateGradientKeyControlLimits(panel, pid_start, gradient);
}

void HeightmapDebugShadingProperties::fillPropertyPanel(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  PropPanel::ContainerPropertyControl *gradientLandGroup = panel.createGroup(pid_start + LPID_GRADIENT_LAND_GROUP, "Gradient land");
  gradientLandGroup->createCheckBox(pid_start + LPID_GRADIENT_LAND_SHOW, "Show gradient land", showGradientColors);

  gradientLandGroup->createCombo(pid_start + LPID_GRADIENT_LAND_GRADIENT_SELECTOR, "", Tab<String>(), -1);
  gradientLandGroup->createButton(pid_start + LPID_GRADIENT_LAND_GRADIENT_CREATE, "Create");
  gradientLandGroup->createButton(pid_start + LPID_GRADIENT_LAND_GRADIENT_CLONE, "Clone", true, false);
  gradientLandGroup->createButton(pid_start + LPID_GRADIENT_LAND_GRADIENT_DELETE, "Delete", true, false);

  gradientLandGroup->createGradientBox(pid_start + LPID_GRADIENT_LAND_GRADIENT, "");

  PropPanel::ContainerPropertyControl *gradientExtensibleGroup =
    gradientLandGroup->createExtensible(pid_start + LPID_GRADIENT_LAND_ANCHOR_EXTENSIBLE_GROUP, true, "create_point", "Add anchor");
  gradientExtensibleGroup->createStatic(0, "Anchors (m):");

  const HeightmapColorGradient *gradient = getSelectedGradient();
  updateGradientSelector(panel, pid_start, gradient);
  updateGradientControl(panel, pid_start, gradient);
  updateGradientKeyControls(panel, pid_start, gradient);
}

void HeightmapDebugShadingProperties::onChangeGradientSelector(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  selectedGradientName = panel.getText(pid_start + LPID_GRADIENT_LAND_GRADIENT_SELECTOR);

  HeightmapColorGradient *gradient = getSelectedGradient();
  updateGradientSelectorButtons(panel, pid_start, gradient);
  updateGradientControl(panel, pid_start, gradient);
  updateGradientKeyControls(panel, pid_start, gradient);
}

void HeightmapDebugShadingProperties::onChangeGradientControl(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  HeightmapColorGradient *gradient = getSelectedGradient();
  if (!gradient)
    return;

  PropPanel::Gradient panelGradient(tmpmem);
  panel.getGradient(pid_start + LPID_GRADIENT_LAND_GRADIENT, &panelGradient);

  gradient->keys.resize(panelGradient.size());

  for (int i = 0; i < panelGradient.size(); ++i)
  {
    gradient->keys[i].color = panelGradient[i].color;
    gradient->keys[i].height = panelGradient[i].position;
  }

  updateGradientKeyControls(panel, pid_start, gradient);
}

bool HeightmapDebugShadingProperties::onPropertyPanelChange(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  if (pcb_id == (pid_start + LPID_GRADIENT_LAND_SHOW))
  {
    showGradientColors = panel.getBool(pcb_id);
  }
  else if (pcb_id == (pid_start + LPID_GRADIENT_LAND_GRADIENT_SELECTOR))
  {
    onChangeGradientSelector(panel, pid_start);
  }
  else if (pcb_id == (pid_start + LPID_GRADIENT_LAND_GRADIENT))
  {
    onChangeGradientControl(panel, pid_start);
  }
  else if (pcb_id >= (pid_start + LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START) &&
           pcb_id <= (pid_start + LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_END))
  {
    HeightmapColorGradient *gradient = getSelectedGradient();
    if (gradient)
    {
      const int parameterPid = pcb_id - pid_start - LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START;
      const int keyIndex = parameterPid / LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT;
      const int parameterIndex = parameterPid % LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT;

      if (keyIndex >= 0 && keyIndex < gradient->keys.size())
      {
        if (parameterIndex == LPID_GRADIENT_LAND_GRADIENT_KEY_COLOR)
        {
          gradient->keys[keyIndex].color = panel.getColor(pcb_id);
          updateGradientControl(panel, pid_start, gradient);
        }
        else if (parameterIndex == LPID_GRADIENT_LAND_GRADIENT_KEY_HEIGHT)
        {
          gradient->keys[keyIndex].height = panel.getFloat(pcb_id);
          updateGradientControl(panel, pid_start, gradient);
          updateGradientKeyControlLimits(panel, pid_start, gradient);
        }
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}

String HeightmapDebugShadingProperties::showNewGradientNamePicker(const char *dialog_title) const
{
  enum
  {
    PID_GRADIENT_NAME = PropPanel::DIALOG_ID_FIRST_FREE,
  };

  eastl::unique_ptr<PropPanel::DialogWindow> dialog(
    EDITORCORE->createDialog(hdpi::_pxScaled(250), hdpi::_pxScaled(125), dialog_title));
  dialog->setInitialFocus(PID_GRADIENT_NAME);
  PropPanel::ContainerPropertyControl *dlgPanel = dialog->getPanel();
  dlgPanel->createEditBox(PID_GRADIENT_NAME, "Name");

  while (true)
  {
    const int ret = dialog->showDialog();
    if (ret != PropPanel::DIALOG_ID_OK)
      return String();

    String name(dlgPanel->getText(PID_GRADIENT_NAME));
    trim(name);
    if (name.empty())
      continue;

    if (!colorGradients.getByName(name))
      return name;

    wingw::message_box(wingw::MBS_EXCL, "Error", "A gradient with this name already exists.");
  }
}

void HeightmapDebugShadingProperties::onClickCreate(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  const String name = showNewGradientNamePicker("Create new gradient");
  if (name.empty())
    return;

  HeightmapColorGradient &newGradient = colorGradients.gradients.push_back(HeightmapColorGradient());
  newGradient.name = name;
  newGradient.keys.push_back();
  newGradient.keys.back().height = 0.0f;
  newGradient.keys.back().color = E3DCOLOR(0, 0, 0, 255);
  newGradient.keys.push_back();
  newGradient.keys.back().height = 1000.0f;
  newGradient.keys.back().color = E3DCOLOR(255, 255, 255, 255);

  colorGradients.sortByName();

  selectedGradientName = name;
  updateGradientSelector(panel, pid_start, getSelectedGradient());
  onChangeGradientSelector(panel, pid_start);
}

void HeightmapDebugShadingProperties::onClickClone(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  const HeightmapColorGradient *gradient = getSelectedGradient();
  if (!gradient)
    return;

  const String name = showNewGradientNamePicker("Clone the selected gradient as");
  if (name.empty())
    return;

  HeightmapColorGradient &newGradient = colorGradients.gradients.push_back(HeightmapColorGradient(*gradient));
  newGradient.name = name;

  colorGradients.sortByName();

  selectedGradientName = name;
  updateGradientSelector(panel, pid_start, getSelectedGradient());
  onChangeGradientSelector(panel, pid_start);
}

void HeightmapDebugShadingProperties::onClickDelete(PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  const int gradientIndex = colorGradients.getIndexByName(selectedGradientName);
  if (gradientIndex < 0)
    return;

  const String message(0, "Delete the \"%s\" gradient?", selectedGradientName);
  const int dialogResult = wingw::message_box(wingw::MBS_EXCL | wingw::MBS_YESNO, "Delete gradient?", message);
  if (dialogResult != wingw::MB_ID_YES)
    return;

  colorGradients.gradients.erase(colorGradients.gradients.begin() + gradientIndex);

  if (colorGradients.gradients.empty())
    selectedGradientName.clear();
  else if (gradientIndex < colorGradients.gradients.size())
    selectedGradientName = colorGradients.gradients[gradientIndex].name;
  else
    selectedGradientName = colorGradients.gradients.back().name;

  updateGradientSelector(panel, pid_start, getSelectedGradient());
  onChangeGradientSelector(panel, pid_start);
}

bool HeightmapDebugShadingProperties::onPropertyPanelClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start)
{
  if (pcb_id == (pid_start + LPID_GRADIENT_LAND_GRADIENT_CREATE))
  {
    onClickCreate(panel, pid_start);
  }
  else if (pcb_id == (pid_start + LPID_GRADIENT_LAND_GRADIENT_CLONE))
  {
    onClickClone(panel, pid_start);
  }
  else if (pcb_id == (pid_start + LPID_GRADIENT_LAND_GRADIENT_DELETE))
  {
    onClickDelete(panel, pid_start);
  }
  else if (pcb_id == (pid_start + LPID_GRADIENT_LAND_ANCHOR_EXTENSIBLE_GROUP))
  {
    HeightmapColorGradient *gradient = getSelectedGradient();
    if (gradient)
    {
      HeightmapColorGradient::Key key;
      if (gradient->keys.size() > 0)
      {
        const int lastIndex = gradient->keys.size() - 1;

        key = gradient->keys[lastIndex];
        if (gradient->keys.size() > 1)
          key.height += abs(gradient->keys[lastIndex].height - gradient->keys[lastIndex - 1].height);
        else
          key.height += 10;
      }

      gradient->keys.push_back(key);
      updateGradientControl(panel, pid_start, gradient);
      updateGradientKeyControls(panel, pid_start, gradient);
    }
  }
  else if (pcb_id >= (pid_start + LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START) &&
           pcb_id <= (pid_start + LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_END))
  {
    HeightmapColorGradient *gradient = getSelectedGradient();
    if (gradient)
    {
      const int parameterPid = pcb_id - pid_start - LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_START;
      const int keyIndex = parameterPid / LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT;
      const int parameterIndex = parameterPid % LPID_GRADIENT_LAND_GRADIENT_KEY_PARAMETER_COUNT;

      if (keyIndex >= 0 && keyIndex < gradient->keys.size())
      {
        if (parameterIndex == LPID_GRADIENT_LAND_GRADIENT_KEY_EXTENSIBLE_GROUP)
        {
          gradient->keys.erase(gradient->keys.begin() + keyIndex);
          updateGradientControl(panel, pid_start, gradient);
          updateGradientKeyControls(panel, pid_start, gradient);
        }
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}

HeightmapColorGradient *HeightmapDebugShadingProperties::getSelectedGradient()
{
  return colorGradients.getByName(selectedGradientName);
}

const HeightmapColorGradient *HeightmapDebugShadingProperties::getSelectedGradient() const
{
  return colorGradients.getByName(selectedGradientName);
}
