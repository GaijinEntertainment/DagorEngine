// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineDebugDraw_api.h"
#include <levelSplines/levelSplines.h>
#include <math/dag_bezier.h>
#include <math/dag_e3dColor.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <util/dag_console.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_localConv.h>
#include <math/dag_mathBase.h>
#include <string.h>

namespace levelsplines
{

enum DebugDrawFlags
{
  DDF_LINES = 1,
  DDF_NODES = 2,
  DDF_NAMES = 4,
};

static const E3DCOLOR SPLINE_COLOR(255, 160, 0, 255);
static const E3DCOLOR NODE_COLOR(0, 255, 255, 255);

static const float DRAW_DIST = 3000.f;
static const float NAMES_DIST = 300.f;
static const float LINE_STEP = 10.f;
static const int MAX_SEG_STEPS = 64;
static const float NODE_RADIUS = 2.f;

static unsigned debug_flags = 0;
static SimpleString debug_name;

bool is_debug_draw_on() { return debug_flags != 0; }

static bool is_drawable(const DebugSpline &s) { return s.spl && !s.spl->segs.empty(); }

static void draw_spline(const DebugSpline &s, const Point3 &cam_pos, bool no_cull)
{
  const bool wantName = (debug_flags & DDF_NAMES) && s.name;
  float nameDistSq = no_cull ? FLT_MAX : NAMES_DIST * NAMES_DIST;
  Point3 namePos;
  bool hasName = false;
  for (const auto &seg : s.spl->segs)
  {
    Point3 prev = seg.point(0.f);
    if (!no_cull && length(prev - cam_pos) - seg.len > DRAW_DIST)
      continue;
    const int steps = clamp(int(ceilf(seg.len / LINE_STEP)), 1, MAX_SEG_STEPS);
    for (int k = 1; k <= steps; k++)
    {
      const Point3 p = seg.point(float(k) / steps);
      if (debug_flags & DDF_LINES)
        draw_cached_debug_line(prev, p, SPLINE_COLOR);
      if (wantName)
      {
        const float distSq = lengthSq(p - cam_pos);
        if (distSq < nameDistSq)
        {
          nameDistSq = distSq;
          namePos = p;
          hasName = true;
        }
      }
      prev = p;
    }
  }
  if (hasName)
    add_debug_text_mark(namePos, s.name);
}

static void draw_nodes(dag::ConstSpan<DebugSpline> splines, const SplineIntersections &paths, const Point3 &cam_pos, int only_id)
{
  for (int i = 0, n = paths.getNodesCount(); i < n; i++)
  {
    Point3 sum = Point3(0, 0, 0);
    int cnt = 0;
    bool onOnly = only_id < 0;
    for (const IntersectionSplinePoint &pt : paths.getIntersectionNode(i).points)
      if (pt.splineId < splines.size() && is_drawable(splines[pt.splineId]))
      {
        sum += splines[pt.splineId].spl->get_pt(pt.pos);
        cnt++;
        onOnly |= pt.splineId == only_id;
      }
    if (!cnt || !onOnly)
      continue;
    const Point3 center = sum / cnt;
    if (only_id >= 0 || length(center - cam_pos) < DRAW_DIST)
      draw_cached_debug_sphere(center, NODE_RADIUS, NODE_COLOR, 8);
  }
}

void draw_debug_splines(dag::ConstSpan<DebugSpline> splines, const SplineIntersections *paths, const Point3 &cam_pos)
{
  if (!debug_flags)
    return;

  int onlyId = -1;
  if (!debug_name.empty())
  {
    for (int i = 0; i < splines.size() && onlyId < 0; i++)
      if (splines[i].name && strcmp(splines[i].name, debug_name.str()) == 0)
        onlyId = i;
    if (onlyId < 0 || !is_drawable(splines[onlyId]))
    {
      if (onlyId < 0)
        console::print_d("app.debug_level_splines: spline '%s' not found; keys: off lines nodes names", debug_name.str());
      else
        console::print_d("app.debug_level_splines: spline '%s' has no curve", debug_name.str());
      debug_flags = 0;
      return;
    }
  }

  if (debug_flags & (DDF_LINES | DDF_NAMES))
    for (int i = 0; i < splines.size(); i++)
      if ((onlyId < 0 || i == onlyId) && is_drawable(splines[i]))
        draw_spline(splines[i], cam_pos, onlyId >= 0);

  if (paths && (debug_flags & DDF_NODES))
    draw_nodes(splines, *paths, cam_pos, onlyId);
}

} // namespace levelsplines

static bool level_splines_console_handler(const char *argv[], int argc)
{
  using namespace levelsplines;
  static const struct
  {
    const char *key;
    unsigned flag;
  } keys[] = {{"lines", DDF_LINES}, {"nodes", DDF_NODES}, {"names", DDF_NAMES}};
  static const unsigned ALL = DDF_LINES | DDF_NODES | DDF_NAMES;

  eastl::vector<console::CommandOptions> options;
  options.emplace_back("off");
  for (const auto &k : keys)
  {
    eastl::vector<console::CommandOptions> others;
    for (const auto &o : keys)
      if (&o != &k)
        others.emplace_back(o.key);
    options.emplace_back(k.key, eastl::move(others));
  }

  int found = 0;
  CONSOLE_CHECK_NAME_WITH_OPTIONS("app", "debug_level_splines", 1, 6, options)
  {
    unsigned flags = 0;
    const char *name = nullptr;
    bool off = argc == 1 && debug_flags != 0;
    for (int i = 1; i < argc; i++)
    {
      if (dd_stricmp(argv[i], "off") == 0)
      {
        off = true;
        continue;
      }
      bool isKey = false;
      for (const auto &k : keys)
        if (dd_stricmp(argv[i], k.key) == 0)
        {
          flags |= k.flag;
          isKey = true;
        }
      if (!isKey)
        name = argv[i];
    }
    debug_flags = off ? 0 : (flags ? flags : ALL);
    debug_name = off ? "" : name;
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(level_splines_console_handler);
