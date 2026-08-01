#include "ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01.h"
#include "ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69.h"

typedef union ffx_fsr2_lock_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_lock_pass_16bit_PermutationKey;

typedef struct ffx_fsr2_lock_pass_16bit_PermutationInfo {
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
} ffx_fsr2_lock_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_fsr2_lock_pass_16bit_IndirectionTable[] = {
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

static const ffx_fsr2_lock_pass_16bit_PermutationInfo g_ffx_fsr2_lock_pass_16bit_PermutationInfo[] = {
    { g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_size, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_data, 1, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_CBVResourceNames, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_CBVResourceBindings, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_CBVResourceCounts, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_SRVResourceNames, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_SRVResourceBindings, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_SRVResourceCounts, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_UAVResourceNames, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_UAVResourceBindings, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_UAVResourceCounts, g_ffx_fsr2_lock_pass_16bit_44878b75b70edc2855edcfe71012aa01_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_size, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_data, 1, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_CBVResourceNames, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_CBVResourceBindings, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_CBVResourceCounts, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_CBVResourceSpaces, 1, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_SRVResourceNames, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_SRVResourceBindings, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_SRVResourceCounts, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_SRVResourceSpaces, 2, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_UAVResourceNames, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_UAVResourceBindings, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_UAVResourceCounts, g_ffx_fsr2_lock_pass_16bit_7cca69e4d7a6857b8ace04058f8dfe69_UAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

