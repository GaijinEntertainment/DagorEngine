#define TORN_WOUND_SIZE 4     // cut record size in float4 (oriented ellipsoid: 3 rows + params)
#define HOLE_WOUND_SIZE 5    // hole record size in float4 (oriented ellipsoid: 3 rows + time + ease)
#define DECAL_WOUND_SIZE 5    // decal record size in float4

#define WOUND_REV(stride, slot) ((stride) - 1 - (slot))

// Hole record (wound_holes), CPU push order
#define WOUND_HOLE_ROW0 0 // r0.xyz, center.x
#define WOUND_HOLE_ROW1 1 // r1.xyz, center.y
#define WOUND_HOLE_ROW2 2 // r2.xyz, center.z
#define WOUND_HOLE_TIME 3 // spawnTime, holdDuration, closeDuration, (free)
#define WOUND_HOLE_EASE 4 // openDuration, pulseAmp, (free), (free)
//Cut record (broken_bones), CPU push order
#define WOUND_CUT_ROW0 0     // r0.xyz, center.x
#define WOUND_CUT_ROW1 1     // r1.xyz, center.y
#define WOUND_CUT_ROW2 2     // r2.xyz, center.z
#define WOUND_CUT_PARAMS 3   // startFadeFrom, trimVerticesTo, discardPixelTo, discardTexUvScale

// Decal record (projective_wounds), CPU push order
#define WOUND_DECAL_AXIS_X 0    // rotated cube axis X.xyz, (free)
#define WOUND_DECAL_AXIS_Y 1    // rotated cube axis Y.xyz, (free)
#define WOUND_DECAL_AXIS_Z 2    // rotated cube axis Z.xyz, (free)
#define WOUND_DECAL_DIR_TIME 3  // hitNormal.xyz, packed(startTime, texIndex)
#define WOUND_DECAL_POS_SCALE 4 // pos.xyz, scale
