// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class SqModules;

namespace loading_ui
{

bool is_fully_covering();      // ui fully redraw each pixel of the screen, no need to render anything
void set_fully_covering(bool); // ui fully redraw each pixel of the screen, no need to render anything

void bind(SqModules *moduleMgr);

} // namespace loading_ui
