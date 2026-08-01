/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "math.h"
#include "bit_tricks.h"
#include "util/bird.h"
#include <algorithm>
#include <omm.hpp>

namespace omm
{
    static constexpr int    kTexCoordInvalid = 0x7FFFFFFF;
    static constexpr int    kTexCoordBorder = 0x7FFFFFFE;
    static inline int2   kTexCoordInvalid2{ kTexCoordInvalid, kTexCoordInvalid };
    static inline int2   kTexCoordBorder2{ kTexCoordBorder, kTexCoordBorder };

    enum TexelOffset {
        I0x0,
        I1x0,
        I0x1,
        I1x1,
        MAX_NUM,
    };

    template<ommTextureAddressMode eAddressMode, bool bTexSizeIsPow2>
    static inline int2 GetTexCoord(const int2& texCoord, const int2& texSize, const int2& texSizeLog2) {
        switch (eAddressMode)
        {
        case ommTextureAddressMode_Wrap: {

            if constexpr (bTexSizeIsPow2)
            {
                return uint2(texCoord) & (uint2(texSize - 1));
            }
            else
            {
                return int2(uint2(texCoord) % uint2(texSize));
            }
        }
        case ommTextureAddressMode_Mirror: {

            if (bTexSizeIsPow2)
            {
                int2 texCoordAbs;
                texCoordAbs.x = glm::abs(texCoord.x) - (texCoord.x < 0);
                texCoordAbs.y = glm::abs(texCoord.y) - (texCoord.y < 0);

                bool isFlippedX = (texCoordAbs.x >> texSizeLog2.x) & 0x1;
                bool isFlippedY = (texCoordAbs.y >> texSizeLog2.y) & 0x1;
                const int2 wrapped = uint2(texCoordAbs) & (uint2(texSize - 1));
                return { isFlippedX ? texSize.x - wrapped.x - 1 : wrapped.x ,
                         isFlippedY ? texSize.y - wrapped.y - 1 : wrapped.y };
            }
           else
            {
                const int2 texCoordAbs = (int2)glm::abs((float2)texCoord + 0.5f);
                const uint2 isFlipped = (uint2((texCoordAbs) / texSize) % uint2(2u));
                const int2 wrapped = int2(uint2((texCoordAbs)) % uint2(texSize));
                return { isFlipped.x ? texSize.x - wrapped.x - 1 : wrapped.x ,
                         isFlipped.y ? texSize.y - wrapped.y - 1 : wrapped.y };
            }
        }
        case ommTextureAddressMode_Clamp: {
            return { std::clamp(texCoord.x, 0, texSize.x - 1), std::clamp(texCoord.y, 0, texSize.y - 1) };
        }
        case ommTextureAddressMode_Border: {
            int2 res = texCoord;
            if (texCoord.x >= texSize.x || texCoord.x < 0)
                res.x = kTexCoordBorder;
            if (texCoord.y >= texSize.y || texCoord.y < 0)
                res.y = kTexCoordBorder;
            return res;
        }
        case ommTextureAddressMode_MirrorOnce: {
            const int2 texCoordAbs = (int2)glm::abs(float2(texCoord) + 0.5f);
            return { std::clamp(texCoordAbs.x, 0, texSize.x - 1), std::clamp(texCoordAbs.y, 0, texSize.y - 1) };
        }
        default: {
            return kTexCoordInvalid2;
        }
        }
    }

