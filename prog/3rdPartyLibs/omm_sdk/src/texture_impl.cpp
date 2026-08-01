/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "texture_impl.h"

#include "defines.h"
#include "std_containers.h"

#include "util/math.h"
#include "util/bit_tricks.h"
#include "util/texture.h"

#include <cstring>

namespace omm
{
    TextureImpl::TextureImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log) :
        m_stdAllocator(stdAllocator),
        m_log(log),
        m_mips(stdAllocator),
        m_textureFormat(ommCpuTextureFormat_MAX_NUM),
        m_textureFlags(ommCpuTextureFlags_None),
        m_tilingMode(TilingMode::MAX_NUM),
        m_alphaCutoff(-1.f),
        m_data(nullptr),
        m_dataSize(0),
        m_dataSAT(nullptr),
        m_dataSATSize(0)
    {
    }

    TextureImpl::~TextureImpl()
    {
        Deallocate();
    }

    ommResult TextureImpl::Validate(const ommCpuTextureDesc& desc) const {
        if (desc.mipCount == 0)
            return m_log.InvalidArg("[Invalid Arg] - mipCount must be non-zero");
        if (desc.format == ommCpuTextureFormat_MAX_NUM)
            return m_log.InvalidArg("[Invalid Arg] - format is not set");

        for (uint32_t i = 0; i < desc.mipCount; ++i)
        {
            if (!desc.mips[i].textureData)
                return m_log.InvalidArg("[Invalid Arg] - mips.textureData is not set");
            if (desc.mips[i].width == 0)
                return m_log.InvalidArg("[Invalid Arg] - mips.width must be non-zero");
            if (desc.mips[i].height == 0)
                return m_log.InvalidArg("[Invalid Arg] - mips.height must be non-zero");
            if (desc.mips[i].width > kMaxDim.x)
                return m_log.InvalidArg("[Invalid Arg] - mips.width must be less than kMaxDim.x (65536)");
            if (desc.mips[i].height > kMaxDim.y)
                return m_log.InvalidArg("[Invalid Arg] - mips.height must be less than kMaxDim.y (65536)");
        }

        return ommResult_SUCCESS;
    }

    static size_t GetSizePerPixel(ommCpuTextureFormat format)
    {
        if (format == ommCpuTextureFormat_UNORM8)
            return sizeof(uint8_t);
        else if (format == ommCpuTextureFormat_FP32)
            return sizeof(float);
        OMM_ASSERT(false);
        return 0;
    }

    ommResult TextureImpl::Create(const ommCpuTextureDesc& desc)
    {
        RETURN_STATUS_IF_FAILED(Validate(desc));

        Deallocate();

        m_mips.resize(desc.mipCount);
        m_tilingMode = !!((uint32_t)desc.flags & (uint32_t)ommCpuTextureFlags_DisableZOrder) ? TilingMode::Linear : TilingMode::MortonZ;
        m_textureFormat = desc.format;
        m_alphaCutoff = desc.alphaCutoff;
        m_textureFlags = desc.flags;

        const size_t sizePerPixel = GetSizePerPixel(m_textureFormat);

        const bool enableSAT = std::numeric_limits<uint32_t>::max() > m_mips[0].numElements && m_alphaCutoff >= 0;

        m_dataSize = 0;
        m_dataSATSize = 0;
        for (uint32_t mipIt = 0; mipIt < desc.mipCount; ++mipIt)
        {
            m_mips[mipIt].size = { desc.mips[mipIt].width, desc.mips[mipIt].height };
            m_mips[mipIt].sizeLog2.x = ctz(m_mips[mipIt].size.x);
            m_mips[mipIt].sizeLog2.y = ctz(m_mips[mipIt].size.y);
            m_mips[mipIt].sizef = (float2)m_mips[mipIt].size;
            m_mips[mipIt].sizeMinusOne = m_mips[mipIt].size - 1;
            m_mips[mipIt].rcpSize = 1.f / (float2)m_mips[mipIt].size;
            m_mips[mipIt].sizeIsPow2 = omm::isPow2(m_mips[mipIt].size.x) && omm::isPow2(m_mips[mipIt].size.y);
            m_mips[mipIt].dataOffset = m_dataSize;
            m_mips[mipIt].dataOffsetSAT = m_dataSATSize;

            if (m_tilingMode == TilingMode::Linear)
            {
              //  m_mips[mipIt].rowPitch = sizePerPixel * size_t(m_mips[mipIt].size.x);
                m_mips[mipIt].numElements = size_t(m_mips[mipIt].size.x) * m_mips[mipIt].size.y;
            }
            else if (m_tilingMode == TilingMode::MortonZ)
            {
                size_t maxDim = nextPow2(std::max(m_mips[mipIt].size.x, m_mips[mipIt].size.y));
             //   m_mips[mipIt].rowPitch = sizePerPixel * maxDim;
                m_mips[mipIt].numElements = maxDim * maxDim;
            }
            else
            {
                OMM_ASSERT(false);
                return ommResult_FAILURE;
            }

            m_dataSize += sizePerPixel * m_mips[mipIt].numElements;
            m_dataSize = math::Align(m_dataSize, kAlignment);

            if (enableSAT)
            {
                m_dataSATSize += sizeof(uint32_t) * m_mips[mipIt].numElements;;
                m_dataSATSize = math::Align(m_dataSATSize, kAlignment);
            }
        }

        m_data = m_stdAllocator.allocate(m_dataSize, kAlignment);
        m_dataSAT = enableSAT ? m_stdAllocator.allocate(m_dataSATSize, kAlignment) : nullptr;

        for (uint32_t mipIt = 0; mipIt < desc.mipCount; ++mipIt)
        {
            if (m_tilingMode == TilingMode::Linear)
            {
                const size_t kDefaultRowPitch = desc.mips[mipIt].width;
                const size_t srcRowPitch = desc.mips[mipIt].rowPitch == 0 ? kDefaultRowPitch : desc.mips[mipIt].rowPitch;

                if (kDefaultRowPitch == srcRowPitch)
                {
                    void* dst = m_data + m_mips[mipIt].dataOffset;
                    const void* src = (desc.mips[mipIt].textureData);
                    std::memcpy(dst, src, sizePerPixel * m_mips[mipIt].numElements);
                }
                else
                {
                    uint8_t* dstBegin = m_data + m_mips[mipIt].dataOffset;
                    const uint8_t* srcBegin = (const uint8_t*)desc.mips[mipIt].textureData;

                    const size_t dstRowPitch = m_mips[mipIt].size.x * sizePerPixel;
                    for (uint32_t rowIt = 0; rowIt < desc.mips[mipIt].height; rowIt++)
                    {
                        uint8_t* dst = dstBegin + rowIt * dstRowPitch;
                        const uint8_t* src = srcBegin + rowIt * (sizePerPixel * srcRowPitch);
                        std::memcpy(dst, src, dstRowPitch);
                    }
                }
            }
            else if (m_tilingMode == TilingMode::MortonZ)
            {
                uint8_t* dst = (uint8_t*)(m_data + m_mips[mipIt].dataOffset);
                const uint8_t* src = (uint8_t*)(desc.mips[mipIt].textureData);

                const size_t rowPitch = desc.mips[mipIt].rowPitch == 0 ? desc.mips[mipIt].width : desc.mips[mipIt].rowPitch;

                for (int j = 0; j < m_mips[mipIt].size.y; ++j)
                {
                    for (int i = 0; i < m_mips[mipIt].size.x; ++i)
                    {
                        const uint64_t idx = From2Dto1D<TilingMode::MortonZ>(int2(i, j), m_mips[mipIt].size);
                        OMM_ASSERT(idx < m_mips[mipIt].numElements);

                        uint8_t* cpyDst         = dst + idx * sizePerPixel;
                        const uint8_t* cpySrc   = src + (i + j * rowPitch) * sizePerPixel;

                        memcpy(cpyDst, cpySrc, sizePerPixel);
                    }
                }
            }
            else
            {
                OMM_ASSERT(false);
                return ommResult_FAILURE;
            }

            if (enableSAT)
            {
                uint32_t* dataSAT = (uint32_t * )(m_dataSAT + m_mips[mipIt].dataOffsetSAT);

                for (int j = 0; j < m_mips[mipIt].size.y; ++j)
                {
                    for (int i = 0; i < m_mips[mipIt].size.x; ++i)
                    {
                        dataSAT[i + j * m_mips[mipIt].size.x] = Load(int2(i, j), mipIt) > m_alphaCutoff;
                    }
                }

                // sum in X
                for (int j = 0; j < m_mips[mipIt].size.y; ++j)
                {
                    for (int i = 1; i < m_mips[mipIt].size.x; ++i)
                    {
                        dataSAT[i + j * m_mips[mipIt].size.x] += dataSAT[i - 1 + j * m_mips[mipIt].size.x];
                    }
                }

                // sum in Y
                for (int j = 1; j < m_mips[mipIt].size.y; ++j)
                {
                    for (int i = 0; i < m_mips[mipIt].size.x; ++i)
                    {
                        dataSAT[i + j * m_mips[mipIt].size.x] += dataSAT[i + (j - 1) * m_mips[mipIt].size.x];
                    }
                }
            }
        }

        return ommResult_SUCCESS;
    }

    void TextureImpl::Deallocate()
    {
        if (m_data != nullptr)
        {
            m_stdAllocator.deallocate(m_data, 0);
            m_data = nullptr;
        }
        if (m_dataSAT != nullptr)
        {
            m_stdAllocator.deallocate((uint8_t*)m_dataSAT, 0);
            m_dataSAT = nullptr;
        }
        m_mips.clear();
    }

    float TextureImpl::Load(const int2& texCoord, int32_t mip) const 
    {
        if (m_textureFormat == ommCpuTextureFormat_FP32)
        {
            if (m_tilingMode == TilingMode::Linear)
                return Load<ommCpuTextureFormat_FP32, TilingMode::Linear>(texCoord, mip);
            else if (m_tilingMode == TilingMode::MortonZ)
                return Load<ommCpuTextureFormat_FP32, TilingMode::MortonZ>(texCoord, mip);
        }
        else if (m_textureFormat == ommCpuTextureFormat_UNORM8)
        {
            if (m_tilingMode == TilingMode::Linear)
                return Load<ommCpuTextureFormat_UNORM8, TilingMode::Linear>(texCoord, mip);
            else if (m_tilingMode == TilingMode::MortonZ)
                return Load<ommCpuTextureFormat_UNORM8, TilingMode::MortonZ>(texCoord, mip);
        }
        OMM_ASSERT(false);
        return 0.f;
    }

    float TextureImpl::Bilinear(ommTextureAddressMode mode, const float2& p, int32_t mip) const 
    {
        float2 pixel = p * (float2)(m_mips[mip].size)-0.5f;
        float2 pixelFloor = glm::floor(pixel);
        int2 coords[omm::TexelOffset::MAX_NUM];
        omm::GatherTexCoord4(mode, m_mips[mip].sizeIsPow2, int2(pixelFloor), m_mips[mip].size, m_mips[mip].sizeLog2, coords);

        float a = (float)Load(coords[omm::TexelOffset::I0x0], mip);
        float b = (float)Load(coords[omm::TexelOffset::I0x1], mip);
        float c = (float)Load(coords[omm::TexelOffset::I1x0], mip);
        float d = (float)Load(coords[omm::TexelOffset::I1x1], mip);

        const float2 weight = glm::fract(pixel);
        float ac = glm::lerp<float>(a, c, weight.x);
        float bd = glm::lerp<float>(b, d, weight.x);
        float bilinearValue = glm::lerp(ac, bd, weight.y);
        return bilinearValue;
    }

    ommResult TextureImpl::GetTextureDesc(ommCpuTextureDesc& desc) const
    {
        desc.format = m_textureFormat;
        desc.flags = m_textureFlags;
        desc.alphaCutoff = m_alphaCutoff;
        desc.mipCount = (uint32_t)m_mips.size();

        if (desc.mips == nullptr)
            return ommResult_SUCCESS;

        for (uint32_t mipIt = 0; mipIt < desc.mipCount; ++mipIt)
        {
            ommCpuTextureMipDesc& mip = const_cast<ommCpuTextureMipDesc&>(desc.mips[mipIt]);
            mip.width = m_mips[mipIt].size.x;
            mip.height = m_mips[mipIt].size.y;
            mip.rowPitch = m_mips[mipIt].size.x;
            if (mip.textureData != nullptr)
            {
                const size_t sizePerPixel = GetSizePerPixel(m_textureFormat);

                if (m_tilingMode == TilingMode::MortonZ)
                {
                    const uint8_t* src = (uint8_t*)(m_data + m_mips[mipIt].dataOffset);
                    uint8_t* dst = (uint8_t*)(desc.mips[mipIt].textureData);

                    for (int i = 0; i < m_mips[mipIt].numElements; ++i)
                    {
                        const int2 pos = From1Dto2D<TilingMode::MortonZ>(i, m_mips[mipIt].size);

                        const uint8_t* cpySrc = src + i * sizePerPixel;
                        uint8_t* cpyDst = dst + (pos.x + pos.y * desc.mips[mipIt].rowPitch) * sizePerPixel;

                        memcpy(cpyDst, cpySrc, sizePerPixel);
                    }
                }
                else // if (m_tilingMode == TilingMode::Linear)
                {
                    const uint8_t* src = (uint8_t*)(m_data + m_mips[mipIt].dataOffset);
                    uint8_t* dst = (uint8_t*)(desc.mips[mipIt].textureData);
                    size_t numTexels = mip.width * mip.height;
                    memcpy(dst, src, numTexels * sizePerPixel);
                }
            }
        }
        return ommResult_SUCCESS;
    }

    template<>
    uint32_t TextureImpl::From2Dto1D<TilingMode::Linear>(const int2& idx, const int2& size)
    {
        return idx.x + idx.y * uint32_t(size.x);
    }

    template<>
    uint32_t TextureImpl::From2Dto1D<TilingMode::MortonZ>(const int2& idx, const int2& size)
    {
        // Based on
        // "Optimizing Memory Access on GPUs using Morton Order Indexing"
        // https://www.nocentino.com/Nocentino10.pdf
        // return mortonNumberBinIntl(idx.x, idx.y);

        // https://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
        return xy_to_morton(idx.x, idx.y);
    }

    template<> uint2 TextureImpl::From1Dto2D<TilingMode::Linear>(const uint32_t idx, const int2& size)
    {
        uint2 pos;
        pos.x = idx % size.x;
        pos.y = idx / size.x;
        return pos;
    }

    template<> uint2 TextureImpl::From1Dto2D<TilingMode::MortonZ>(const uint32_t idx, const int2& size)
    {
        uint2 res;
        morton_to_xy(idx, res.x, res.y);
        return res;
    }

}
