#pragma once

#include <imgui/imgui.h>
#include <daScript/simulate/debug_info.h>

namespace das
{
template <> struct isCloneable<ImGuiListClipper> : false_type {};
template <> struct isCloneable<ImDrawListSplitter> : false_type {};
template <> struct isCloneable<ImDrawList> : false_type {};
template <> struct isCloneable<ImFontAtlas> : false_type {};
template <> struct isCloneable<ImFont> : false_type {};
template <> struct isCloneable<ImGuiViewport> : false_type {};
}
