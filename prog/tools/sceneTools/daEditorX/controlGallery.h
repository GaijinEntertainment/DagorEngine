// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

class ControlGallery : public PropPanel::ControlEventHandler
{
public:
  ControlGallery()
  {
    panelWindow = IEditorCoreEngine::get()->createPropPanel(nullptr, "Control gallery");
    panelWindow->setEventHandler(this);
    fillControlGallery();
  }

  PropPanel::PanelWindowPropertyControl &getPanel()
  {
    G_ASSERT(panelWindow);
    return *panelWindow;
  }

private:
  enum
  {
    Gallery_EnabledControls = 1,
    Gallery_SeparatorBetweenControls,
    Gallery_Note,

    Gallery_BeginControl,

    Gallery_Button = Gallery_BeginControl,
    Gallery_ButtonLText,
    Gallery_CheckBox,
    Gallery_ColorBox,
    Gallery_Combo,
    Gallery_CurveEdit,
    Gallery_EditBox,
    Gallery_EditBox_Multiline,
    Gallery_EditFloat,
    Gallery_EditInt,
    Gallery_Extensible_Empty,
    Gallery_Extensible,
    Gallery_Extensible_Button,
    Gallery_ExtGroup,
    Gallery_ExtGroup_Button,
    Gallery_ExtGroup_EditBox,
    Gallery_FileButton,
    Gallery_FileEditBox,
    Gallery_GradientBox,
    Gallery_Group,
    Gallery_Group_Button,
    Gallery_Group_EditBox,
    Gallery_Group_Group,
    Gallery_Group_Group_Button,
    Gallery_Group_Group_EditBox,
    Gallery_GroupBox,
    Gallery_GroupBox_Button,
    Gallery_GroupBox_EditBox,
    Gallery_List,
    Gallery_List_Multiselect,
    Gallery_Point2,
    Gallery_Point3,
    Gallery_Point4,
    Gallery_Radio_One,
    Gallery_Radio_Two,
    Gallery_Radio_Three,
    Gallery_RadioGroup,
    Gallery_SimpleColor,
    Gallery_Static,
    Gallery_TargetButton,
    Gallery_TrackFloat,
    Gallery_TrackInt,
    Gallery_Tree,

