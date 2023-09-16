#define __XBOX_STRIP_DXIL  1
#define BLIT_SIGNATURE  "RootConstants(num32BitConstants=4, b0, visibility = SHADER_VISIBILITY_VERTEX)," \
                        "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
                        "StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT," \
                        "addressU = TEXTURE_ADDRESS_CLAMP, "\
                        "addressV = TEXTURE_ADDRESS_CLAMP, "\
                        "addressW = TEXTURE_ADDRESS_CLAMP," \
                        "maxAnisotropy = 1," \
                        "comparisonFunc = COMPARISON_ALWAYS," \
                        "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
                        "visibility = SHADER_VISIBILITY_PIXEL)," \
                        "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS)"