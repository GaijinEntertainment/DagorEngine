#include "convexVhacdProcessing.h"
#include "propPanelPids.h"
#include "collisionUtils.h"
#include <libTools/collision/vhacdMeshChecker.h>
#include <debug/dag_debug3d.h>
#include <util/dag_delayedAction.h>

void ConvexVhacdProcessing::VhacdProgress::Update(const double overallProgress, const double stageProgress, const char *const stage,
  const char *operation)
{
  add_delayed_action(make_delayed_action([&, overallProgress]() {
    if (!isComputed)
      processing->printCalcProgress(overallProgress);
  }));
}

void ConvexVhacdProcessing::VhacdProgress::NotifyVHACDComplete()
{
  add_delayed_action(make_delayed_action([&]() {
    processing->printCalcProgress(100.f);
    processing->checkComputedInterface();
    isComputed = true;
  }));
}

void ConvexVhacdProcessing::init(CollisionResource *collision_res, PropertyContainerControlBase *prop_panel)
{
  collisionRes = collision_res;
  panel = prop_panel;
  progressCallback.processing = this;
}

void ConvexVhacdProcessing::calcSelectedInterface()
{
  selectedSettings.depth = panel->getInt(PID_CONVEX_DEPTH);
  selectedSettings.maxHulls = panel->getInt(PID_CONVEX_MAX_HULLS);
  selectedSettings.maxVerts = panel->getInt(PID_CONVEX_MAX_VERTS);
  selectedSettings.resolution = panel->getInt(PID_CONVEX_RESOLUTION);
  calcInterface(selectedSettings);
}

void ConvexVhacdProcessing::calcInterface(const ConvexVhacdSettings &settings)
{
  selectedInterface->Clean();
  selectedInterface->Release();
  selectedInterface = VHACD::CreateVHACD_ASYNC();
  progressCallback.isComputed = false;
  VHACD::IVHACD::Parameters p;
  dag::Vector<float> verts;
  dag::Vector<uint32_t> indices;
  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  for (const auto &refNode : settings.selectedNodes.refNodes)
  {
    G_ASSERT_LOG(add_verts_and_indices_from_node(nodes, refNode, verts, indices), "Collision node not found: %s", refNode);
  }
  p.m_maxRecursionDepth = settings.depth;
  p.m_maxConvexHulls = settings.maxHulls;
  p.m_maxNumVerticesPerCH = settings.maxVerts;
  p.m_resolution = settings.resolution;
  p.m_callback = &progressCallback;
  selectedInterface->Compute(verts.data(), verts.size() / 3, indices.data(), indices.size() / 3, p);
}

void ConvexVhacdProcessing::setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes)
{
  selectedSettings.selectedNodes = eastl::move(selected_nodes);
}

void ConvexVhacdProcessing::addSelectedInterface()
{
  interfaces.push_back(eastl::move(selectedInterface));
  selectedInterface = VHACD::CreateVHACD_ASYNC();
}

void ConvexVhacdProcessing::printCalcProgress(const double progress)
{
  panel->setText(PID_CONVEX_BUILD_INFO, String(100, "Progress %d%%", progress));
}

void ConvexVhacdProcessing::setConvexVhacdParams(const ConvexVhacdSettings &settings)
{
  selectedSettings.depth = settings.depth;
  selectedSettings.maxHulls = settings.maxHulls;
  selectedSettings.maxVerts = settings.maxVerts;
  selectedSettings.resolution = settings.resolution;
}

void ConvexVhacdProcessing::updatePanelParams()
{
  panel->setInt(PID_CONVEX_DEPTH, selectedSettings.depth);
  panel->setInt(PID_CONVEX_MAX_HULLS, selectedSettings.maxHulls);
  panel->setInt(PID_CONVEX_MAX_VERTS, selectedSettings.maxVerts);
  panel->setInt(PID_CONVEX_RESOLUTION, selectedSettings.resolution);
}

