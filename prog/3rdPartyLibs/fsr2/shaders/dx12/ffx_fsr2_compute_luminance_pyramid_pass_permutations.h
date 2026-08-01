#include "ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4.h"

typedef union ffx_fsr2_compute_luminance_pyramid_pass_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_compute_luminance_pyramid_pass_PermutationKey;

typedef struct ffx_fsr2_compute_luminance_pyramid_pass_PermutationInfo {
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
} ffx_fsr2_compute_luminance_pyramid_pass_PermutationInfo;

static const uint32_t g_ffx_fsr2_compute_luminance_pyramid_pass_IndirectionTable[] = {
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

static const ffx_fsr2_compute_luminance_pyramid_pass_PermutationInfo g_ffx_fsr2_compute_luminance_pyramid_pass_PermutationInfo[] = {
    { g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_size, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_data, 2, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_CBVResourceNames, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_CBVResourceBindings, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_CBVResourceCounts, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_CBVResourceSpaces, 1, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SRVResourceNames, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SRVResourceBindings, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SRVResourceCounts, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SRVResourceSpaces, 4, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_UAVResourceNames, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_UAVResourceBindings, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_UAVResourceCounts, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_UAVResourceSpaces, 1, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SamplerResourceNames, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SamplerResourceBindings, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SamplerResourceCounts, g_ffx_fsr2_compute_luminance_pyramid_pass_ec2901cea95a9b143e04b9d43c2d15b4_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

