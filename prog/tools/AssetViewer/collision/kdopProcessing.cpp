// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kdopProcessing.h"
#include "propPanelPids.h"
#include "collisionUtils.h"
#include <debug/dag_debug3d.h>
#include <gui/dag_stdGuiRenderEx.h>

void KdopProcessing::init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel)
{
  collisionRes = collision_res;
  panel = prop_panel;
  selectedKdop.reset();
}

void KdopProcessing::calcSelectedKdop()
{
  selectedSettings.preset = static_cast<KdopPreset>(panel->getInt(PID_SELECTED_PRESET));
  switchSlidersByPreset();
  selectedSettings.rotX = panel->getInt(PID_ROT_X);
  selectedSettings.rotY = panel->getInt(PID_ROT_Y);
  selectedSettings.rotZ = panel->getInt(PID_ROT_Z);
  selectedSettings.segmentsCountX = panel->getInt(PID_SEGMENTS_COUNT_X);
  selectedSettings.segmentsCountY = panel->getInt(PID_SEGMENTS_COUNT_Y);
  if (panel->getBool(PID_KDOP_CUT_OFF_PLANES))
  {
    selectedSettings.cutOffThreshold = panel->getFloat(PID_KDOP_CUT_OFF_THRESHOLD);
  }
  panel->setEnabledById(PID_SAVE_KDOP, selectedSettings.preset != KdopPreset::SET_EMPTY);

  calcKdop(selectedSettings);
}

void KdopProcessing::calcKdop(const KdopSettings &settings)
{
  if (settings.preset == KdopPreset::SET_EMPTY)
    return;

  dag::Vector<Point3_vec4> verts;
  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  for (const auto &refNode : settings.selectedNodes.refNodes)
  {
    G_ASSERT_LOG(add_verts_from_node(nodes, refNode, verts), "Collision node not found: %s", refNode);
  }
  selectedKdop.setPreset(settings.preset, settings.cutOffThreshold, settings.segmentsCountX, settings.segmentsCountY);
  selectedKdop.setRotation(Point3(settings.rotX, settings.rotY, settings.rotZ));
  if (!verts.empty())
  {
    selectedKdop.calcKdop(verts);
  }
}

void KdopProcessing::setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes_settings)
{
  selectedSettings.selectedNodes = eastl::move(selected_nodes_settings);
}

void KdopProcessing::addSelectedKdop() { kdops.push_back(eastl::move(selectedKdop)); }

void KdopProcessing::setKdopParams(const KdopSettings &settings)
{
  selectedSettings.rotX = settings.rotX;
  selectedSettings.rotY = settings.rotY;
  selectedSettings.rotZ = settings.rotZ;
  selectedSettings.preset = settings.preset;
  selectedSettings.segmentsCountX = settings.segmentsCountX;
  selectedSettings.segmentsCountY = settings.segmentsCountY;
  selectedSettings.cutOffThreshold = settings.cutOffThreshold;
}

void KdopProcessing::updatePanelParams()
{
  panel->setInt(PID_ROT_X, selectedSettings.rotX);
  panel->setInt(PID_ROT_Y, selectedSettings.rotY);
  panel->setInt(PID_ROT_Z, selectedSettings.rotZ);
  panel->setInt(PID_SELECTED_PRESET, selectedSettings.preset);
  panel->setInt(PID_SEGMENTS_COUNT_X, selectedSettings.segmentsCountX);
  panel->setInt(PID_SEGMENTS_COUNT_Y, selectedSettings.segmentsCountY);
  panel->setBool(PID_KDOP_CUT_OFF_PLANES, selectedSettings.cutOffThreshold > 0.0f);
  panel->setFloat(PID_KDOP_CUT_OFF_THRESHOLD, selectedSettings.cutOffThreshold);
}

void KdopProcessing::switchSlidersByPreset()
{
  switch (selectedSettings.preset)
  {
    case SET_EMPTY:
    case SET_6DOP:
    case SET_14DOP:
    case SET_18DOP:
    case SET_26DOP:
      panel->setEnabledById(PID_SEGMENTS_COUNT_X, false);
      panel->setEnabledById(PID_SEGMENTS_COUNT_Y, false);
      break;

    case SET_KDOP_FIXED_X:
    case SET_KDOP_FIXED_Y:
    case SET_KDOP_FIXED_Z:
      panel->setCaption(PID_SEGMENTS_COUNT_X, "Segments count:");
      panel->setEnabledById(PID_SEGMENTS_COUNT_X, true);
      panel->setEnabledById(PID_SEGMENTS_COUNT_Y, false);
      break;

    case SET_CUSTOM_KDOP:
      panel->setCaption(PID_SEGMENTS_COUNT_X, "X segments count:");
      panel->setEnabledById(PID_SEGMENTS_COUNT_X, true);
      panel->setEnabledById(PID_SEGMENTS_COUNT_Y, true);
      break;
  }
}

