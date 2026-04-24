#ifndef BURNT_GROUND_HLSLI_INCLUDED
#define BURNT_GROUND_HLSLI_INCLUDED

// Decal placement:
//  1) Animate decal fade-in in heightmap shader -> animated fade-in effect
//  2) Place clipmap decal -> appears without fade-in, but possibly with a small delay
//  3) Wait a bit to make sure clipmap decal is placed: wait `BURNT_GROUND_EXTRA_FADE_OUT_TIME_S` seconds
//  4) Do animation fade-out
//  5) Remove the animated decal
#define BURNT_GROUND_EXTRA_FADE_OUT_TIME_S 1.0

// This only applies to animated decals
// Decal instances are frustum culled, then collected into bins: each world XZ tile within a range has a bin.
// The renderer will iterate over every decal in the corresponding bin for every pixel.
#define BURNT_GROUND_TILE_SIZE 6.0
#define BURNT_GROUND_TILE_COUNT 16
#define BURNT_GROUND_VISIBILITY_RANGE (BURNT_GROUND_TILE_COUNT/2 * BURNT_GROUND_TILE_SIZE)

struct BurntGroundDecal
{
  float2 posXZ;
  float radius;
  float progress;
};

#endif