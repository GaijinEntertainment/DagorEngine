// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <libTools/util/strUtil.h>
#include <EditorCore/ec_interface.h>

#include <EASTL/unique_ptr.h>

class ECSEditorAddObjectMemberDlg
{
public:
  // member_name: in and out parameter, if null then only the type selector will be shown
  static bool show(String *member_name, int &member_type)
  {
    enum
    {
      PID_NAME = 11000,
      PID_TYPE,
    };

    struct TypeInfo
    {
      const char *name;
      unsigned type;
    };

    const TypeInfo types[] = {
      {"Array", ecs::ComponentTypeInfo<ecs::Array>::type},
      {"bool", ecs::ComponentTypeInfo<bool>::type},
      {"E3DCOLOR", ecs::ComponentTypeInfo<E3DCOLOR>::type},
      {"EntityId", ecs::ComponentTypeInfo<ecs::EntityId>::type},
      {"float", ecs::ComponentTypeInfo<float>::type},
      {"int", ecs::ComponentTypeInfo<int>::type},
      {"Object", ecs::ComponentTypeInfo<ecs::Object>::type},
      {"Point2", ecs::ComponentTypeInfo<Point2>::type},
      {"Point3", ecs::ComponentTypeInfo<Point3>::type},
      {"Point4", ecs::ComponentTypeInfo<Point4>::type},
      {"string", ecs::ComponentTypeInfo<ecs::string>::type},
    };

    eastl::unique_ptr<PropPanel::DialogWindow> dialogWindow(
      EDITORCORE->createDialog(hdpi::_pxScaled(250), hdpi::_pxScaled(100), "Add object member"));
    dialogWindow->setInitialFocus(PropPanel::DIALOG_ID_NONE);

    PropPanel::ContainerPropertyControl &panel = *dialogWindow->getPanel();
    if (member_name)
      panel.createEditBox(PID_NAME, "Name", *member_name);

    Tab<String> values;
    for (int i = 0; i < countof(types); ++i)
      values.emplace_back(types[i].name);

    panel.createCombo(PID_TYPE, "Type", values, 0);

    panel.setFocusById(member_name ? PID_NAME : PID_NAME);
    dialogWindow->autoSize();
    if (dialogWindow->showDialog() != PropPanel::DIALOG_ID_OK)
      return false;

    if (member_name)
    {
      *member_name = panel.getText(PID_NAME);
      trim(*member_name);
      if (member_name->empty())
        return false;
    }

    const int comboSelectionIndex = panel.getInt(PID_TYPE);
    if (comboSelectionIndex < 0)
      return false;

    member_type = types[comboSelectionIndex].type;
    return true;
  }
};
