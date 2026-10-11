#ifndef LAND_DETILE_INCLUDED
#define LAND_DETILE_INCLUDED 1

#define LAND_DETILE_MAX_CLUSTERS 8
#define LAND_DETILE_HARDNESS     0.2f // each side of a cluster transition that shows one cluster with no blend
// R2 low discrepancy: cluster k takes offset frac(k * R2) of the tile
#define LAND_DETILE_R2_X         0.7548776662f
#define LAND_DETILE_R2_Y         0.5698402910f

// land_detile_params_buf: one record per landclass, indexed by the landclass id
#define LAND_DETILE_PARAMS_STRIDE   16 // bytes
#define LAND_DETILE_PARAMS_CLUSTERS 0  // uint, 0 = off
#define LAND_DETILE_PARAMS_ANGLE    4  // float, radians
#define LAND_DETILE_PARAMS_SPREAD   8  // float, radians
#define LAND_DETILE_PARAMS_SIZE     12 // float, metres

#endif
