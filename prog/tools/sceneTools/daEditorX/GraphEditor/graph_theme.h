// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>

// The canvas is a design surface, not editor chrome: it carries the design's own colors and must not
// follow the daEditorX light/dark theme. Every value here is what the surface renders under the dark
// theme, so a theme switch leaves the canvas and its status bar untouched.

// Opaque, unlike ne's default StyleColor_Bg (alpha 200) which let the themed window background
// through. == that default composited over dark.blk Color_WindowBg.
constexpr ImU32 GRAPH_CANVAS_BG_COLOR = IM_COL32(0x3A, 0x3A, 0x42, 0xFF);

constexpr ImU32 GRAPH_POPUP_BG_COLOR = IM_COL32(0x29, 0x2D, 0x33, 0xFF);   // == dark.blk Color_PopupBg
constexpr ImU32 GRAPH_POPUP_BORDER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 32); // == dark.blk Color_Border
constexpr ImU32 GRAPH_TEXT_COLOR = IM_COL32(0xE6, 0xE6, 0xE6, 0xFF);       // == dark.blk Color_Text

// Rows of the menus the canvas raises. The design draws their labels at pure white, and shortcuts,
// comments and the title row at 50% white -- not at the GRAPH_TEXT_COLOR the rest of the panel uses.
constexpr ImU32 GRAPH_MENU_LABEL_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr ImU32 GRAPH_MENU_SECONDARY_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 128);
constexpr ImU32 GRAPH_MENU_ROW_HOVERED_COLOR = IM_COL32(0x4D, 0x6B, 0x99, 0xFF); // == dark.blk Color_HeaderHovered
constexpr ImU32 GRAPH_MENU_ROW_SELECTED_COLOR = IM_COL32(0x70, 0x7B, 0x8C, 0xFF);
constexpr ImU32 GRAPH_MENU_ROW_SUBMENU_OPEN_COLOR = IM_COL32(0x66, 0x66, 0x66, 0xFF);

// A node's selection plate, and the picked row of the add-node popup.
constexpr ImU32 NODE_PLATE_FILL_COLOR = IM_COL32(0x59, 0x7D, 0xB3, 0xFF); // == dark.blk Color_FrameBgHovered

// Add-node search popup: darker than the menus above, and borderless, per the design.
constexpr ImU32 GRAPH_ADD_NODE_POPUP_BG_COLOR = IM_COL32(0x13, 0x13, 0x13, 0xFF);
// The field is a themed propPanel widget, so the canvas restates its colors or a light editor theme
// would put a light field on that near-black panel.
constexpr ImU32 GRAPH_SEARCH_FIELD_BG_COLOR = IM_COL32(0x4D, 0x4D, 0x4D, 0xFF);
constexpr ImU32 GRAPH_SEARCH_FIELD_BORDER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 26); // white at 0.10
constexpr ImU32 GRAPH_SEARCH_FIELD_FOCUS_COLOR = IM_COL32(0xBE, 0xD0, 0xF9, 0xFF);

constexpr ImU32 GRAPH_PENDING_LINK_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 33); // white at 0.13

constexpr ImU32 GRAPH_ERROR_BORDER_COLOR = IM_COL32(0xF6, 0x31, 0x31, 0xFF);

// The subgraph window (2848:513437). Its content takes the canvas color.
constexpr ImU32 SUBGRAPH_WINDOW_HEADER_COLOR = IM_COL32(0x21, 0x21, 0x21, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_FOOTER_COLOR = IM_COL32(0x47, 0x47, 0x47, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_BUTTON_COLOR = IM_COL32(0x66, 0x66, 0x66, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_BUTTON_HOVER_COLOR = IM_COL32(0xB3, 0xB3, 0xB3, 0xFF);  // == dark.blk Color_ButtonHovered
constexpr ImU32 SUBGRAPH_WINDOW_BUTTON_ACTIVE_COLOR = IM_COL32(0x4D, 0x4D, 0x4D, 0xFF); // == dark.blk Color_ButtonActive
constexpr ImU32 SUBGRAPH_WINDOW_BUTTON_BORDER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 26);   // white at 0.10
constexpr ImU32 SUBGRAPH_WINDOW_BORDER_COLOR = IM_COL32(0xB3, 0xB3, 0xB3, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_FOCUS_COLOR = IM_COL32(0xFF, 0xFF, 0x00, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_DIM_COLOR = IM_COL32(0xD9, 0xD9, 0xD9, 26);           // #D9D9D9 at 0.10
constexpr ImU32 SUBGRAPH_WINDOW_PARENT_CRUMB_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 179); // white at 0.70
constexpr ImU32 SUBGRAPH_WINDOW_CRUMB_HOVER_COLOR = IM_COL32(0x7F, 0xBF, 0xFF, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_CLOSE_HOVER_COLOR = IM_COL32(0xF6, 0x31, 0x31, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_GRIP_COLOR = IM_COL32(0x77, 0x77, 0x77, 0xFF);
constexpr ImU32 SUBGRAPH_WINDOW_RESIZE_HOT_COLOR = IM_COL32(0x59, 0x7D, 0xB3, 0xFF); // == dark.blk Color_ResizeGripHovered

// A minimized subgraph window's card, and the stack tile under the cards.
constexpr ImU32 SUBGRAPH_CARD_COLOR = IM_COL32(0x33, 0x33, 0x33, 0xFF);
constexpr ImU32 SUBGRAPH_CARD_HOVER_COLOR = IM_COL32(0x44, 0x44, 0x44, 0xFF);
constexpr ImU32 SUBGRAPH_CARD_BORDER_COLOR = IM_COL32(0xA4, 0xA4, 0xA4, 0xFF);
