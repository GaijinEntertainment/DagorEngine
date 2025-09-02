//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daNet/bitStream.h>
#include <util/dag_string.h>
#include <gameMath/quantization.h>


enum class BitStreamWalkerMode
{
  Read,
  Write,
};

template <BitStreamWalkerMode mode>
struct BitStreamWalker : das::DataWalker
{
  static inline uint32_t bytes2bits(uint32_t by) { return by << 3; }

  typedef std::conditional_t<mode == BitStreamWalkerMode::Write, danet::BitStream, const danet::BitStream> BitStreamType;
  BitStreamType *stream;
  das::LineInfo *debugInfo;
  bool compressNext = false;
  int packedUnitVectorNext = -1;
  int quantizedPosNext = -1;

  BitStreamWalker(BitStreamType *stream, das::Context *context, das::LineInfo *debugInfo) : stream(stream), debugInfo(debugInfo)
  {
    this->context = context;
  }

  bool canVisitHandle(char *, das::TypeInfo *) override { return false; }  // TODO
  bool canVisitVariant(char *, das::TypeInfo *) override { return false; } // TODO
  bool canVisitTable(char *, das::TypeInfo *) override { return false; }   // TODO
  bool canVisitTableData(das::TypeInfo *) override { return false; }
  bool canVisitPointer(das::TypeInfo *) override { return false; }
  bool canVisitLambda(das::TypeInfo *) override { return false; }
  bool canVisitIterator(das::TypeInfo *) override { return false; }

  bool isCompressible(das::TypeInfo *ti)
  {
    const das::Type type = ti->type == das::Type::tArray ? ti->firstType->type : ti->type;
    // all 2 and 4 byte int types
    return type == das::Type::tInt16 || type == das::Type::tUInt16 || type == das::Type::tInt || type == das::Type::tUInt ||
           type == das::Type::tInt2 || type == das::Type::tUInt2 || type == das::Type::tInt3 || type == das::Type::tUInt3 ||
           type == das::Type::tInt4 || type == das::Type::tUInt4 || type == das::Type::tRange || type == das::Type::tURange ||
           type == das::Type::tBitfield || type == das::Type::tEnumeration16 || type == das::Type::tEnumeration;
  }

  bool isPackableUnitVector(das::TypeInfo *ti)
  {
    const das::Type type = ti->type == das::Type::tArray ? ti->firstType->type : ti->type;
    return type == das::Type::tFloat3;
  }

  void beforeStructureField(char *, das::StructInfo *, char *, das::VarInfo *vi, bool) override
  {
    // TODO: allow @compressed for nested arrays, like array<array<int>>
    if (vi->annotation_arguments != nullptr)
    {
      const bool typeIsCompressible = isCompressible(vi);
      const bool typeIsPackable = isPackableUnitVector(vi);
      const bool typeIsQuantizedPos = typeIsPackable; // currently only float3 is supported
      auto *annArgs = reinterpret_cast<das::AnnotationArguments *>(vi->annotation_arguments);
      for (das::AnnotationArgument &ann : *annArgs)
      {
        if (ann.name == "packedUnitVector")
        {
          if (!typeIsPackable)
          {
            class String err(0, "packedUnitVector can only be used with float3 type (%s)", vi->name);
            error(err.c_str());
            return;
          }
          if (DAGOR_UNLIKELY(ann.type != das::Type::tInt))
          {
            class String err(0, "packedUnitVector value must be int (%s)", vi->name);
            error(err.c_str());
            return;
          }
          packedUnitVectorNext = ann.iValue;
          if (!(packedUnitVectorNext == 16 || packedUnitVectorNext == 24 || packedUnitVectorNext == 32))
          {
            class String err(0, "packedUnitVector value must be 16, 24 or 32 bits (%d)", packedUnitVectorNext);
            error(err.c_str());
            return;
          }
          break;
        }
        if (ann.name == "compressed")
        {
          if (!typeIsCompressible)
          {
            class String err(0, "compressed can only be used with signed and unsigned int types (%s)", vi->name);
            error(err.c_str());
            return;
          }
          if (DAGOR_UNLIKELY(ann.type != das::Type::tBool))
          {
            class String err(0, "compressed value must be bool (%s)", vi->name);
            error(err.c_str());
            return;
          }
          compressNext = ann.bValue;
          break;
        }
        if (typeIsQuantizedPos && ann.name == "quantizedPos")
        {
          if (DAGOR_UNLIKELY(ann.type != das::Type::tInt))
          {
            class String err(0, "quantizedPos value must be int (%s)", vi->name);
            error(err.c_str());
            return;
          }
          quantizedPosNext = ann.iValue;
          if (!(quantizedPosNext == 32 || quantizedPosNext == 64))
          {
            class String err(0, "quantizedPos value must be 34 or 64 bits (%d)", quantizedPosNext);
            error(err.c_str());
            return;
          }
          break;
        }
      }
    }
  }
  void afterStructureField(char *, das::StructInfo *, char *, das::VarInfo *, bool) override
  {
    compressNext = false;
    packedUnitVectorNext = -1;
    quantizedPosNext = -1;
  }
  void beforeArray(das::Array *pa, das::TypeInfo *ti) override
  {
    if constexpr (mode == BitStreamWalkerMode::Write)
    {
      stream->Write(pa->size);
    }
    else
    {
      uint32_t size;
      if (!stream->Read(size))
      {
        class String err(0, "Failed to read array size %@bits (%@/%@)", bytes2bits(sizeof(size)), stream->GetReadOffset(),
          stream->GetNumberOfBitsUsed());
        error(err.c_str());
        return;
      }
      builtin_array_clear(*pa, context, nullptr);
      builtin_array_resize(*pa, size, ti->firstType->size, context, nullptr);
    }
  }

