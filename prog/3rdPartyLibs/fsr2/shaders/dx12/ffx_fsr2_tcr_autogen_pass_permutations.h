#include "ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934.h"
#include "ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e.h"

typedef union ffx_fsr2_tcr_autogen_pass_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_tcr_autogen_pass_PermutationKey;

typedef struct ffx_fsr2_tcr_autogen_pass_PermutationInfo {
    const uint32_t       blobSize;
    const unsigned char* blobData;


    const uint32_t  numCBVResources;
    const char**    cbvResourceNames;
    const uint32_t* cbvResourceBindings;
    const uint32_t* cbvResourceCounts;
    const uint32_t* cbvResourceSpaces;

    const uint32_t  numSRVResources;
    const char**    srvResourceNames;
    const uint32_t* srvResourceBindings;
    const uint32_t* srvResourceCounts;
    const uint32_t* srvResourceSpaces;

    const uint32_t  numUAVResources;
    const char**    uavResourceNames;
    const uint32_t* uavResourceBindings;
    const uint32_t* uavResourceCounts;
    const uint32_t* uavResourceSpaces;

    const uint32_t  numSamplerResources;
    const char**    samplerResourceNames;
    const uint32_t* samplerResourceBindings;
    const uint32_t* samplerResourceCounts;
    const uint32_t* samplerResourceSpaces;

    const uint32_t  numRTAccelerationStructureResources;
    const char**    rtAccelerationStructureResourceNames;
    const uint32_t* rtAccelerationStructureResourceBindings;
    const uint32_t* rtAccelerationStructureResourceCounts;
    const uint32_t* rtAccelerationStructureResourceSpaces;
} ffx_fsr2_tcr_autogen_pass_PermutationInfo;

static const uint32_t g_ffx_fsr2_tcr_autogen_pass_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
};

static const ffx_fsr2_tcr_autogen_pass_PermutationInfo g_ffx_fsr2_tcr_autogen_pass_PermutationInfo[] = {
    { g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_size, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_data, 2, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_CBVResourceNames, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_CBVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_CBVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_CBVResourceSpaces, 7, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_SRVResourceNames, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_SRVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_SRVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_SRVResourceSpaces, 4, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_UAVResourceNames, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_UAVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_UAVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_a13be1795b5d7a688cd3fb1fe86a9934_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_size, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_data, 2, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_CBVResourceNames, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_CBVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_CBVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_CBVResourceSpaces, 7, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_SRVResourceNames, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_SRVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_SRVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_SRVResourceSpaces, 4, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_UAVResourceNames, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_UAVResourceBindings, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_UAVResourceCounts, g_ffx_fsr2_tcr_autogen_pass_e9d66b5e60072d343ccb1dfaf286559e_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

