#include "ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80.h"
#include "ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0.h"

typedef union ffx_fsr2_lock_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_lock_pass_wave64_PermutationKey;

typedef struct ffx_fsr2_lock_pass_wave64_PermutationInfo {
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
} ffx_fsr2_lock_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_fsr2_lock_pass_wave64_IndirectionTable[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
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
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static const ffx_fsr2_lock_pass_wave64_PermutationInfo g_ffx_fsr2_lock_pass_wave64_PermutationInfo[] = {
    { g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_size, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_data, 1, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_CBVResourceNames, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_CBVResourceBindings, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_CBVResourceCounts, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_SRVResourceNames, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_SRVResourceBindings, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_SRVResourceCounts, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_UAVResourceNames, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_UAVResourceBindings, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_UAVResourceCounts, g_ffx_fsr2_lock_pass_wave64_730b8d52ca031841f5fff5e612baea80_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_size, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_data, 1, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_CBVResourceNames, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_CBVResourceBindings, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_CBVResourceCounts, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_SRVResourceNames, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_SRVResourceBindings, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_SRVResourceCounts, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_UAVResourceNames, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_UAVResourceBindings, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_UAVResourceCounts, g_ffx_fsr2_lock_pass_wave64_b597a4c71dfb74a855991adaed0ad8c0_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

