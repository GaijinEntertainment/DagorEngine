#pragma once

#include <string_view>
#include <vector>

#include "../compileResult.h"

struct RegisterData
{
  // uniform_bufers
  std::vector<uint8_t> bRegistersConstBuffer{}; // cbuf

  // storage_buffers
  std::vector<uint8_t> tRegistersBuffer{};            // Structured ByteAddressBuffer
  std::vector<uint8_t> uRegistersBuffer{};            // RWByteAddressBuffer RWStructuredBuffer
  std::vector<uint8_t> uRegistersBufferWithCounter{}; // ??

  // separate_images
  // static std::vector<uint8_t> tRegistersSampledImages{}; // @smp2d
  // static std::vector<uint8_t> tRegistersSampledImagesWithCompare{}; // @shd
  // static std::vector<uint8_t> tRegistersBufferSampledImage{}; // Buffer<uint>
  std::vector<uint8_t> tRegistersForImages{};

  // storage_images
  std::vector<uint8_t> uRegistersImage{};       // 0 RWTexture2D
  std::vector<uint8_t> uRegistersBufferImage{}; // ??

  //
  std::vector<uint8_t> tRegistersInputAttachement{};      // ??
  std::vector<uint8_t> tRegistersAccelerationStructure{}; // self-explanatory
  //
};

struct Hlsl2SpirvResult
{
  bool failed;

  std::vector<unsigned int> byteCode;

  ComputeShaderInfo computeInfo;

  RegisterData registers;
};


Hlsl2SpirvResult hlsl2spirv(const char *source, const char *profile, const char *entry, bool skip_validation,
  CompileResult &compile_result);
