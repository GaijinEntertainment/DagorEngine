// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "input/inputControls.h"
#include "input/globInput.h"
#include "input/uiInput.h"
#include "phys/netPhys.h"
#include <ecs/core/entitySystem.h>
#include <ecs/input/debugInputEvents.h>

ECS_DEF_PULL_VAR(input);
size_t framework_input_pulls = 0;

void init_glob_input() {}
void destroy_glob_input() {}
void pull_input_das() {}
bool have_glob_input() { return false; }

namespace controls
{
void init_drivers() {}
void init_control(const char *, const char *) {}
void process_input(double) {}
void dump() {}
bool is_visual_dump_shown() { return false; }
void show_visual_dump(bool) {}
void destroy() {}
void global_init() {}
void global_destroy() {}
} // namespace controls

namespace debuginputevents
{
void init() {}
void close() {}
} // namespace debuginputevents

#include <daInput/input_api.h>
namespace dainput
{
unsigned get_last_used_device_mask(unsigned) { G_ASSERT_RETURN(false, 0); }
} // namespace dainput
