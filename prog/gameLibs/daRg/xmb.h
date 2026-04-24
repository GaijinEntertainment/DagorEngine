// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_bounds2.h>
#include <daRg/dag_guiConstants.h>


namespace darg
{

class Element;

namespace xmb
{

void rebuild_xmb(Element *node, Element *xmb_parent);

BBox2 calc_xmb_viewport(Element *node, Element **nearest_xmb_viewport);
bool is_xmb_node_input_covered(Element *node);

// Navigate a regular grid XMB structure (defineed by isGridLine=true on rows/columns)
Element *xmb_grid_navigate(Element *cur_focus, Direction dir);
// Walk to the next/previous sibling in the XMB structure
Element *xmb_siblings_navigate(Element *cur_focus, Direction dir);
// Navigate in screen space, finding the nearest element in the given direction
Element *xmb_screen_space_navigate(Element *cur_focus, Direction dir);

} // namespace xmb

} // namespace darg
