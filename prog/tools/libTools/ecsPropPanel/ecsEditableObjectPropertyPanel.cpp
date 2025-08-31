// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecsPropPanel/ecsEditableObjectPropertyPanel.h>
#include <ecsPropPanel/ecsBasicObjectEditor.h>
#include <ecsPropPanel/ecsTemplateSelectorDialog.h>
#include "ecsEditorAddObjectMemberDlg.h"
#include "ecsEditorRenameObjectMemberDlg.h"
#include "ecsPropertyPanelMapping.h"

#include <ecs/core/entityManager.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <EditorCore/ec_cm.h>
#include <propPanel/control/container.h>

enum
{
  PID_TEMPLATE_LIST = 1,
  PID_TEMPLATE_ADD,
  PID_TEMPLATE_REMOVE,
  PID_COMPONENT_FIRST,
};

static ECSPropertyPanelMapping ecs_property_panel_mapping;

template <typename T>
static void handle_addable_array_and_object_types(unsigned member_type, T callback)
{
  switch (member_type)
  {
    case ecs::ComponentTypeInfo<bool>::type: callback(ZERO<bool>()); break;
    case ecs::ComponentTypeInfo<int>::type: callback(ZERO<int>()); break;
    case ecs::ComponentTypeInfo<float>::type: callback(ZERO<float>()); break;
    case ecs::ComponentTypeInfo<Point2>::type: callback(ZERO<Point2>()); break;
    case ecs::ComponentTypeInfo<Point3>::type: callback(ZERO<Point3>()); break;
    case ecs::ComponentTypeInfo<Point4>::type: callback(ZERO<Point4>()); break;
    case ecs::ComponentTypeInfo<E3DCOLOR>::type: callback(E3DCOLOR(0, 0, 0)); break;
    case ecs::ComponentTypeInfo<ecs::string>::type: callback(ecs::string()); break;
    case ecs::ComponentTypeInfo<ecs::EntityId>::type: callback(ecs::INVALID_ENTITY_ID); break;
    case ecs::ComponentTypeInfo<ecs::Array>::type: callback(ecs::Array()); break;
    case ecs::ComponentTypeInfo<ecs::Object>::type: callback(ecs::Object()); break;
    default: G_ASSERT(false); break;
  }
}

ECSEditableObjectPropertyPanel::ECSEditableObjectPropertyPanel(PropPanel::ContainerPropertyControl &main_panel) : mainPanel(main_panel)
{}