    Gallery_EndControl,
  };

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == Gallery_EnabledControls)
    {
      enabledControls = !enabledControls;
      panelWindow->setPostEvent(pcb_id);
    }
    else if (pcb_id == Gallery_SeparatorBetweenControls)
    {
      separatorBetweenControls = !separatorBetweenControls;
      panelWindow->setPostEvent(pcb_id);
    }
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == Gallery_Extensible)
    {
      if (panel->getInt(pcb_id) == PropPanel::EXT_BUTTON_REMOVE)
        panel->getContainerById(pcb_id)->clear();
      else
        panel->getContainerById(pcb_id)->createButton(Gallery_Extensible_Button, "Button in Extensible", enabledControls);
    }
  }

  void onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == Gallery_EnabledControls || pcb_id == Gallery_SeparatorBetweenControls)
    {
      panelWindow->clear();
      fillControlGallery();
    }
  }

  void addControlGallerySeparator()
  {
    if (separatorBetweenControls)
      panelWindow->createSeparator();
  }

  void fillControlGallery()
  {
    panelWindow->createCheckBox(Gallery_EnabledControls, "Enabled controls", enabledControls);
    panelWindow->createCheckBox(Gallery_SeparatorBetweenControls, "Separator between controls", separatorBetweenControls);
    panelWindow->createEditBox(Gallery_Note, "Note",
      "There are few more controls in the\ncolor selector dialog. Also there is\nthe tab control and the toolbar.\nUse Ctrl+Shift+F12 "
      "for ImGui's theme editor.",
      false, true, true, hdpi::_pxScaled(100));
    panelWindow->createSeparator();

    panelWindow->createButton(Gallery_Button, "Button", enabledControls);
    addControlGallerySeparator();

    panelWindow->createButtonLText(Gallery_ButtonLText, "ButtonLText", enabledControls);
    addControlGallerySeparator();

    panelWindow->createCheckBox(Gallery_CheckBox, "CheckBox", false, enabledControls);
    addControlGallerySeparator();

    panelWindow->createColorBox(Gallery_ColorBox, "ColorBox", E3DCOLOR(0, 0, 0, 255), enabledControls);
    addControlGallerySeparator();

    Tab<String> comboValues;
    comboValues.emplace_back("One");
    comboValues.emplace_back("Two");
    comboValues.emplace_back("Three");
    panelWindow->createCombo(Gallery_Combo, "Combo", comboValues, "Two", enabledControls);
    addControlGallerySeparator();

    panelWindow->createCurveEdit(Gallery_CurveEdit, "CurveEdit", hdpi::_pxScaled(200), enabledControls);
    panelWindow->setInt(Gallery_CurveEdit, PropPanel::CURVE_CUBICPOLYNOM_APP);
    addControlGallerySeparator();

    panelWindow->createEditBox(Gallery_EditBox, "EditBox", "", enabledControls);
    addControlGallerySeparator();

    panelWindow->createEditBox(Gallery_EditBox_Multiline, "EditBox multiline", "", enabledControls, true, true, hdpi::_pxScaled(80));
    addControlGallerySeparator();

    panelWindow->createEditFloat(Gallery_EditFloat, "EditFloat", 0.0f, 2, enabledControls);
    addControlGallerySeparator();

    panelWindow->createEditInt(Gallery_EditInt, "EditInt", 0, enabledControls);
    addControlGallerySeparator();

    panelWindow->createExtensible(Gallery_Extensible_Empty);
    panelWindow->setText(Gallery_Extensible_Empty, "Extensible (empty)");
    addControlGallerySeparator();

    PropPanel::ContainerPropertyControl *extensible = panelWindow->createExtensible(Gallery_Extensible);
    extensible->setTextValue("Extensible");
    extensible->setIntValue(1 << PropPanel::EXT_BUTTON_REMOVE);
    extensible->createButton(Gallery_Extensible_Button, "Button in Extensible", enabledControls);
    addControlGallerySeparator();

    PropPanel::ContainerPropertyControl *extGoup = panelWindow->createExtGroup(Gallery_ExtGroup, "ExtGroup");
    extGoup->setIntValue(
      (1 << PropPanel::EXT_BUTTON_INSERT) | (1 << PropPanel::EXT_BUTTON_REMOVE) | (1 << PropPanel::EXT_BUTTON_RENAME));
    extGoup->createButton(Gallery_ExtGroup_Button, "Button", enabledControls);
    extGoup->createEditBox(Gallery_ExtGroup_EditBox, "EditBox", "", enabledControls);
    addControlGallerySeparator();

    panelWindow->createFileButton(Gallery_FileButton, "FileButton", "", enabledControls);
    addControlGallerySeparator();

    panelWindow->createFileEditBox(Gallery_FileEditBox, "FileEditBox", "", enabledControls);
    addControlGallerySeparator();

    panelWindow->createGradientBox(Gallery_GradientBox, "GradientBox", enabledControls);
    addControlGallerySeparator();

    PropPanel::ContainerPropertyControl *group = panelWindow->createGroup(Gallery_Group, "Group");
    group->createButton(Gallery_Group_Button, "Button", enabledControls);
    group->createEditBox(Gallery_Group_EditBox, "EditBox", "", enabledControls);
    group = group->createGroup(Gallery_Group_Group, "Group child");
    group->createButton(Gallery_Group_Group_Button, "Button", enabledControls);
    group->createEditBox(Gallery_Group_Group_EditBox, "EditBox", "", enabledControls);

    PropPanel::ContainerPropertyControl *groupBox = panelWindow->createGroupBox(Gallery_GroupBox, "GroupBox");
    groupBox->createButton(Gallery_GroupBox_Button, "Button", enabledControls);
    groupBox->createEditBox(Gallery_GroupBox_EditBox, "EditBox", "", enabledControls);

    panelWindow->createList(Gallery_List, "List", comboValues, "Two", enabledControls);
    addControlGallerySeparator();

    panelWindow->createMultiSelectList(Gallery_List_Multiselect, comboValues, hdpi::_pxScaled(200), enabledControls);
    addControlGallerySeparator();

    panelWindow->createPoint2(Gallery_Point2, "Point2", Point2::ZERO, 2, enabledControls);
    addControlGallerySeparator();

    panelWindow->createPoint3(Gallery_Point3, "Point3", Point3::ZERO, 2, enabledControls);
    addControlGallerySeparator();

    panelWindow->createPoint4(Gallery_Point4, "Point4", Point4::ZERO, 2, enabledControls);
    addControlGallerySeparator();

    PropPanel::ContainerPropertyControl *radioGroup = panelWindow->createRadioGroup(Gallery_RadioGroup, "RadioGroup");
    radioGroup->createRadio(Gallery_Radio_One, "One", enabledControls);
    radioGroup->createRadio(Gallery_Radio_Two, "Two", enabledControls);
    radioGroup->createRadio(Gallery_Radio_Three, "Three", enabledControls);
    addControlGallerySeparator();

    panelWindow->createSeparator();

    panelWindow->createSimpleColor(Gallery_SimpleColor, "SimpleColor", E3DCOLOR(0, 0, 0, 255), enabledControls);
    addControlGallerySeparator();

    panelWindow->createStatic(Gallery_Static, "Static");
    addControlGallerySeparator();

    panelWindow->createTargetButton(Gallery_TargetButton, "TargetButton", "", enabledControls);
    addControlGallerySeparator();

    panelWindow->createTrackFloat(Gallery_TrackFloat, "TrackFloat", 0.0f, -1.0f, 1.0f, 0.01f, enabledControls);
    addControlGallerySeparator();

    panelWindow->createTrackInt(Gallery_TrackInt, "TrackInt", 0, -180, 180, 1, enabledControls);
    addControlGallerySeparator();

    PropPanel::ContainerPropertyControl *tree = panelWindow->createTree(Gallery_Tree, "Tree", hdpi::_pxScaled(400));
    PropPanel::TLeafHandle treeRoot = tree->createTreeLeaf(0, "Root", nullptr);
    tree->createTreeLeaf(treeRoot, "One", nullptr);
    tree->createTreeLeaf(treeRoot, "Two", nullptr);
    tree->createTreeLeaf(treeRoot, "Three", nullptr);
    addControlGallerySeparator();

    for (int id = Gallery_BeginControl; id < Gallery_EndControl; ++id)
      panelWindow->setTooltipId(id, String(32, "Tooltip %d", id));
  }

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  bool enabledControls = true;
  bool separatorBetweenControls = false;
};