void KdopProcessing::renderKdop(bool is_faded)
{
  const auto drawKdop = [&](const Kdop &kdop, bool is_faded = false, E3DCOLOR color = E3DCOLOR_MAKE(153, 255, 153, 255)) {
    if (is_faded)
      color.a = 30;
    for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
    {
      uint32_t size = kdop.planeDefinitions[i].vertices.size();
      for (int j = 0; j < size; ++j)
      {
        Point3 p1 = kdop.planeDefinitions[i].vertices[j];
        Point3 p2 = kdop.planeDefinitions[i].vertices[(j + 1) % size];

        draw_cached_debug_line(p1, p2, color);
      }
    }
  };

  const auto drawKdopFaces = [&](const Kdop &kdop) {
    E3DCOLOR color = E3DCOLOR_MAKE(153, 255, 153, 255);
    for (int i = 0; i < kdop.indices.size(); i += 3)
    {
      Point3 p1 = kdop.vertices[kdop.indices[i]];
      Point3 p2 = kdop.vertices[kdop.indices[i + 1]];
      Point3 p3 = kdop.vertices[kdop.indices[i + 2]];

      draw_cached_debug_line(p1, p2, color);
      draw_cached_debug_line(p2, p3, color);
      draw_cached_debug_line(p3, p1, color);
    }
  };

  const auto drawKdopDirs = [&](const Kdop &kdop) {
    for (const auto &def : kdop.planeDefinitions)
    {
      Point3 p1 = kdop.center;
      Point3 p2 = def.planeNormal * def.limit + p1;
      draw_cached_debug_line(p1, p2, COLOR_BLUE);
    }
  };

  begin_draw_cached_debug_lines();

  set_cached_debug_lines_wtm(TMatrix::IDENT);
  for (int i = 0; i < kdops.size(); ++i)
  {
    drawKdop(kdops[i], is_faded, E3DCOLOR(colors[(i) % (sizeof(colors) / sizeof(colors[0]))]));
  }

  if (showKdop && !selectedKdop.vertices.empty())
  {
    drawKdop(selectedKdop);
  }
  if (showKdopFaces && !selectedKdop.vertices.empty())
  {
    drawKdopFaces(selectedKdop);
  }
  if (showKdopDirs && !selectedKdop.vertices.empty())
  {
    drawKdopDirs(selectedKdop);
  }

  end_draw_cached_debug_lines();
}

static void fill_preset_names(Tab<String> &preset_names)
{
  preset_names.push_back() = "6-dop";
  preset_names.push_back() = "14-dop";
  preset_names.push_back() = "18-dop";
  preset_names.push_back() = "26-dop";
  preset_names.push_back() = "k-dop with fixed X-axis";
  preset_names.push_back() = "k-dop with fixed Y-axis";
  preset_names.push_back() = "k-dop with fixed Z-axis";
  preset_names.push_back() = "Custom k-dop";
}

void KdopProcessing::fillKdopPanel()
{
  PropPanel::ContainerPropertyControl &kdopGroup = *panel->createGroup(PID_KDOP_GROUP, "k-dop options");
  kdopGroup.createCheckBox(PID_SHOW_KDOP, "Show k-dop", showKdop);
  kdopGroup.createCheckBox(PID_SHOW_KDOP_FACES, "Show k-dop faces", showKdopFaces);
  kdopGroup.createCheckBox(PID_SHOW_KDOP_DIRS, "Show k-dop directions", showKdopDirs);
  {
    auto &kdopRotateGroup = *kdopGroup.createGroup(PID_KDOP_ROT_GROUP, "k-dop rotations");
    kdopRotateGroup.createTrackInt(PID_ROT_X, "Rotate around X, deg:", 0, -180, 180, 1);
    kdopRotateGroup.createTrackInt(PID_ROT_Y, "Rotate around Y, deg:", 0, -180, 180, 1);
    kdopRotateGroup.createTrackInt(PID_ROT_Z, "Rotate around Z, deg:", 0, -180, 180, 1);
    kdopRotateGroup.setBoolValue(true);
  }
  {
    auto &kdopCutOffPlanesGroup = *kdopGroup.createGroup(PID_KDOP_CUT_OFF_GROUP, "k-dop cut off planes");
    kdopCutOffPlanesGroup.createCheckBox(PID_KDOP_CUT_OFF_PLANES, "Cut off small k-dop planes");
    kdopCutOffPlanesGroup.createTrackFloat(PID_KDOP_CUT_OFF_THRESHOLD, "threshold cut off area, %", 0.0f, 0.0f, 40.0f, 0.01f, false);
    kdopCutOffPlanesGroup.setBoolValue(true);
  }

  Tab<String> presetNames(midmem);
  fill_preset_names(presetNames);
  kdopGroup.createCombo(PID_SELECTED_PRESET, "Selected presset:", presetNames, -1);
  kdopGroup.createTrackInt(PID_SEGMENTS_COUNT_X, "X segments count:", 4, 4, 20, 1, false);
  kdopGroup.createTrackInt(PID_SEGMENTS_COUNT_Y, "Y segments count:", 4, 4, 20, 1, false);
  kdopGroup.createButton(PID_PRINT_KDOP_LOG, "Print kdop debug-log");
  kdopGroup.createSeparator();
  kdopGroup.createButton(PID_SAVE_KDOP, "Save settings", false);
  kdopGroup.createButton(PID_CANCEL_KDOP, "Cancel");
}
