// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/colors.h>
#include <ioSys/dag_dataBlock.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_gizmoSettings.h>

E3DCOLOR GizmoSettings::axisColor[3] = {E3DCOLOR(255, 0, 0), E3DCOLOR(0, 255, 0), E3DCOLOR(0, 0, 255)};

void GizmoSettings::load(DataBlock &block)
{
  int gizmoStyleId = block.getInt("gizmoStyle", eastl::to_underlying(GizmoEventFilter::Style::Classic));
  EDITORCORE->getGizmoEventFilter().setGizmoStyle(static_cast<GizmoEventFilter::Style>(gizmoStyleId));
  lineThickness = block.getInt("gizmoLineThickness", lineThickness);
  viewportGizmoScaleFactor = block.getReal("viewportAxisScaleFactor", viewportGizmoScaleFactor);

  // colors
  const char *colorNames[3] = {"gizmoColorX", "gizmoColorY", "gizmoColorZ"};
  for (int i = 0; i < 3; ++i)
  {
    if (block.findParam(colorNames[i]) < 0)
      continue;
    axisColor[i] = block.getE3dcolor(colorNames[i], axisColor[i]);
    overrideColor[i] = true;
  }
}

void GizmoSettings::save(DataBlock &block)
{
  block.setInt("gizmoStyle", eastl::to_underlying(EDITORCORE->getGizmoEventFilter().getGizmoStyle()));
  block.setInt("gizmoLineThickness", lineThickness);
  block.setReal("viewportAxisScaleFactor", viewportGizmoScaleFactor);

  const char *colorNames[3] = {"gizmoColorX", "gizmoColorY", "gizmoColorZ"};
  for (int i = 0; i < 3; ++i)
  {
    if (!overrideColor[i])
      continue;
    block.setE3dcolor(colorNames[i], axisColor[i]);
  }
}

void GizmoSettings::updateAxisColors()
{
  const int pid[3] = {PropPanel::ColorOverride::AXIS_X, PropPanel::ColorOverride::AXIS_Y, PropPanel::ColorOverride::AXIS_Z};
  for (int i = 0; i < 3; ++i)
  {
    if (overrideColor[i])
      continue;
    const ImVec4 imColor = PropPanel::getOverriddenColor(pid[i]);
    axisColor[i] = E3DCOLOR(real2uchar(imColor.x), real2uchar(imColor.y), real2uchar(imColor.z));
  }
}

void GizmoSettings::retrieveAxisColors(E3DCOLOR &colorX, E3DCOLOR &colorY, E3DCOLOR &colorZ)
{
  if (overrideColor[0])
  {
    colorX = axisColor[0];
  }
  else
  {
    const ImVec4 imColorX = PropPanel::getOverriddenColor(PropPanel::ColorOverride::AXIS_X);
    colorX = E3DCOLOR(real2uchar(imColorX.x), real2uchar(imColorX.y), real2uchar(imColorX.z));
  }
  if (overrideColor[1])
  {
    colorY = axisColor[1];
  }
  else
  {
    const ImVec4 imColorY = PropPanel::getOverriddenColor(PropPanel::ColorOverride::AXIS_Y);
    colorY = E3DCOLOR(real2uchar(imColorY.x), real2uchar(imColorY.y), real2uchar(imColorY.z));
  }
  if (overrideColor[2])
  {
    colorZ = axisColor[2];
  }
  else
  {
    const ImVec4 imColorZ = PropPanel::getOverriddenColor(PropPanel::ColorOverride::AXIS_Z);
    colorZ = E3DCOLOR(real2uchar(imColorZ.x), real2uchar(imColorZ.y), real2uchar(imColorZ.z));
  }
}