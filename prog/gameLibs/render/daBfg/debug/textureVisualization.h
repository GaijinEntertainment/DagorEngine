// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>
#include <EASTL/span.h>
#include <imgui.h>
#include <memory/dag_framemem.h>
#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/resNameId.h>


namespace dabfg
{
struct InternalRegistry;
class NodeHandle;
} // namespace dabfg

bool deselect_button(const char *label);

void fg_texture_visualization_imgui_line(const dabfg::InternalRegistry &registry);

dag::Vector<dabfg::NodeNameId, framemem_allocator> filter_out_debug_node(eastl::span<const dabfg::NodeNameId> exec_order,
  const dabfg::InternalRegistry &registry);

void close_visualization_texture();
void clear_visualization_node();

struct PreciseTimePoint
{
  dabfg::NodeNameId precedingNode;
  dabfg::NodeNameId followingNode;
};
struct ReadTimePoint
{};
using TimePoint = eastl::variant<PreciseTimePoint, ReadTimePoint>;

struct Selection
{
  TimePoint when;
  dabfg::ResNameId what;
};
void update_fg_debug_tex(const eastl::optional<Selection> &selection, dabfg::InternalRegistry &registry);
void set_manual_showtex_params(const char *params);
