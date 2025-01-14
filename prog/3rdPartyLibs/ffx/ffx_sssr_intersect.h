// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define M_PI                               3.14159265358979f
#define FFX_SSSR_FLOAT_MAX                 3.402823466e+38
#define FFX_SSSR_DEPTH_HIERARCHY_MAX_MIP   6


void FFX_SSSR_InitialAdvanceRay(FfxFloat32x3 origin, FfxFloat32x3 direction, FfxFloat32x3 inv_direction, FfxFloat32x2 current_mip_resolution, FfxFloat32x2 current_mip_resolution_inv, FfxFloat32x2 floor_offset, FfxFloat32x2 uv_offset, FFX_PARAMETER_OUT FfxFloat32x3 position, FFX_PARAMETER_OUT FfxFloat32 current_t) {
    FfxFloat32x2 current_mip_position = current_mip_resolution * origin.xy;

    // Intersect ray with the half box that is pointing away from the ray origin.
    FfxFloat32x2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;

    // o + d * t = p' => t = (p' - o) / d
    FfxFloat32x2 t = xy_plane * inv_direction.xy - origin.xy * inv_direction.xy;
    current_t = min(t.x, t.y);
    position = origin + current_t * direction;
}

FfxBoolean FFX_SSSR_AdvanceRay(FfxFloat32x3 origin, FfxFloat32x3 direction, FfxFloat32x3 inv_direction, FfxFloat32x2 current_mip_position, FfxFloat32x2 current_mip_resolution_inv, FfxFloat32x2 floor_offset, FfxFloat32x2 uv_offset, FfxFloat32 surface_z, FFX_PARAMETER_INOUT FfxFloat32x3 position, bool hit_validation, FFX_PARAMETER_INOUT FfxFloat32 current_t, FFX_PARAMETER_INOUT FfxFloat32x4 back_surface_begin_pos) {
     // Skip tile if view space depth is below the finest surface thickness
    bool below_surface = hit_validation && FFX_BiasedComparisonOfRawDepth(position.z, surface_z);

    if (back_surface_begin_pos.w == 0 && below_surface)
        back_surface_begin_pos = FfxFloat32x4(position, 1);

    // Create boundary planes
    FfxFloat32x2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;
    FfxFloat32x3 boundary_planes = FfxFloat32x3(xy_plane, surface_z);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    FfxFloat32x3 t = boundary_planes * inv_direction - origin * inv_direction;

    // Prevent using z plane when shooting out of the depth buffer.
#if FFX_SSSR_OPTION_INVERTED_DEPTH
    t.z = direction.z < 0 ? t.z : FFX_SSSR_FLOAT_MAX;
#else
    t.z = direction.z > 0 ? t.z : FFX_SSSR_FLOAT_MAX;
#endif

    // Choose nearest intersection with a boundary.
    FfxFloat32 t_min = min(t.x, t.y);
    t_min = below_surface ? t_min : min(t.z, t_min);

#if FFX_SSSR_OPTION_INVERTED_DEPTH
    // Larger z means closer to the camera.
    FfxBoolean above_surface = surface_z < position.z;
#else
    // Smaller z means closer to the camera.
    FfxBoolean above_surface = surface_z > position.z;
#endif

    FfxBoolean surface_check = below_surface || above_surface;

    // Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
    // We use the asuint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if t_min is the t.z we fed into the min3 above.
    FfxBoolean skipped_tile = ffxAsUInt32(t_min) != ffxAsUInt32(t.z) && surface_check;

    // Make sure to only advance the ray if we're still above the surface.
    current_t = surface_check ? t_min : current_t;

    // Advance ray
    position = origin + current_t * direction;

    return skipped_tile;
}

FfxFloat32x2 FFX_SSSR_GetMipResolution(FfxFloat32x2 screen_dimensions, FfxInt32 mip_level) {
    return screen_dimensions * pow(0.5, mip_level);
}

bool is_ray_inside_borders(float3 position)
{
    return all(position >= 0) && all(position < 1);
}

struct FFX_RaymarchResult
{
    FfxFloat32x4 hitUVZw; // Hit UV, Z, W - confidence 1 - hit 100% valid.
    FfxFloat32x4 backSurfaceBeginUVZw; // Starting point of back surface.
    FfxFloat32x3 backSurfaceEndUVZ; // EndPoint of ray marching
};

