// Iterates visible lights of one type from the per-tile z-binned light lists,
// invoking the light body for each light index. Helper lanes skip the walk,
// so the light body must not need derivatives.
//
// Usage:
//   #define TILED_LIGHTS_WALK_SPOT 0|1           // light type: omni (0) or spot (1)
//   #define TILED_LIGHTS_WALK_DEPTH w            // linear view depth of the shaded point
//   #define TILED_LIGHTS_WALK_TILE_OFFSET expr   // first dword of this tile in the lights list
//   #define TILED_LIGHTS_WALK_LIGHT_BODY(light_index) ... // per-light statements; `continue` skips to the next light
//   #define TILED_LIGHTS_WALK_SINGLE_WORD 0|1    // optional: single-light shader variant, collapses the word loop
//   #define TILED_LIGHTS_WALK_BUFFER_AT(b, i)    // optional: buffer accessor, defaults to b[i]
//   #include <tiled_lights_walk.hlsli>
//
// All TILED_LIGHTS_WALK_* parameters are consumed and undefined by this include.

#include <tiled_lights_walk_body.hlsli>

#ifndef TILED_LIGHTS_WALK_SINGLE_WORD
#define TILED_LIGHTS_WALK_SINGLE_WORD 0
#endif
#ifndef TILED_LIGHTS_WALK_BUFFER_AT
#define TILED_LIGHTS_WALK_BUFFER_AT(buffer, index) buffer[index]
#endif

TILED_LIGHTS_WALK_BODY_IMPL(
    TILED_LIGHTS_WALK_SPOT,
    TILED_LIGHTS_WALK_SINGLE_WORD,
    TILED_LIGHTS_WALK_TILE_OFFSET,
    TILED_LIGHTS_WALK_DEPTH)

#undef TILED_LIGHTS_WALK_LIGHT_BODY
#undef TILED_LIGHTS_WALK_BUFFER_AT
#undef TILED_LIGHTS_WALK_SINGLE_WORD
#undef TILED_LIGHTS_WALK_SPOT
#undef TILED_LIGHTS_WALK_DEPTH
#undef TILED_LIGHTS_WALK_TILE_OFFSET
