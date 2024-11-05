// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <squirrel.h>

namespace darg
{
struct IGuiScene;
}

class TMatrix4;
class TMatrix;

namespace user_ui
{
void init();
void before_render();
void term();
void reload_user_ui_script(bool full_reinit);

bool is_fully_covering();      // ui fully redraw each pixel of the screen, no need to render anything
void set_fully_covering(bool); // ui fully redraw each pixel of the screen, no need to render anything

void rebind_das();

HSQUIRRELVM get_vm();

extern eastl::unique_ptr<darg::IGuiScene> gui_scene;
} // namespace user_ui
