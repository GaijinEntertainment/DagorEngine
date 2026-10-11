// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debugMultiTextLabels/debugMultiTextLabels_api.h>
#include <render/debugMultiTextOverlay.h>
#include <debug/dag_textMarks.h>
#include <hudprim/dag_hudPrimitives.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_resetDevice.h>
#include <EASTL/string_view.h>
#include <EASTL/sort.h>
#include <EASTL/unique_ptr.h>

static eastl::unique_ptr<HudPrimitives> labels_hud;

void render_debug_multitext_labels(const TMatrix4 &globtm)
{
  dag::ConstSpan<DebugMultiTextLabel> frameLabels = get_debug_multitext_labels();
  if (frameLabels.empty())
    return;

  if (!labels_hud)
  {
    labels_hud = eastl::make_unique<HudPrimitives>();
    labels_hud->createGuiBuffer(4096, 512);
    labels_hud->createHudBuffer(4096, 512);
  }
  labels_hud->resetFrame();
  labels_hud->updateTarget();

  dag::Vector<DebugMultiTextLabel, framemem_allocator> labels(frameLabels.begin(), frameLabels.end());
  eastl::sort(labels.begin(), labels.end(),
    [](const DebugMultiTextLabel &a, const DebugMultiTextLabel &b) { return a.accentColor.u < b.accentColor.u; });

  dag::Vector<Point3, framemem_allocator> positions;
  dag::Vector<eastl::string_view, framemem_allocator> names;
  positions.reserve(labels.size());
  names.reserve(labels.size());

  for (size_t groupBegin = 0, groupEnd = 0; groupBegin < labels.size(); groupBegin = groupEnd)
  {
    positions.clear();
    names.clear();
    for (groupEnd = groupBegin; groupEnd < labels.size(); ++groupEnd)
    {
      const DebugMultiTextLabel &label = labels[groupEnd];
      if (label.accentColor.u != labels[groupBegin].accentColor.u)
        break;
      positions.push_back(label.pos);
      names.emplace_back(label.name);
    }

    DebugMultiTextOverlay cfg;
    cfg.borderColor = cfg.lineColor = labels[groupBegin].accentColor.u;
    draw_debug_multitext_overlay_simple(make_span_const(positions.data(), positions.size()),
      make_span_const(names.data(), names.size()), labels_hud.get(), globtm, cfg);
  }

  discard_debug_multitext_labels();
}

void shutdown_debug_multitext_labels() { labels_hud.reset(); }

static void debug_multitext_labels_after_reset_device(bool) { labels_hud.reset(); }

REGISTER_D3D_AFTER_RESET_FUNC(debug_multitext_labels_after_reset_device);
