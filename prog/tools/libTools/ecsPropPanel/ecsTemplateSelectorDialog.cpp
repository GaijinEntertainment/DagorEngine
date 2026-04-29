// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecsPropPanel/ecsTemplateSelectorDialog.h>
#include <ecsPropPanel/ecsBasicObjectEditor.h>
#include <ecsPropPanel/ecsTemplateSelectorClient.h>

#include <propPanel/constants.h>

enum
{
  PID_TEMPLATE_GROUPS = 11000,
  PID_TEMPLATES_FILTER,
  PID_TEMPLATES_LIST,
  PID_SCENE_LIST,
};

ECSTemplateSelectorDialog::ECSTemplateSelectorDialog(const char caption[], ECSBasicObjectEditor *editable_object_objed) :
  DialogWindow(nullptr, hdpi::_pxScaled(250), hdpi::_pxScaled(250), caption), editor(*editable_object_objed)
{
  const dag::Vector<String> templateGroups = editor.getEcsTemplatesGroups();
  Tab<String> values;
  for (const String &group : templateGroups)
    values.push_back(group);
  getPanel()->createCombo(PID_TEMPLATE_GROUPS, "Groups", values, values.empty() ? "" : values[0].c_str());

  {
    ecs::Scene::SceneId currId = ecs::g_scenes ? ecs::g_scenes->getActiveScene().getTargetSceneId() : ecs::Scene::C_INVALID_SCENE_ID;

    Tab<String> sceneComboEntries;
    for (const auto &[name, _] : editor.getEcsScenes())
    {
      sceneComboEntries.push_back(String{name.c_str()});
    }
    const auto foundIt = eastl::find_if(editor.getEcsScenes().cbegin(), editor.getEcsScenes().cend(),
      [currId](auto &&val) { return val.second == currId; });
    getPanel()->createCombo(PID_SCENE_LIST, "Scene", sceneComboEntries,
      foundIt != editor.getEcsScenes().cend() ? foundIt->first.c_str() : sceneComboEntries[0].c_str());
  }

  getPanel()->createEditBox(PID_TEMPLATES_FILTER, "Filter");

  values.clear();
  getPanel()->createList(PID_TEMPLATES_LIST, "Templates", values, -1);
  getPanel()->getById(PID_TEMPLATES_LIST)->setHeight(PropPanel::Constants::LISTBOX_FULL_HEIGHT);
  fillTemplates();

  if (!hasEverBeenShown())
    setWindowSize(IPoint2(hdpi::_pxS(450), (int)(ImGui::GetIO().DisplaySize.y * 0.8f)));
}

void ECSTemplateSelectorDialog::updateScenes()
{
  Tab<String> sceneComboEntries;
  for (const auto &[name, _] : editor.getEcsScenes())
  {
    sceneComboEntries.push_back(String{name.c_str()});
  }
  ecs::Scene::SceneId currId = ecs::g_scenes ? ecs::g_scenes->getActiveScene().getTargetSceneId() : ecs::Scene::C_INVALID_SCENE_ID;
  const auto foundIt = eastl::find_if(editor.getEcsScenes().cbegin(), editor.getEcsScenes().cend(),
    [currId](auto &&val) { return val.second == currId; });
  getPanel()->setStrings(PID_SCENE_LIST, sceneComboEntries);
  if (foundIt != editor.getEcsScenes().cend())
  {
    getPanel()->setText(PID_SCENE_LIST, foundIt->first.c_str());
  }
}

void ECSTemplateSelectorDialog::fillTemplates()
{
  const SimpleString selectedGroup = getPanel()->getText(PID_TEMPLATE_GROUPS);

  const dag::Vector<String> templates = editor.getEcsTemplates(selectedGroup);

  Tab<String> values;
  for (const String &templateName : templates)
    if (filter.matchesFilter(templateName))
      values.push_back(templateName);

  getPanel()->setStrings(PID_TEMPLATES_LIST, values);
}

void ECSTemplateSelectorDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_TEMPLATE_GROUPS)
    fillTemplates();
  else if (pcb_id == PID_TEMPLATES_FILTER)
  {
    filter.setSearchText(panel->getText(pcb_id));
    fillTemplates();
  }
  else if (pcb_id == PID_TEMPLATES_LIST)
  {
    dag::ConstSpan<String> templateNames = panel->getStrings(pcb_id);
    const int selectionIndex = panel->getInt(pcb_id);
    if (selectionIndex >= 0)
    {
      selectedTemplate = templateNames[selectionIndex];

      if (client)
        client->onTemplateSelectorTemplateSelected(selectedTemplate);
    }
  }
  else if (pcb_id == PID_SCENE_LIST && ecs::g_scenes)
  {
    const eastl::string comboVal{panel->getText(PID_SCENE_LIST).c_str()};
    if (auto it = editor.getEcsScenes().find(comboVal); it != editor.getEcsScenes().cend())
    {
      ecs::g_scenes->getActiveScene().setTargetSceneId(it->second);
    }
    else
    {
      logerr("While setting working scene value (%s) from combobox was not found in scene list", comboVal);
      ecs::g_scenes->getActiveScene().setTargetSceneId(ecs::Scene::C_INVALID_SCENE_ID);
    }
  }
}