void ECSEditableObjectPropertyPanel::fillComponentPropertyPanel(PropPanel::ContainerPropertyControl &panel, int &control_id,
  const ecs::EntityComponentRef &component, const char *name, int parent_control_id)
{
  const auto compData = g_entity_mgr->getDataComponents().getComponentById(component.getComponentId());
  if ((compData.flags & ecs::DataComponent::IS_COPY) != 0)
    return;

  ecs::component_type_t ctype = component.getUserType();
  switch (ctype)
  {
    case ecs::ComponentTypeInfo<bool>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createCheckBox(control_id, name, component.get<bool>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<int>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createEditInt(control_id, name, component.get<int>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<float>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createEditFloat(control_id, name, component.get<float>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<Point2>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createPoint2(control_id, name, component.get<Point2>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<Point3>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createPoint3(control_id, name, component.get<Point3>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<Point4>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createPoint4(control_id, name, component.get<Point4>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<E3DCOLOR>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createColorBox(control_id, name, component.get<E3DCOLOR>());
      ++control_id;
      break;

      // case ecs::ComponentTypeInfo<TMatrix>::type:
      //   panel.createMatrix(control_id, name, component.get<TMatrix>());
      // break;

    case ecs::ComponentTypeInfo<ecs::string>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createEditBox(control_id, name, component.get<ecs::string>().c_str());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<ecs::EntityId>::type:
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id);
      panel.createEditInt(control_id, name, (unsigned)component.get<ecs::EntityId>());
      ++control_id;
      break;

    case ecs::ComponentTypeInfo<ecs::Object>::type:
    {
      const int groupControlId = control_id;
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id,
        ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);
      PropPanel::ContainerPropertyControl *objectGroupPanel = panel.createGroup(control_id, name);
      ++control_id;

      for (const eastl::pair<eastl::string, ecs::ChildComponent> &child : component.get<ecs::Object>())
      {
        const int extensibleContainerControlId = control_id;
        ecs_property_panel_mapping.addControl(extensibleContainerControlId, ecs::get_key_string(child.first).c_str(), groupControlId,
          ECSEditorPropertyInfo::PropertyType::ComponentObjectExtensibleContainer);
        PropPanel::ContainerPropertyControl *objectPropertyPanel = objectGroupPanel->createExtensible(extensibleContainerControlId);
        objectPropertyPanel->setIntValue((1 << PropPanel::EXT_BUTTON_REMOVE) | (1 << PropPanel::EXT_BUTTON_RENAME));
        ++control_id;

        fillComponentPropertyPanel(*objectPropertyPanel, control_id, child.second.getEntityComponentRef(),
          ecs::get_key_string(child.first).c_str(), extensibleContainerControlId);
      }

      ecs_property_panel_mapping.addControl(control_id, nullptr, groupControlId,
        ECSEditorPropertyInfo::PropertyType::ComponentObjectAddButton);
      objectGroupPanel->createButton(control_id, "Add");
      ++control_id;
    }
    break;

    case ecs::ComponentTypeInfo<ecs::Array>::type:
    {
      const int groupControlId = control_id;
      ecs_property_panel_mapping.addControl(control_id, name, parent_control_id,
        ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup);
      PropPanel::ContainerPropertyControl *objectGroupPanel = panel.createGroup(control_id, name);
      ++control_id;

      String arrayElementName;
      int arrayIndex = 0;
      for (const ecs::ChildComponent &child : component.get<ecs::Array>())
      {
        const int extensibleContainerControlId = control_id;
        ecs_property_panel_mapping.addControl(extensibleContainerControlId, nullptr, groupControlId,
          ECSEditorPropertyInfo::PropertyType::ComponentArrayExtensibleContainer, arrayIndex);
        PropPanel::ContainerPropertyControl *objectPropertyPanel = objectGroupPanel->createExtensible(extensibleContainerControlId);
        objectPropertyPanel->setIntValue(1 << PropPanel::EXT_BUTTON_REMOVE);
        ++control_id;

        arrayElementName.printf(0, "Element[%d]", arrayIndex);
        fillComponentPropertyPanel(*objectPropertyPanel, control_id, child.getEntityComponentRef(), arrayElementName,
          extensibleContainerControlId);
        ++arrayIndex;
      }

      ecs_property_panel_mapping.addControl(control_id, nullptr, groupControlId,
        ECSEditorPropertyInfo::PropertyType::ComponentArrayAddButton);
      objectGroupPanel->createButton(control_id, "Add");
      ++control_id;
    }
    break;

    default:
      String title(0, "%s (%s)", name, g_entity_mgr->getComponentTypes().findTypeName(component.getUserType()));
      panel.createStatic(control_id, title);
      ++control_id;
      break;
  }
}

void ECSEditableObjectPropertyPanel::fillProps(ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed)
{
  ecs_property_panel_mapping.clear();

  if (editable_object_eid == ecs::INVALID_ENTITY_ID)
    return;

  // Display transform first. It is also handled differently by ObjectEditor.
  if (g_entity_mgr->has(editable_object_eid, ECS_HASH("transform")))
  {
    mainPanel.createGroup(RenderableEditableObject::PID_TRANSFORM_GROUP, "transform");
    editable_object_objed->createPanelTransform(CM_OBJED_MODE_MOVE);
    editable_object_objed->createPanelTransform(CM_OBJED_MODE_ROTATE);
    editable_object_objed->createPanelTransform(CM_OBJED_MODE_SCALE);
  }

  int controlId = PID_COMPONENT_FIRST;
  for (ecs::ComponentsIterator it = g_entity_mgr->getComponentsIterator(editable_object_eid); it; ++it)
  {
    const auto &comp = *it;

    if (comp.second.getUserType() == ecs::ComponentTypeInfo<TMatrix>::type && strcmp(comp.first, "transform") == 0)
      continue;

    fillComponentPropertyPanel(mainPanel, controlId, comp.second, comp.first, 0);
  }

  const char *templateName = g_entity_mgr->getEntityTemplateName(editable_object_eid);
  const char *templateFutureName = g_entity_mgr->getEntityFutureTemplateName(editable_object_eid);
  if (templateFutureName && strcmp(templateName, templateFutureName) != 0)
    templateName = templateFutureName;

  Tab<String> values;
  for_each_sub_template_name(templateName, [&values](const char *sub_template) { values.push_back(String(sub_template)); });

  mainPanel.createSeparator();
  mainPanel.createList(PID_TEMPLATE_LIST, "Templates", values, -1);
  mainPanel.createButton(PID_TEMPLATE_ADD, "Add");
  mainPanel.createButton(PID_TEMPLATE_REMOVE, "Remove", true, false);
  updateTemplateRemoveButton();
}

void ECSEditableObjectPropertyPanel::setComponentValueByPropertyPanel(int control_id, ecs::EntityComponentRef &component)
{
  const auto compData = g_entity_mgr->getDataComponents().getComponentById(component.getComponentId());
  if ((compData.flags & ecs::DataComponent::IS_COPY) != 0)
    return;

  ecs::component_type_t ctype = component.getUserType();
  switch (ctype)
  {
    case ecs::ComponentTypeInfo<bool>::type: component.getRW<bool>() = mainPanel.getBool(control_id); break;
    case ecs::ComponentTypeInfo<int>::type: component.getRW<int>() = mainPanel.getInt(control_id); break;
    case ecs::ComponentTypeInfo<float>::type: component.getRW<float>() = mainPanel.getFloat(control_id); break;
    case ecs::ComponentTypeInfo<Point2>::type: component.getRW<Point2>() = mainPanel.getPoint2(control_id); break;
    case ecs::ComponentTypeInfo<Point3>::type: component.getRW<Point3>() = mainPanel.getPoint3(control_id); break;
    case ecs::ComponentTypeInfo<Point4>::type: component.getRW<Point4>() = mainPanel.getPoint4(control_id); break;

    case ecs::ComponentTypeInfo<E3DCOLOR>::type:
      component.getRW<E3DCOLOR>() = mainPanel.getColor(control_id);
      break;

      // case ecs::ComponentTypeInfo<TMatrix>::type:
      //   component.getRW<TMatrix>() = panel.getMatrix(control_id);;
      // break;

    case ecs::ComponentTypeInfo<ecs::string>::type: component.getRW<ecs::string>() = mainPanel.getText(control_id); break;

    case ecs::ComponentTypeInfo<ecs::EntityId>::type:
      component.getRW<ecs::EntityId>() = ecs::EntityId(mainPanel.getInt(control_id));
      break;

    default: G_ASSERT(false); break;
  }
}

void ECSEditableObjectPropertyPanel::onPPChange(int pid, bool edit_finished, ecs::EntityId editable_object_eid,
  ECSBasicObjectEditor *editable_object_objed)
{
  if (pid == PID_TEMPLATE_LIST)
  {
    updateTemplateRemoveButton();
    return;
  }

  const char *rootComponentName = nullptr;
  ecs::EntityComponentRef componentRef = ecs_property_panel_mapping.getComponentRef(editable_object_eid, pid, rootComponentName);
  if (componentRef.isNull())
    return;

  setComponentValueByPropertyPanel(pid, componentRef);
  editable_object_objed->saveComponent(editable_object_eid, rootComponentName);
}

void ECSEditableObjectPropertyPanel::onAddTemplatePressed(ecs::EntityId editable_object_eid,
  ECSBasicObjectEditor *editable_object_objed)
{
  ECSTemplateSelectorDialog dialog("Template selector", editable_object_objed);
  dialog.setManualModalSizingEnabled();
  if (dialog.showDialog() != PropPanel::DIALOG_ID_OK)
    return;

  const char *templateToAdd = dialog.getSelectedTemplate();
  if (templateToAdd == nullptr || *templateToAdd == 0)
    return;

  const char *oldTemplate = g_entity_mgr->getEntityTemplateName(editable_object_eid);
  if (!oldTemplate)
    return;

  const auto newTemplate = add_sub_template_name(oldTemplate, templateToAdd);
  if (strcmp(newTemplate.c_str(), oldTemplate) == 0)
    return;

  g_entity_mgr->reCreateEntityFromAsync(editable_object_eid, newTemplate.c_str());
  editable_object_objed->saveAddTemplate(editable_object_eid, templateToAdd);
  editable_object_objed->invalidateObjectProps();
}

void ECSEditableObjectPropertyPanel::onRemoveTemplatePressed(ecs::EntityId editable_object_eid,
  ECSBasicObjectEditor *editable_object_objed)
{
  dag::ConstSpan<String> templateNames = mainPanel.getStrings(PID_TEMPLATE_LIST);
  const int selectionIndex = mainPanel.getInt(PID_TEMPLATE_LIST);
  if (selectionIndex < 0)
    return;

  const char *oldTemplate = g_entity_mgr->getEntityTemplateName(editable_object_eid);
  if (!oldTemplate)
    return;

  const char *templateToRemove = templateNames[selectionIndex];
  const auto newTemplate = remove_sub_template_name(oldTemplate, templateToRemove);
  if (strcmp(newTemplate.c_str(), oldTemplate) == 0)
    return;

  g_entity_mgr->reCreateEntityFromAsync(editable_object_eid, newTemplate.c_str());
  editable_object_objed->saveDelTemplate(editable_object_eid, templateToRemove, true);
  editable_object_objed->invalidateObjectProps();
}

void ECSEditableObjectPropertyPanel::updateTemplateRemoveButton()
{
  dag::ConstSpan<String> templateNames = mainPanel.getStrings(PID_TEMPLATE_LIST);
  const int selectionIndex = mainPanel.getInt(PID_TEMPLATE_LIST);

  if (selectionIndex > 0)
  {
    const bool internalTemplate = strcmp(templateNames[selectionIndex], ECS_EDITOR_SELECTED_TEMPLATE) == 0;
    mainPanel.setEnabledById(PID_TEMPLATE_REMOVE, !internalTemplate);
    mainPanel.setTooltipId(PID_TEMPLATE_REMOVE,
      internalTemplate ? "This internal template used by the editor cannot be removed." : "");
  }
  else
  {
    mainPanel.setEnabledById(PID_TEMPLATE_REMOVE, false);
    mainPanel.setTooltipId(PID_TEMPLATE_REMOVE, selectionIndex == 0 ? "The first template cannot be removed." : "");
  }
}

bool ECSEditableObjectPropertyPanel::addComponentArrayElementPressed(ecs::Array &component_array)
{
  int memberType;
  if (!ECSEditorAddObjectMemberDlg::show(nullptr, memberType))
    return false;

  handle_addable_array_and_object_types(memberType, [&]<typename T>(const T &value) { component_array.push_back(value); });

  return true;
}

bool ECSEditableObjectPropertyPanel::removeComponentArrayElementPressed(ecs::Array &component_array, int element_index)
{
  if (element_index < 0 || element_index >= component_array.size())
    return false;

  component_array.erase(component_array.begin() + element_index);
  return true;
}

bool ECSEditableObjectPropertyPanel::addComponentObjectMemberPressed(ecs::Object &component_object)
{
  String memberName;
  int memberType;
  if (!ECSEditorAddObjectMemberDlg::show(&memberName, memberType))
    return false;

  handle_addable_array_and_object_types(memberType,
    [&]<typename T>(const T &value) { component_object.addMember(memberName, value); });

  return true;
}

bool ECSEditableObjectPropertyPanel::removeComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name)
{
  ecs::Object::const_iterator it = component_object.find(member_name);
  if (it == component_object.end())
    return false;

  component_object.erase(it);

  return true;
}

bool ECSEditableObjectPropertyPanel::renameComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name)
{
  ecs::Object::const_iterator it = component_object.find(member_name);
  if (it == component_object.end())
    return false;

  String newMemberName(member_name);
  if (!ECSEditorRenameObjectMemberDlg::show(newMemberName))
    return false;

  if (component_object.find(newMemberName) != component_object.end())
    return false;

  ecs::ChildComponent childComponent = it->second;
  component_object.erase(it);
  component_object.insert(newMemberName) = childComponent;

  return true;
}

void ECSEditableObjectPropertyPanel::onPPBtnPressed(int pid, ecs::EntityId editable_object_eid,
  ECSBasicObjectEditor *editable_object_objed)
{
  if (pid == PID_TEMPLATE_ADD)
    onAddTemplatePressed(editable_object_eid, editable_object_objed);
  else if (pid == PID_TEMPLATE_REMOVE)
    onRemoveTemplatePressed(editable_object_eid, editable_object_objed);
  else
  {
    const ECSEditorPropertyInfo *editorPropertyInfo = ecs_property_panel_mapping.getControl(pid);
    if (!editorPropertyInfo)
      return;

    bool modified = false;
    const char *rootComponentName = nullptr;

    if (ecs::Array *array = ecs_property_panel_mapping.getComponentArray(editable_object_eid, pid, rootComponentName))
    {
      if (editorPropertyInfo->propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayExtensibleContainer)
      {
        const int action = mainPanel.getInt(pid); // ExtensiblePropertyControl's getInt() can only be called once.
        if (action == PropPanel::EXT_BUTTON_REMOVE)
        {
          G_ASSERT(editorPropertyInfo->arrayIndex >= 0);
          modified = removeComponentArrayElementPressed(*array, editorPropertyInfo->arrayIndex);
        }
      }
      else if (editorPropertyInfo->propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayAddButton)
        modified = addComponentArrayElementPressed(*array);
    }
    else if (ecs::Object *object = ecs_property_panel_mapping.getComponentObject(editable_object_eid, pid, rootComponentName))
    {
      if (editorPropertyInfo->propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectExtensibleContainer)
      {
        const int action = mainPanel.getInt(pid); // ExtensiblePropertyControl's getInt() can only be called once.
        if (action == PropPanel::EXT_BUTTON_REMOVE)
          modified = removeComponentObjectMemberPressed(*object, editorPropertyInfo->componentName);
        else if (action == PropPanel::EXT_BUTTON_RENAME)
          modified = renameComponentObjectMemberPressed(*object, editorPropertyInfo->componentName);
      }
      else if (editorPropertyInfo->propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectAddButton)
        modified = addComponentObjectMemberPressed(*object);
    }

    if (modified)
    {
      editable_object_objed->saveComponent(editable_object_eid, rootComponentName);
      editable_object_objed->invalidateObjectProps();
    }
  }
}
