// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <EASTL/string_view.h>
#include <debug/dag_debug3d.h>
#include <scene/dag_tiledScene.h>
#include <util/dag_string.h>
#include <math/dag_mathUtils.h>
#include <render/debugBoxRenderer.h>
#include <rendInst/rendInstExtra.h>


DebugBoxRenderer::DebugBoxRenderer(eastl::vector<const char *> &&names, eastl::vector<const scene::TiledScene *> &&scenes,
  eastl::vector<E3DCOLOR> &&colors) :
  groupNames(names), groupScenes(scenes), groupColors(colors), verifyRIDistance(-1), needLogText(false)
{
  G_ASSERT(groupNames.size() == groupScenes.size());
  G_ASSERT(groupNames.size() == groupColors.size());
  groupStates.assign(groupNames.size(), false);
}

String DebugBoxRenderer::processCommand(const char *argv[], int argc)
{
  if (argc == 1)
  {
    groupStates.assign(groupNames.size(), false);
    String help("Usage: render.show_boxes <boxes groups>.\n"
                "This command toggles boxes groups independently.\n"
                "Available boxes groups: all, none, ");
    for (int i = 0; i < (int)groupNames.size() - 1; ++i)
      help = help + groupNames[i] + ", ";
    return help + groupNames.back() + ".";
  }
  for (int argId = 1; argId < argc; ++argId)
    if (strcmp("all", argv[argId]) == 0)
    {
      groupStates.assign(groupNames.size(), true);
      return String("All boxes are visible.");
    }
    else if (strcmp("none", argv[argId]) == 0)
    {
      groupStates.assign(groupNames.size(), false);
      return String("All boxes are invisible.");
    }
  String response("Groups ");
  for (int argId = 1; argId < argc; ++argId)
    for (int i = 0; i < groupNames.size(); ++i)
      if (strcmp(groupNames[i], argv[argId]) == 0)
      {
        groupStates[i] = !groupStates[i];
        response = response + groupNames[i] + " ";
      }
  return response + "are switched.";
}

void draw_transfromed_box(mat44f_cref m, const BBox3 &box, E3DCOLOR color)
{
  alignas(16) TMatrix tm;
  v_mat_43ca_from_mat44(tm.m[0], m);
  set_cached_debug_lines_wtm(tm);
  draw_cached_debug_box(box, color);
}

void DebugBoxRenderer::render(const RiGenVisibility *visibility, const Point3 &view_pos)
{
  TIME_D3D_PROFILE(DebugBoxRenderer)
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  mat44f globtm;
  d3d::getglobtm(globtm);
  const vec4f vpos_distscale = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, scene::defaultDisappearSq / (80 * 80));
  begin_draw_cached_debug_lines(false, false);
  for (int i = 0; i < groupNames.size(); ++i)
  {
    if (groupStates[i] && groupScenes[i])
    {
      E3DCOLOR currentColor = groupColors[i];
      groupScenes[i]->template frustumCull<false, true, false>(globtm, vpos_distscale, 0, 0, nullptr,
        [&box, &currentColor](scene::node_index, mat44f_cref m, vec4f) { draw_transfromed_box(m, box, currentColor); });
    }
  }

  if (verifyRIDistance > 0)
  {
    BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));

    float squareDistanceMax = verifyRIDistance * verifyRIDistance;
    mat44f globtm;
    d3d::getglobtm(globtm);
    constexpr int binCount = 10;
    eastl::vector<eastl::vector_map<eastl::string_view, int>> name_count_bins(binCount);

    vec4f pos_distscale = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, 1.f);
    for (int i : {0, 2}) // 0 indoor_walls, 2 - envi_probes in WorldRenderer::createDebugBoxRender
      groupScenes[i]->frustumCull<false, true, false>(globtm, pos_distscale, 0, 0, nullptr,
        [&](scene::node_index, mat44f_cref m, vec4f sqDist) {
          float squareDistance = v_extract_x(sqDist);
          if (squareDistanceMax < squareDistance)
            return;

          auto lambda = [&](mat44f_cref m, const BBox3 &box, const char *name) {
            draw_transfromed_box(m, box, E3DCOLOR(0, 100, 100));
            float distToRI = v_extract_x(v_length3(v_sub(pos_distscale, m.col3)));
            int bin = distToRI / verifyRIDistance * binCount;
            if (bin >= binCount)
              bin = binCount - 1;
            if (needLogText)
              name_count_bins[bin][eastl::string_view(name)]++;
          };

          if (rendinst::gatherRIGenExtraBboxes(visibility, m, lambda))
          {
            draw_transfromed_box(m, box, E3DCOLOR(255, 0, 0));
          }
        });
    if (needLogText)
    {
      for (int bin = 0; bin < binCount; bin++)
      {
        float minDist = verifyRIDistance * (bin + 0.f) / binCount;
        float maxDist = verifyRIDistance * (bin + 1.f) / binCount;
        debug("Range : %f - %f", minDist, maxDist);
        G_UNUSED(minDist);
        G_UNUSED(maxDist);
        eastl::vector_map<int, eastl::string_view> count_name_map; // for sort
        for (const auto &it : name_count_bins[bin])
          count_name_map[-it.second] = it.first; //-it.second from max to min
        for (const auto &it : count_name_map)
        {
          debug("RI name %s %d", it.second.data(), -it.first);
          G_UNUSED(it);
        }
      }
      needLogText = false;
    }
  }
  end_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(TMatrix::IDENT);
}