// Requires origin and direction of the ray to be in screen space [0, 1] x [0, 1]
FFX_RaymarchResult FFX_SSSR_HierarchicalRaymarch(FfxFloat32x3 origin, FfxFloat32x3 direction, FfxBoolean is_mirror, FfxFloat32x2 screen_size, FfxInt32 most_detailed_mip, FfxUInt32 min_traversal_occupancy, FfxUInt32 max_traversal_intersections)
{
    const FfxFloat32x3 inv_direction = FFX_SELECT(direction != FfxFloat32x3(0.0f, 0.0f, 0.0f), FfxFloat32x3(1.0f, 1.0f, 1.0f) / direction, FfxFloat32x3(FFX_SSSR_FLOAT_MAX, FFX_SSSR_FLOAT_MAX, FFX_SSSR_FLOAT_MAX));

    // Start on mip with highest detail.
    FfxInt32 current_mip = most_detailed_mip;

    // Could recompute these every iteration, but it's faster to hoist them out and update them.
    FfxFloat32x2 current_mip_resolution = FFX_SSSR_GetMipResolution(screen_size, current_mip);
    FfxFloat32x2 current_mip_resolution_inv = ffxReciprocal(current_mip_resolution);

    // Offset to the bounding boxes uv space to intersect the ray with the center of the next pixel.
    // This means we ever so slightly over shoot into the next region.
    FfxFloat32x2 uv_offset = 0.005 * exp2(most_detailed_mip) / screen_size;
    uv_offset.x = direction.x < 0.0f ? -uv_offset.x : uv_offset.x;
    uv_offset.y = direction.y < 0.0f ? -uv_offset.y : uv_offset.y;

    // Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
    FfxFloat32x2 floor_offset;
    floor_offset.x = direction.x < 0.0f ? 0.0f : 1.0f;
    floor_offset.y = direction.y < 0.0f ? 0.0f : 1.0f;

    // Initially advance ray to avoid immediate self intersections.
    FfxFloat32 current_t;
    FfxFloat32x3 position;
    FFX_SSSR_InitialAdvanceRay(origin, direction, inv_direction, current_mip_resolution, current_mip_resolution_inv, floor_offset, uv_offset, position, current_t);

    {
        float surface_z = FFX_SSSR_LoadDepth(int2(screen_size * position.xy), 0).r;
        // Check if we was above surface on origin and then go under, then we intersect surface and we could not trace at all.
        // This is could be case in ssr water tracing, since origin point always located on water surface and not downsampled depth tex.
        if (!FFX_BiasedComparisonOfRawDepth(origin.z, surface_z) &&
            FFX_BiasedComparisonOfRawDepth(position.z, surface_z))
          current_mip = -1;
    }

    FfxFloat32x4 back_surface_begin_pos = 0;

    FfxBoolean exit_due_to_low_occupancy = false;
    FfxInt32 i = 0;
    while (i < max_traversal_intersections && current_mip >= most_detailed_mip && !exit_due_to_low_occupancy &&
           is_ray_inside_borders(position))
    {
        FfxFloat32x2 current_mip_position = current_mip_resolution * position.xy;
        FfxInt32x2 samplePos = FfxInt32x2(min(current_mip_position, current_mip_resolution - 1.0f));
        FfxFloat32 surface_z = FFX_SSSR_LoadDepth(samplePos, current_mip);

        #if FFW_WAVE
        exit_due_to_low_occupancy = !is_mirror && ffxWaveActiveCountBits(true) <= min_traversal_occupancy;
        #endif

        FfxBoolean skipped_tile = FFX_SSSR_AdvanceRay(origin, direction, inv_direction, current_mip_position, current_mip_resolution_inv, floor_offset, uv_offset, surface_z, position, current_mip == most_detailed_mip, current_t, back_surface_begin_pos);

        #ifdef FFX_SSSR_OPTION_INVERTED_DEPTH
        if ((current_mip_position.x >= floor(current_mip_resolution.x)) ||
            (current_mip_position.y >= floor(current_mip_resolution.y)) ||
            (current_mip >= downsampled_depth_mip_count))
            surface_z = 1;
        #endif

        // Don't increase mip further than this because we did not generate it
        FfxBoolean nextMipIsOutOfRange = skipped_tile && (current_mip >= downsampled_depth_mip_count-1);
        if (!nextMipIsOutOfRange)
        {
            current_mip += skipped_tile ? 1 : -1;
            current_mip_resolution *= skipped_tile ? 0.5 : 2;
            current_mip_resolution_inv *= skipped_tile ? 2 : 0.5;;
        }

        ++i;
    }

    bool isRayInsideBorders = is_ray_inside_borders(position);
    int2 resTC = clamp(int2(screen_size * position.xy), int2(0,0), screen_size-1);
    FfxFloat32 surface_z = texelFetch(downsampled_close_depth_tex, resTC, 0).r;

    bool valid_hit = isRayInsideBorders && (i < max_traversal_intersections) && (current_mip < most_detailed_mip) && (surface_z > 0);

    FFX_RaymarchResult result;
    result.backSurfaceEndUVZ = position.xyz;
    result.hitUVZw = float4(float3(position.xy, surface_z), valid_hit);

    if (back_surface_begin_pos.w > 0)
    {
      // Reject backface tracing hit if we didnt advance the ray significantly to avoid immediate self reflection
      float2 back_surface_manhattan_dist = abs(position.xy - back_surface_begin_pos.xy);
      if ((back_surface_manhattan_dist.x < (2.0f / screen_size.x)) && (back_surface_manhattan_dist.y < (2.0f / screen_size.y)))
        back_surface_begin_pos.w = 0;
    }

    result.backSurfaceBeginUVZw = back_surface_begin_pos;
    return result;
}