// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include "const3d.h"

dag::ConstSpan<uint32_t> transcode_vertex_shader(dag::ConstSpan<uint32_t> vpr);
dag::ConstSpan<uint32_t> transcode_pixel_shader(dag::ConstSpan<uint32_t> fsh);
dag::ConstSpan<int32_t> transcode_stcode(dag::ConstSpan<int32_t> stcode);

dag::ConstSpan<int> process_stblkcode(dag::ConstSpan<int> stcode, bool sh_blk);
