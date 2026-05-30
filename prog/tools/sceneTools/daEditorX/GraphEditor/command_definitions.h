// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Command identifiers registered with the daEditorX EditorCommands registry
// (see GraphEditorPlg::registerEditorCommands). The id strings are also the
// keys used by the rebind UI and by de3_hotkeys.blk persistence.

inline constexpr const char *SHOW_GRAPH = "Plugin.GraphEditor.ShowGraph";
inline constexpr const char *SHOW_TEXTURE_PREVIEW = "Plugin.GraphEditor.ShowTexturePreview";
inline constexpr const char *SHOW_HISTOGRAM = "Plugin.GraphEditor.ShowHistogram";
inline constexpr const char *SHOW_LANDSCAPE_PREVIEW = "Plugin.GraphEditor.ShowLandscapePreview";
inline constexpr const char *SHOW_BASE_NODES = "Plugin.GraphEditor.ShowBaseNodes";
inline constexpr const char *SHOW_PROPERTIES = "Plugin.GraphEditor.ShowProperties";
inline constexpr const char *NEW_GRAPH = "Plugin.GraphEditor.NewGraph";
inline constexpr const char *LOAD_GRAPH = "Plugin.GraphEditor.LoadGraph";
inline constexpr const char *LOAD_GRAPH_BLK = "Plugin.GraphEditor.LoadGraphBlk";
inline constexpr const char *SAVE_GRAPH_BLK = "Plugin.GraphEditor.SaveGraphBlk";
inline constexpr const char *SAVE_AS_SUBGRAPH_BLK = "Plugin.GraphEditor.SaveAsSubgraphBlk";
inline constexpr const char *SAVE_TEXTURES = "Plugin.GraphEditor.SaveTextures";
inline constexpr const char *SAVE_TEXTURES_BY_SUBSTRING = "Plugin.GraphEditor.SaveTexturesBySubstring";
inline constexpr const char *RELOAD_TEXTURES = "Plugin.GraphEditor.ReloadTextures";
inline constexpr const char *FORCE_REBUILD = "Plugin.GraphEditor.ForceRebuild";

// Canvas-local shortcuts. Dispatched by graph_panel.cpp via ImGui::Shortcut with
// RouteFocused; imgui-node-editor's matching built-ins are suppressed at editor
// creation (see disable_node_editor_shortcuts).
inline constexpr const char *CANVAS_DELETE_SELECTED = "Plugin.GraphEditor.Canvas.DeleteSelected";
inline constexpr const char *CANVAS_FRAME_SELECTED = "Plugin.GraphEditor.Canvas.FrameSelected";
inline constexpr const char *CANVAS_FRAME_SELECTED_WITH_MARGIN = "Plugin.GraphEditor.Canvas.FrameSelectedWithMargin";
inline constexpr const char *CANVAS_COPY = "Plugin.GraphEditor.Canvas.Copy";
inline constexpr const char *CANVAS_CUT = "Plugin.GraphEditor.Canvas.Cut";
inline constexpr const char *CANVAS_PASTE = "Plugin.GraphEditor.Canvas.Paste";
