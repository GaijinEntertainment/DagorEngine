// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <EditorCore/ec_GizmoSettingsDialog.h>
#include <EditorCore/ec_gizmoSettings.h>
#include <EditorCore/ec_gizmofilter.h>
#include <propPanel/control/container.h>

enum
{
  PID_GIZMO_TYPE = 1,
  PID_VIEWPORT_GIZMO_SIZE,
  PID_GIZMO_LINE_THICKNESS,
  PID_GIZMO_AXIS_X_COLOR,
  PID_GIZMO_AXIS_Y_COLOR,
  PID_GIZMO_AXIS_Z_COLOR
};

GizmoSettingsDialog::GizmoSettingsDialog() :
  PropPanel::DialogWindow(nullptr, hdpi::_pxScaled(300), hdpi::_pxScaled(400), "Gizmo settings")
{
  fillPanel();
  PropPanel::ContainerPropertyControl *buttonsContainer = buttonsPanel->getContainer();
  buttonsContainer->removeById(PropPanel::DIALOG_ID_CANCEL);
  buttonsContainer->setText(PropPanel::DIALOG_ID_OK, "Close");
}

void GizmoSettingsDialog::fillPanel()
{
  PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();

  auto &gizmo = EDITORCORE->getGizmoEventFilter();
  Tab<String> gizmoTypes;
  gizmoTypes.push_back() = "Classic";
  gizmoTypes.push_back() = "New";
  panel->createCombo(PID_GIZMO_TYPE, "Type:", gizmoTypes, eastl::to_underlying(gizmo.getGizmoStyle()));

  panel->createTrackFloat(PID_VIEWPORT_GIZMO_SIZE, "Viewport axis size:", GizmoSettings::viewportGizmoScaleFactor, 1.0f, 2.0f, 0.01f);
  panel->createTrackInt(PID_GIZMO_LINE_THICKNESS, "Line thickness:", GizmoSettings::lineThickness, 1, 7, 1);

  GizmoSettings::updateAxisColors();
  panel->createColorBox(PID_GIZMO_AXIS_X_COLOR, "X axis color:", GizmoSettings::axisColor[0]);
  panel->createColorBox(PID_GIZMO_AXIS_Y_COLOR, "Y axis color:", GizmoSettings::axisColor[1]);
  panel->createColorBox(PID_GIZMO_AXIS_Z_COLOR, "Z axis color:", GizmoSettings::axisColor[2]);
}

void GizmoSettingsDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_GIZMO_TYPE:
    {
      auto &gizmo = EDITORCORE->getGizmoEventFilter();
      gizmo.setGizmoStyle(static_cast<GizmoEventFilter::Style>(panel->getInt(pcb_id)));
      break;
    }
    case PID_VIEWPORT_GIZMO_SIZE: GizmoSettings::viewportGizmoScaleFactor = panel->getFloat(pcb_id); break;
    case PID_GIZMO_LINE_THICKNESS: GizmoSettings::lineThickness = panel->getInt(pcb_id); break;
    case PID_GIZMO_AXIS_X_COLOR:
      GizmoSettings::overrideColor[0] = true;
      GizmoSettings::axisColor[0] = panel->getColor(pcb_id);
      break;
    case PID_GIZMO_AXIS_Y_COLOR:
      GizmoSettings::overrideColor[1] = true;
      GizmoSettings::axisColor[1] = panel->getColor(pcb_id);
      break;
    case PID_GIZMO_AXIS_Z_COLOR:
      GizmoSettings::overrideColor[2] = true;
      GizmoSettings::axisColor[2] = panel->getColor(pcb_id);
      break;
  }
}
