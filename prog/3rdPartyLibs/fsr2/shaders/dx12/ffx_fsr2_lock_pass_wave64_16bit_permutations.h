#include "ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9.h"
#include "ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6.h"

typedef union ffx_fsr2_lock_pass_wave64_16bit_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_lock_pass_wave64_16bit_PermutationKey;

typedef struct ffx_fsr2_lock_pass_wave64_16bit_PermutationInfo {
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
} ffx_fsr2_lock_pass_wave64_16bit_PermutationInfo;

static const uint32_t g_ffx_fsr2_lock_pass_wave64_16bit_IndirectionTable[] = {
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

static const ffx_fsr2_lock_pass_wave64_16bit_PermutationInfo g_ffx_fsr2_lock_pass_wave64_16bit_PermutationInfo[] = {
    { g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_size, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_data, 1, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_CBVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_CBVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_CBVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_SRVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_SRVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_SRVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_UAVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_UAVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_UAVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_c34b026516fd01349b887fbebb98c3a9_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_size, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_data, 1, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_CBVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_CBVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_CBVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_SRVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_SRVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_SRVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_UAVResourceNames, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_UAVResourceBindings, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_UAVResourceCounts, g_ffx_fsr2_lock_pass_wave64_16bit_3254eec3ac44ba071908086b09661eb6_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

