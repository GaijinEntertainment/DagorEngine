#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/string.h>
#include <imgui.h>
#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/resNameId.h>


namespace dabfg
{
struct InternalRegistry;
class NodeHandle;
} // namespace dabfg

void fg_texture_visualization_imgui_line(dabfg::ResNameId &selectedResId, const dabfg::InternalRegistry &registry,
  dabfg::NodeNameId selectedPrecedingNode);

void update_fg_debug_tex(dabfg::NodeNameId selectedPrecedingNode, dabfg::NodeNameId *selectedFollowingNode,
  dabfg::ResNameId selectedResId, UniqueTex &copiedTexture, dabfg::InternalRegistry &registry,
  dabfg::NodeHandle &debugTextureCopyNode);
