// Iterates lights of one type from a frustum-cluster bit grid, invoking the
// light body for each light index.
//
// Usage:
//   #define CLUSTER_GRID_WALK_ADDRESS expr        // first word of this cluster's bit list
//   #define CLUSTER_GRID_WALK_WORD_COUNT expr     // words to read starting at the address (the loop bound)
//   #define CLUSTER_GRID_WALK_LIGHT_BODY(light_index) ... // per-light statements; `continue` skips to the next light
//   #define CLUSTER_GRID_WALK_MERGE_MASK(m)       // optional: wave-merge override, pass (m) where wave ops are unavailable (VS)
//   #define CLUSTER_GRID_WALK_WORD_LOOP_ATTR      // optional: attribute for the word loop (LOOP / UNROLL), defaults to none
//   #define CLUSTER_GRID_WALK_ACTUAL_WORDS_COUNT expr // optional: words actually in the grid, when WORD_COUNT can exceed it; the rest read as 0
//   #include <cluster_grid_lights_walk.hlsli>
//
// All CLUSTER_GRID_WALK_* parameters are consumed and undefined by this include.

#ifndef CLUSTER_GRID_WALK_MERGE_MASK
#define CLUSTER_GRID_WALK_MERGE_MASK(m) MERGE_MASK(m)
#endif
#ifndef CLUSTER_GRID_WALK_WORD_LOOP_ATTR
#define CLUSTER_GRID_WALK_WORD_LOOP_ATTR
#endif

{
  CLUSTER_GRID_WALK_WORD_LOOP_ATTR
  for (uint grid_walk_word = 0; grid_walk_word < (CLUSTER_GRID_WALK_WORD_COUNT); ++grid_walk_word)
  {
    // Load bit mask data per lane
    #if defined (CLUSTER_GRID_WALK_ACTUAL_WORDS_COUNT)
      bool grid_walk_word_valid = grid_walk_word < (CLUSTER_GRID_WALK_ACTUAL_WORDS_COUNT);
      uint grid_walk_mask = flatBitArray[grid_walk_word_valid ? (CLUSTER_GRID_WALK_ADDRESS) + grid_walk_word : 0];
      grid_walk_mask = grid_walk_word_valid ? grid_walk_mask : 0;
    #else
      uint grid_walk_mask = flatBitArray[(CLUSTER_GRID_WALK_ADDRESS) + grid_walk_word];
    #endif
    uint grid_walk_mergedMask = CLUSTER_GRID_WALK_MERGE_MASK(grid_walk_mask);
    while (grid_walk_mergedMask != 0) // processed per lane
    {
      uint grid_walk_bitIndex = firstbitlow(grid_walk_mergedMask);
      grid_walk_mergedMask ^= (1U << grid_walk_bitIndex);
      uint grid_walk_lightIndex = ((grid_walk_word << 5) + grid_walk_bitIndex);
      CLUSTER_GRID_WALK_LIGHT_BODY(grid_walk_lightIndex)
    }
  }
}

#undef CLUSTER_GRID_WALK_LIGHT_BODY
#undef CLUSTER_GRID_WALK_MERGE_MASK
#undef CLUSTER_GRID_WALK_WORD_LOOP_ATTR
#undef CLUSTER_GRID_WALK_ADDRESS
#undef CLUSTER_GRID_WALK_WORD_COUNT
#undef CLUSTER_GRID_WALK_ACTUAL_WORDS_COUNT
