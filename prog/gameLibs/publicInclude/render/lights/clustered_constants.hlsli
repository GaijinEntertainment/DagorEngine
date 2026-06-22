#define CLUSTERS_W 32
#define CLUSTERS_H 16
#define CLUSTERS_D 24

//some shaders use calculations when sliceId equals CLUSTERS_D. so it's convenient to have empty CLUSTERS_D-th cluster
#define CLUSTERS_PER_GRID (CLUSTERS_W * CLUSTERS_H * (CLUSTERS_D + 1))