// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <stdint.h>

// Critical path extraction from SDF distance field.
//
// Given a distance-to-shore field, extracts the critical water connectivity
// path: contour boundary + ridge centerlines that lack line-of-sight to
// the contour through water.
//
// Pipeline: detect ridges -> thin 2x2 blocks -> prune branches ->
// remove small components -> filter by LOS to contour -> combine with contour.
//
// dist            - distance field, w*w floats
// water_mask      - packed bitmask, bit=1 for water pixels
// w               - grid width (square grid)
// contour_dist    - distance from shore for the contour (e.g. 64.0)
// min_branch_len  - branches shorter than this are pruned (e.g. 50), 0 to disable
// min_comp_size   - connected components smaller than this are removed (e.g. 30), 0 to disable
// out_path        - output bitmask, will be resized and zeroed, ceil(w*w/32) words
//
// Returns number of critical path pixels.
int hmap_sdf_extract_critical_path(dag::ConstSpan<float> dist, dag::ConstSpan<uint32_t> water_mask, int w, float contour_dist,
  int min_branch_len, int min_comp_size, dag::Vector<uint32_t> &out_path);
