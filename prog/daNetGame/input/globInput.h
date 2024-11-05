// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace HumanInput
{
class IGenJoystick;
}

void init_glob_input();
void destroy_glob_input();
bool have_glob_input();

bool glob_input_process_top_level_key(bool down, int key_idx, unsigned keyModif);
bool glob_input_process_controller(const HumanInput::IGenJoystick *joy);
bool glob_input_gpu_profiler_handle_F11_key(bool alt_modif);
