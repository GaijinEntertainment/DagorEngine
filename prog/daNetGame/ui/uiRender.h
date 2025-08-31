// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>

namespace darg
{
struct IGuiScene;
}

class TMatrix;
class TMatrix4;

namespace uirender
{
using all_darg_scenes_t = dag::RelocatableFixedVector<darg::IGuiScene *, 2, false>;

all_darg_scenes_t get_all_scenes();
bool has_scenes();

void update_all_gui_scenes_mainthread(float dt);
void before_render(float dt, const TMatrix &view_itm, const TMatrix &view_tm, const TMatrix4 &proj_tm);
void start_ui_render_job(bool wake);
void start_ui_before_render_job();
void skip_ui_render_job();
void wait_ui_render_job_done();
void wait_ui_before_render_job_done();

void init();

extern bool multithreaded;
extern bool beforeRenderMultithreaded;
} // namespace uirender
