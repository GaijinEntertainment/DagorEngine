// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/integer/dag_IPoint2.h>


class BBox2;

namespace darg
{

extern bool require_font_size_slot;
extern void (*do_ui_blur)(const Tab<BBox2> &boxes);

} // namespace darg