    static inline int2 GetTexCoord(ommTextureAddressMode addressingMode, bool bTexSizeIsPow2, const int2& texCoord, const int2& texSize, const int2& texSizeLog2) {
        switch (addressingMode)
        {
        case ommTextureAddressMode_Wrap: {
            return bTexSizeIsPow2 ? GetTexCoord<ommTextureAddressMode_Wrap, true>(texCoord, texSize, texSizeLog2) : GetTexCoord<ommTextureAddressMode_Wrap, false>(texCoord, texSize, texSizeLog2);
        }
        case ommTextureAddressMode_Mirror: {
            return bTexSizeIsPow2 ? GetTexCoord<ommTextureAddressMode_Mirror, true>(texCoord, texSize, texSizeLog2) : GetTexCoord<ommTextureAddressMode_Mirror, false>(texCoord, texSize, texSizeLog2);
        }
        case ommTextureAddressMode_Clamp: {
            return bTexSizeIsPow2 ? GetTexCoord<ommTextureAddressMode_Clamp, true>(texCoord, texSize, texSizeLog2) : GetTexCoord<ommTextureAddressMode_Clamp, false>(texCoord, texSize, texSizeLog2);
        }
        case ommTextureAddressMode_Border: {
            return bTexSizeIsPow2 ? GetTexCoord<ommTextureAddressMode_Border, true>(texCoord, texSize, texSizeLog2) : GetTexCoord<ommTextureAddressMode_Border, false>(texCoord, texSize, texSizeLog2);
        }
        case ommTextureAddressMode_MirrorOnce: {
            return bTexSizeIsPow2 ? GetTexCoord<ommTextureAddressMode_MirrorOnce, true>(texCoord, texSize, texSizeLog2) : GetTexCoord<ommTextureAddressMode_MirrorOnce, false>(texCoord, texSize, texSizeLog2);
        }
        default: {
            return kTexCoordInvalid2;
        }
        }
    }

    static inline int2 GetTexCoord(omm::TextureAddressMode addressingMode, bool texSizeIsLog2, const int2& texCoord, const int2& texSize, const int2& texSizeLog2) {
        return GetTexCoord((ommTextureAddressMode)addressingMode, texSizeIsLog2, texCoord, texSize, texSizeLog2);
    }

    static inline void GatherTexCoord4(ommTextureAddressMode addressingMode, bool texSizeIsLog2, const int2& texCoord, const int2& texSize, const int2& texSizeLog2, int2* coords) {
        const int2 offset   = GetTexCoord(addressingMode, texSizeIsLog2, texCoord, texSize, texSizeLog2);
        const int2 offset11 = GetTexCoord(addressingMode, texSizeIsLog2, texCoord + int2{ 1, 1 }, texSize, texSizeLog2);
        coords[TexelOffset::I0x0] = { offset.x,     offset.y };
        coords[TexelOffset::I1x0] = { offset11.x,   offset.y };
        coords[TexelOffset::I0x1] = { offset.x,     offset11.y };
        coords[TexelOffset::I1x1] = { offset11.x,   offset11.y };
    }

    template<ommTextureAddressMode eAddressMode, bool bTexSizeIsPow2>
    static inline void GatherTexCoord4(const int2& texCoord, const int2& texSize, const int2& texSizeLog2, int2* __restrict coords) {
        const int2 offset = GetTexCoord<eAddressMode, bTexSizeIsPow2>(texCoord, texSize, texSizeLog2);
        const int2 offset11 = GetTexCoord<eAddressMode, bTexSizeIsPow2>(texCoord + int2{ 1, 1 }, texSize, texSizeLog2);
        coords[TexelOffset::I0x0] = { offset.x,     offset.y };
        coords[TexelOffset::I1x0] = { offset11.x,   offset.y };
        coords[TexelOffset::I0x1] = { offset.x,     offset11.y };
        coords[TexelOffset::I1x1] = { offset11.x,   offset11.y };
    }

    template<ommTextureAddressMode eAddressMode, bool bTexSizeIsPow2>
    static inline void GatherTexCoord4(const int2& texCoord, const int2& texSize, const int2& texSizeLog2, int2& out00, int2& out10, int2& out01, int2& out11) {
        const int2 offset = GetTexCoord<eAddressMode, bTexSizeIsPow2>(texCoord, texSize, texSizeLog2);
        const int2 offset11 = GetTexCoord<eAddressMode, bTexSizeIsPow2>(texCoord + int2{ 1, 1 }, texSize, texSizeLog2);
        out00 = { offset.x,     offset.y };
        out10 = { offset11.x,   offset.y };
        out01 = { offset.x,     offset11.y };
        out11 = { offset11.x,   offset11.y };
    }

    static inline uint32_t GetTexCoordFormatSize(ommTexCoordFormat format) {
        switch (format) {
        case ommTexCoordFormat_UV16_UNORM:
            return sizeof(uint16_t) * 2;
        case ommTexCoordFormat_UV16_FLOAT:
            return sizeof(uint16_t) * 2;
        case ommTexCoordFormat_UV32_FLOAT:
            return sizeof(float2);
        default:
            return 0;
        }
    }

} // namespace omm