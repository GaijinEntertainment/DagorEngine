// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

class GraphEditorPlg;

// The ghost that follows the cursor while a node-library drag is in flight. It has to sit over every
// panel, not just the canvas, so it draws on the hovered viewport's foreground list rather than from
// either of them.
class NodeDragOverlay
{
public:
  // Once per frame, after the panels: drop_target_hot is the canvas drop target's verdict from that same frame.
  void draw(GraphEditorPlg &plugin, bool drop_target_hot);

private:
  void resolveTemplate(GraphEditorPlg &plugin, const char *template_uid);

  // Resolved once per dragged template, then reused for as long as the payload stays put.
  eastl::string uid;
  eastl::string name;
  bool hasInputs = false;
  bool hasOutputs = false;
};
