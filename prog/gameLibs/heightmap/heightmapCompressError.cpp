// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
void report_heightmap_error(uint32_t bl, uint16_t max_err, double mse, float scale)
{
  debug("compressed heightmap to blocks of %d max error %d(%g m) MSE=%g(%g m)", bl, max_err, max_err * scale, mse, mse * scale);
}
