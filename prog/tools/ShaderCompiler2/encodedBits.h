#pragma once

namespace encoded_bits
{
enum
{
  SkipValidation,
  Optimize,
  DebugInfo,
  ScarlettW32,
  HLSL2021,
  EnableFp16,

  BitCount
};

static constexpr int ProfileLength = 6;
static constexpr int ExtraBytes = ProfileLength + 1;
}; // namespace encoded_bits