  template <typename TT>
  __forceinline void process(TT &data)
  {
    if constexpr (mode == BitStreamWalkerMode::Write)
    {
      stream->Write(data);
    }
    else
    {
      if (!stream->Read(data))
      {
        class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(data)), stream->GetReadOffset(),
          stream->GetNumberOfBitsUsed());
        error(err.c_str());
        return;
      }
    }
  }
  template <typename TT>
  __forceinline void processCompressible(TT &data)
  {
    if constexpr (mode == BitStreamWalkerMode::Write)
    {
      compressNext ? stream->WriteCompressed(data) : stream->Write(data);
    }
    else
    {
      const bool ok = compressNext ? stream->ReadCompressed(data) : stream->Read(data);
      if (!ok)
      {
        class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(data)), stream->GetReadOffset(),
          stream->GetNumberOfBitsUsed());
        error(err.c_str());
        return;
      }
    }
  }
  template <int dim, typename TT>
  __forceinline void processCompressibleVector(TT &data)
  {
    processCompressible(data.x);
    processCompressible(data.y);
    if constexpr (dim >= 3)
    {
      processCompressible(data.z);
    }
    if constexpr (dim == 4)
    {
      processCompressible(data.w);
    }
  }

  template <int dim, typename TT>
  void processPackableVector(TT &data)
  {
    if (quantizedPosNext >= 0)
    {
      G_ASSERTF(dim == 3, "packedUnitVector can only be 3D");
      if constexpr (mode == BitStreamWalkerMode::Write)
      {
        if (quantizedPosNext == 32)
        {
          gamemath::QuantizedPos<12, 8, 12, gamemath::NoScale, gamemath::NoScale, gamemath::NoScale> quant;
          quant.packPos(reinterpret_cast<Point3 &>(data));
          stream->Write(quant);
        }
        else // quantizedPosNext == 64
        {
          static constexpr uint64_t POS_CLAMPED_BIT = 1ULL << 63;
          bool posClamped = false;
          gamemath::QuantizedPos<22, 19, 22, gamemath::NoScale, gamemath::NoScale, gamemath::NoScale> quant;
          quant.packPos(reinterpret_cast<Point3 &>(data), &posClamped);
          if (posClamped)
            quant.qpos |= POS_CLAMPED_BIT;
          stream->Write(quant);
        }
        return;
      }
      else
      {
        if (quantizedPosNext == 32)
        {
          gamemath::QuantizedPos<12, 8, 12, gamemath::NoScale, gamemath::NoScale, gamemath::NoScale> quant;
          if (!stream->Read(quant))
          {
            class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(quant)), stream->GetReadOffset(),
              stream->GetNumberOfBitsUsed());
            error(err.c_str());
            return;
          }
          reinterpret_cast<Point3 &>(data) = quant.unpackPos();
        }
        else // quantizedPosNext == 64
        {
          gamemath::QuantizedPos<22, 19, 22, gamemath::NoScale, gamemath::NoScale, gamemath::NoScale> quant;
          if (!stream->Read(quant))
          {
            class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(quant)), stream->GetReadOffset(),
              stream->GetNumberOfBitsUsed());
            error(err.c_str());
            return;
          }
          reinterpret_cast<Point3 &>(data) = quant.unpackPos();
        }
        return;
      }
    }
    if (packedUnitVectorNext >= 0)
    {
      G_ASSERTF(dim == 3, "packedUnitVector can only be 3D");
      if constexpr (mode == BitStreamWalkerMode::Write)
      {
        if (packedUnitVectorNext <= 16)
        {
          uint16_t val = gamemath::pack_unit_vec<uint16_t>(reinterpret_cast<Point3 &>(data), packedUnitVectorNext);
          stream->Write(val);
        }
        else
        {
          uint32_t val = gamemath::pack_unit_vec<uint32_t>(reinterpret_cast<Point3 &>(data), packedUnitVectorNext);
          stream->Write(val);
        }
        return;
      }
      else
      {
        if (packedUnitVectorNext <= 16)
        {
          uint16_t val;
          if (!stream->Read(val))
          {
            class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(val)), stream->GetReadOffset(),
              stream->GetNumberOfBitsUsed());
            error(err.c_str());
            return;
          }
          reinterpret_cast<Point3 &>(data) = gamemath::unpack_unit_vec<uint16_t>(val, packedUnitVectorNext);
        }
        else
        {
          uint32_t val;
          if (!stream->Read(val))
          {
            class String err(0, "Failed to read %@bits (%@/%@)", bytes2bits(sizeof(val)), stream->GetReadOffset(),
              stream->GetNumberOfBitsUsed());
            error(err.c_str());
            return;
          }
          reinterpret_cast<Point3 &>(data) = gamemath::unpack_unit_vec<uint32_t>(val, packedUnitVectorNext);
        }
        return;
      }
    }
    process(data);
  }

  void Bool(bool &data) override { process(data); }
  void Int8(int8_t &data) override { process(data); }
  void UInt8(uint8_t &data) override { process(data); }
  void Int16(int16_t &data) override { processCompressible(data); }
  void UInt16(uint16_t &data) override { processCompressible(data); }
  void Int64(int64_t &data) override { process(data); }
  void UInt64(uint64_t &data) override { process(data); }
  void String(char *&data) override
  {
    if constexpr (mode == BitStreamWalkerMode::Write)
    {
      stream->Write(data);
    }
    else
    {
      eastl::string str;
      if (!stream->Read(str))
      {
        class String err(0, "Failed to read string %@/%@", stream->GetReadOffset(), stream->GetNumberOfBitsUsed());
        error(err.c_str());
        return;
      }
      data = context->allocateString(str.c_str(), str.size(), debugInfo);
    }
  }
  void Double(double &data) override { process(data); }
  void Float(float &data) override { process(data); }
  void Int(int32_t &data) override { processCompressible(data); }
  void UInt(uint32_t &data) override { processCompressible(data); }
  void Bitfield(uint32_t &data, das::TypeInfo *) override { processCompressible(data); }
  void Int2(das::int2 &data) override { processCompressibleVector<2>(data); }
  void Int3(das::int3 &data) override { processCompressibleVector<3>(data); }
  void Int4(das::int4 &data) override { processCompressibleVector<4>(data); }
  void UInt2(das::uint2 &data) override { processCompressibleVector<2>(data); }
  void UInt3(das::uint3 &data) override { processCompressibleVector<3>(data); }
  void UInt4(das::uint4 &data) override { processCompressibleVector<4>(data); }
  void Float2(das::float2 &data) override { process(data); }
  void Float3(das::float3 &data) override { processPackableVector<3>(data); }
  void Float4(das::float4 &data) override { process(data); }
  void Range(das::range &data) override { processCompressibleVector<2>(data); }
  void URange(das::urange &data) override { processCompressibleVector<2>(data); }
  void Range64(das::range64 &data) override { process(data); }
  void URange64(das::urange64 &data) override { process(data); }
  void WalkEnumeration(int32_t &data, das::EnumInfo *) override { processCompressible(data); }
  void WalkEnumeration8(int8_t &data, das::EnumInfo *) override { process(data); }
  void WalkEnumeration16(int16_t &data, das::EnumInfo *) override { processCompressible(data); }
  void WalkEnumeration64(int64_t &data, das::EnumInfo *) override { process(data); }
};