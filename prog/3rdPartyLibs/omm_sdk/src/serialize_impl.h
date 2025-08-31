/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "omm.h"
#include "defines.h"
#include "std_containers.h"
#include "texture_impl.h"
#include "log.h"

#include <shared/math.h>
#include <shared/texture.h>

#include <map>
#include <set>

#include "std_allocator.h"

typedef uint64_t XXH64_hash_t;

namespace omm
{
namespace Cpu
{
    // struct Header_V1
    // {
    //     XXH64_hash_t storedHash;
    //     int major = 0;
    //     int minor = 0;
    //     int patch = 0;
    //     int inputDescVersion = 0;
    //     ommCpuSerializeFlags flags = ommCpuSerializeFlags_None;
    // };

    struct Header
    {
        XXH64_hash_t storedHash;
        int major = 0;
        int minor = 0;
        int patch = 0;
        int inputDescVersion = 0;
        int flags = ommCpuSerializeFlags_None;
        int decompressedSize = 0;
    };

    enum Serialize {
        VERSION = 2
    };


    static inline constexpr int HeaderSizeV1 = sizeof(XXH64_hash_t) + 5 * sizeof(int);
    static inline constexpr int HeaderSizeV2 = sizeof(XXH64_hash_t) + 6 * sizeof(int);

    static inline constexpr int HeaderSize[VERSION] = { HeaderSizeV1, HeaderSizeV2 };

    static ommResult GetHeaderSize(int version, int& outSize)
    {
        if (version > VERSION)
            return ommResult_FAILURE;
        outSize = HeaderSize[version - 1];
        return ommResult_SUCCESS;
    }

    class SerializeResultImpl
    {
    public:
        static inline constexpr HandleType kHandleType = HandleType::SerializeResult;

        SerializeResultImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log);
        ~SerializeResultImpl();

        inline const StdAllocator<uint8_t>& GetStdAllocator() const
        {
            return m_stdAllocator;
        }

        const ommCpuBlobDesc* GetDesc() const
        {
            return &m_desc;
        }

        ommResult Serialize(const ommCpuDeserializedDesc& desc);

    private:
        static uint32_t _GetMaxIndex(const ommCpuBakeInputDesc& inputDesc);

        template<class TMemoryStreamBuf>
        ommResult _Serialize(const ommCpuBakeInputDesc& inputDesc, TMemoryStreamBuf& buffer);
        template<class TMemoryStreamBuf>
        ommResult _Serialize(const ommCpuBakeResultDesc& resultDesc, TMemoryStreamBuf& buffer);
        template<class TMemoryStreamBuf>
        ommResult _Serialize(const ommCpuDeserializedDesc& desc, int compressionEnabled, TMemoryStreamBuf& buffer);

        StdAllocator<uint8_t> m_stdAllocator;
        const Logger& m_log;
        ommCpuBlobDesc m_desc;
    };

    class MemoryStreamBuf : public std::streambuf {
    public:
        MemoryStreamBuf(uint8_t* data, size_t size) {
            setg((char*)data, (char*)data, (char*)data + size);
            setp((char*)data, (char*)data + size);
        }
    };

    class PassthroughStreamBuf : public std::streambuf {
    public:
        PassthroughStreamBuf() { }

        std::streamsize GetWrittenSize() const
        {
            return written_size;
        }

        std::streamsize xsputn(const char* s, std::streamsize n) override {
            written_size += n;
            return n;
        }
    private:
        std::streamsize written_size = 0;
    };

    class DeserializedResultImpl
    {
    public:
        static inline constexpr HandleType kHandleType = HandleType::DeserializeResult;

        DeserializedResultImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log);
        ~DeserializedResultImpl();

        inline const StdAllocator<uint8_t>& GetStdAllocator() const
        {
            return m_stdAllocator;
        }

        const ommCpuDeserializedDesc* GetDesc() const
        {
            return &m_inputDesc;
        }

        ommResult Deserialize(const ommCpuBlobDesc& desc);

    private:

        ommResult _Deserialize(Header& header, XXH64_hash_t hash, MemoryStreamBuf& buffer);
        ommResult _Deserialize(ommCpuBakeInputDesc& inputDesc, const Header& header, MemoryStreamBuf& buffer);
        ommResult _Deserialize(ommCpuBakeResultDesc& resultDesc, const Header& header, MemoryStreamBuf& buffer);
        ommResult _Deserialize(ommCpuDeserializedDesc& desc, const Header& header, MemoryStreamBuf& buffer);

        StdAllocator<uint8_t> m_stdAllocator;
        const Logger& m_log;
        ommCpuDeserializedDesc m_inputDesc;
        vector<uint8_t> m_deserializedData;
    };
} // namespace Cpu
} // namespace omm
