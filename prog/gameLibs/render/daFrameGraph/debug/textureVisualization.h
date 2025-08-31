// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>
#include <EASTL/span.h>
#include <imgui.h>
#include <memory/dag_framemem.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>


namespace dafg
{
struct InternalRegistry;
struct DependencyData;
class NodeHandle;
} // namespace dafg

bool deselect_button(const char *label);
void overlay_checkbox(const char *label);

void fg_texture_visualization_imgui_line(const dafg::InternalRegistry &registry);

dag::Vector<dafg::NodeNameId, framemem_allocator> filter_out_debug_node(eastl::span<const dafg::NodeNameId> exec_order,
  const dafg::InternalRegistry &registry);

void close_visualization_texture();
void clear_visualization_node();

struct PreciseTimePoint
{
  dafg::NodeNameId precedingNode;
  dafg::NodeNameId followingNode;
};
struct ReadTimePoint
{};
using TimePoint = eastl::variant<PreciseTimePoint, ReadTimePoint>;

struct Selection
{
  TimePoint when;
  dafg::ResNameId what;
};
void update_fg_debug_tex(const eastl::optional<Selection> &selection, dafg::InternalRegistry &registry);
void update_external_texture_visualization(dafg::InternalRegistry &registry, const dafg::DependencyData &dep_data);
void set_manual_showtex_params(const char *params);
