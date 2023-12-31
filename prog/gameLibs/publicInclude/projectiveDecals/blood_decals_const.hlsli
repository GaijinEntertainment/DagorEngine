#ifndef BLOOD_DECALS_CONST_INCLUDED
#define BLOOD_DECALS_CONST_INCLUDED 1

#define INSIDE_CHECK_EPSILON 0.05f

#define MAX_SPLASH_DISTANCE 10.0f
#define BLOOD_SPEED 40.0f

#define BLOOD_DIFFUSE_NOISE_SCALE 0.8f
#define BLOOD_DIFFUSE_NOISE_FREQUENCY_SCALE 0.8f

#define DECAL_ATLAS_ROWS 2
#define DECAL_ATLAS_COLUMNS 2

#define ANGLE_FADE_RANGE 0.25f
#define ANGLE_FADE_OFFSET 0.02f

#define Z_FADE_RANGE 0.1f

#define DEPTH_ATLAS_SIZE 256
#define DEPTH_TEX_SIZE 4
#define DEPTH_CMP_SCALE 10.0f
#define DEPTH_BIAS 0.5f


#define ORTHO_MAP_ANGLE_COS 0.4f
#define ORTHO_MAP_Z_FADE 0.45f

#define ORTHO_MAP_NOISE_SIZE 0.7f
#define ORTHO_MAP_NOISE_INTENSITY 10.f

#define TRACE_RESULT_BUFFER_SIZE (DEPTH_TEX_SIZE*DEPTH_TEX_SIZE)

#endif