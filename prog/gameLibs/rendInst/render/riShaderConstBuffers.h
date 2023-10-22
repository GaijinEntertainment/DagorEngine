#pragma once

#include <3d/dag_drv3d.h>


// This is rendinst library private, don't expose it to outside world
namespace rendinst::render
{

enum : int
{
  MIN_INST_COUNT = 16,
  INST_BINS = 8,
  MAX_INSTANCES = MIN_INST_COUNT << (INST_BINS - 1)
};

extern Sbuffer *perDrawCB;

void init_instances_tb();
void close_instances_tb();

} // namespace rendinst::render