void ConvexVhacdProcessing::checkComputedInterface()
{
  for (int i = 0; i < selectedInterface->GetNConvexHulls(); ++i)
  {
    Tab<Point3> verts;
    Tab<int> indices;
    VHACD::IVHACD::ConvexHull ch;
    selectedInterface->GetConvexHull(i, ch);
    for (const auto &p : ch.m_points)
    {
      verts.push_back(Point3(static_cast<float>(p.mX), static_cast<float>(p.mY), static_cast<float>(p.mZ)));
    }
    for (const auto &t : ch.m_triangles)
    {
      indices.push_back(t.mI0);
      indices.push_back(t.mI2);
      indices.push_back(t.mI1);
    }

    if (is_vhacd_has_not_valid_faces(verts, indices))
    {
      panel->setEnabledById(PID_SAVE_CONVEX_VHACD, false);
      panel->setText(PID_CONVEX_BUILD_INFO, "Convex have not valid faces, try another params");
      return;
    }
    else
    {
      panel->setEnabledById(PID_SAVE_CONVEX_VHACD, true);
    }
  }
}

void ConvexVhacdProcessing::renderVhacd(bool is_faded)
{
  const auto drawVhacd = [&](const VHACD::IVHACD *interface, bool is_faded = false) {
    for (int i = 0; i < interface->GetNConvexHulls(); ++i)
    {
      E3DCOLOR color = E3DCOLOR(colors[(i) % (sizeof(colors) / sizeof(colors[0]))]);
      if (is_faded)
        color.a = 30;
      dag::Vector<Point3> verts;
      VHACD::IVHACD::ConvexHull ch;
      interface->GetConvexHull(i, ch);
      for (int j = 0; j < ch.m_triangles.size(); ++j)
      {
        VHACD::Vertex v1 = ch.m_points[ch.m_triangles[j].mI0];
        VHACD::Vertex v2 = ch.m_points[ch.m_triangles[j].mI1];
        VHACD::Vertex v3 = ch.m_points[ch.m_triangles[j].mI2];

        Point3 p1{static_cast<float>(v1.mX), static_cast<float>(v1.mY), static_cast<float>(v1.mZ)};
        Point3 p2{static_cast<float>(v2.mX), static_cast<float>(v2.mY), static_cast<float>(v2.mZ)};
        Point3 p3{static_cast<float>(v3.mX), static_cast<float>(v3.mY), static_cast<float>(v3.mZ)};

        draw_cached_debug_line(p1, p2, color);
        draw_cached_debug_line(p2, p3, color);
        draw_cached_debug_line(p3, p1, color);
      }
    }
  };

  begin_draw_cached_debug_lines();

  if (showConvexVHACD && selectedInterface->IsReady() && selectedInterface->GetNConvexHulls())
  {
    drawVhacd(selectedInterface);
  }

  for (const auto *interface : interfaces)
  {
    drawVhacd(interface, is_faded);
  }

  end_draw_cached_debug_lines();
}

void ConvexVhacdProcessing::fillConvexVhacdPanel()
{
  PropertyContainerControlBase &convexGroup = *panel->createGroup(PID_CONVEX_VHACD_GROUP, "convex v-hacd options");
  convexGroup.createCheckBox(PID_SHOW_CONVEX_VHACD, "Show convex v-hacd", showConvexVHACD);
  convexGroup.createTrackInt(PID_CONVEX_DEPTH, "Convex depth", 10, 1, 20, 1);
  convexGroup.createTrackInt(PID_CONVEX_MAX_HULLS, "Max convex hulls", 4, 1, 32, 1);
  convexGroup.createTrackInt(PID_CONVEX_MAX_VERTS, "Max verts", 32, 4, 64, 1);
  convexGroup.createTrackInt(PID_CONVEX_RESOLUTION, "Resolution", 40000, 10000, 1000000, 10000);
  convexGroup.createStatic(PID_CONVEX_BUILD_INFO, "Convex build progress");
  convexGroup.createSeparator();
  convexGroup.createButton(PID_SAVE_CONVEX_VHACD, "Save settings");
  convexGroup.createButton(PID_CANCEL_CONVEX_VHACD, "Cancel");
}
