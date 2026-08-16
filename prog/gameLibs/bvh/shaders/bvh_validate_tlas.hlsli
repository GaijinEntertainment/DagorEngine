#ifndef BVH_VALIDATE_TLAS_HLSLI_INCLUDED
#define BVH_VALIDATE_TLAS_HLSLI_INCLUDED 1

#define BVH_VALIDATE_DEAD_BLAS 1
#define BVH_VALIDATE_BAD_TRANSFORM 2
#define BVH_VALIDATE_POOLED_BLAS 3
#define BVH_VALIDATE_STALE_TAIL 4

// Report buffer layout: a dword count, then one tail counter per region, then fixed size records.
// sequence is the writer's slot index plus one, so a torn readback can be told apart from a record;
// regionIndex is only known to the tail check, the others write ~0u and let the CPU derive the region
// from the slot. A stale tail spans arbitrarily many slots, so the tail check only counts them here and
// writes a single record per region.
#define BVH_VALIDATE_MAX_REGIONS 12
#define BVH_VALIDATE_REGION_COUNTERS 4
#define BVH_VALIDATE_REPORT_HEADER_DWORDS (BVH_VALIDATE_REGION_COUNTERS + BVH_VALIDATE_MAX_REGIONS)
#define BVH_VALIDATE_REPORT_DWORDS 8

#define BVH_VALIDATE_REPORT_INSTANCE 0
#define BVH_VALIDATE_REPORT_KIND 1
#define BVH_VALIDATE_REPORT_BLAS_LO 2
#define BVH_VALIDATE_REPORT_BLAS_HI 3
#define BVH_VALIDATE_REPORT_INSTANCE_ID_MASK 4
#define BVH_VALIDATE_REPORT_REGION 5
#define BVH_VALIDATE_REPORT_SEQUENCE 6
#define BVH_VALIDATE_REPORT_UNUSED 7

#define BVH_VALIDATE_NO_REGION 0xFFFFFFFFU

#endif
