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
using all_darg_scenes_t = dag::RelocatableFixedVector<darg::IGuiScene *, 1, false>;

all_darg_scenes_t get_all_scenes();

void update_all_gui_scenes_mainthread(float dt);
void before_render(float dt, const TMatrix &view_itm, const TMatrix &view_tm, const TMatrix4 &proj_tm);
void start_ui_render_job(bool wake);
void prepare_to_start_ui_before_render_job();
void start_ui_before_render_job(bool wake = true);
void skip_ui_render_job();
void wait_ui_render_job_done();
void wait_ui_before_render_job_done();

void init();

extern bool multithreaded;
extern bool beforeRenderMultithreaded;
} // namespace uirender
