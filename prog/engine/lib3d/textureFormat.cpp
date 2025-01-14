// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_tex3d.h>
#include <debug/dag_debug.h>
#include <EASTL/tuple.h>

static const TextureFormatDesc format_descs[] = {
  {TEXFMT_A8R8G8B8, 4, false, 1, 1, ChannelDType::UNORM, {}, {8, 16, 0, 0, 1}, {8, 8, 0, 0, 1}, {8, 0, 0, 0, 1}, {8, 24, 0, 0, 1}, {},
    {}},
  {TEXFMT_A2R10G10B10, 4, false, 1, 1, ChannelDType::UNORM, {}, {10, 0, 0, 0, 1}, {10, 10, 0, 0, 1}, {10, 20, 0, 0, 1},
    {2, 30, 0, 0, 1}, {}, {}},
  {TEXFMT_A2B10G10R10, 4, false, 1, 1, ChannelDType::UNORM, {}, {10, 0, 0, 0, 1}, {10, 10, 0, 0, 1}, {10, 20, 0, 0, 1},
    {2, 30, 0, 0, 1}, {}, {}},
  {TEXFMT_A16B16G16R16, 8, false, 1, 1, ChannelDType::UNORM, {}, {16, 0, 0, 0, 1}, {16, 16, 0, 0, 1}, {16, 32, 0, 0, 1},
    {16, 48, 0, 0, 1}, {}, {}},
  {TEXFMT_A16B16G16R16F, 8, false, 1, 1, ChannelDType::SFLOAT, {}, {16, 0, 1, 1, 0}, {16, 16, 1, 1, 0}, {16, 32, 1, 1, 0},
    {16, 48, 1, 1, 0}, {}, {}},
  {TEXFMT_A32B32G32R32F, 16, false, 1, 1, ChannelDType::SFLOAT, {}, {32, 0, 1, 1, 0}, {32, 32, 1, 1, 0}, {32, 64, 1, 1, 0},
    {32, 96, 1, 1, 0}, {}, {}},
  {TEXFMT_G16R16, 4, false, 1, 1, ChannelDType::UNORM, {}, {16, 0, 0, 0, 1}, {16, 16, 0, 0, 1}, {}, {}, {}, {}},
  {TEXFMT_G16R16F, 4, false, 1, 1, ChannelDType::SFLOAT, {}, {16, 0, 1, 1, 0}, {16, 16, 1, 1, 0}, {}, {}, {}, {}},
  {TEXFMT_G32R32F, 8, false, 1, 1, ChannelDType::SFLOAT, {}, {32, 0, 1, 1, 0}, {32, 32, 1, 1, 0}, {}, {}, {}, {}},
  {TEXFMT_R16F, 2, false, 1, 1, ChannelDType::SFLOAT, {}, {16, 0, 1, 1, 0}, {}, {}, {}, {}, {}},
  {TEXFMT_R32F, 4, false, 1, 1, ChannelDType::SFLOAT, {}, {32, 0, 1, 1, 0}, {}, {}, {}, {}, {}},
  {TEXFMT_DXT1, 8, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
  {TEXFMT_DXT3, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {1, 0}, {}, {}},
  {TEXFMT_DXT5, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {}, {}},
  {TEXFMT_R32G32UI, 8, false, 1, 1, ChannelDType::UINT, {}, {32, 0, 0, 0, 0}, {32, 32, 0, 0, 0}, {}, {}, {}, {}},
  {TEXFMT_L16, 2, false, 1, 1, ChannelDType::UNORM, {}, {16, 0, 0, 0, 1}, {}, {}, {}, {}, {}},
  {TEXFMT_A8, 1, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {8, 0, 0, 0, 1}, {}, {}},
  {TEXFMT_L8, 1, false, 1, 1, ChannelDType::UNORM, {}, {8, 0, 0, 0, 1}, {}, {}, {}, {}, {}},
  {TEXFMT_A1R5G5B5, 2, false, 1, 1, ChannelDType::UNORM, {}, {5, 10, 0, 0, 1}, {5, 5, 0, 0, 1}, {5, 0, 0, 0, 1}, {1, 15, 0, 0, 1}, {},
    {}},
  {TEXFMT_A4R4G4B4, 2, false, 1, 1, ChannelDType::UNORM, {}, {4, 8, 0, 0, 1}, {4, 4, 0, 0, 1}, {4, 0, 0, 0, 1}, {4, 12, 0, 0, 1}, {},
    {}},
  {TEXFMT_R5G6B5, 2, false, 1, 1, ChannelDType::UNORM, {}, {5, 11, 0, 0, 1}, {6, 5, 0, 0, 1}, {5, 0, 0, 0, 1}, {}, {}, {}},
  {TEXFMT_A16B16G16R16S, 8, false, 1, 1, ChannelDType::SNORM, {}, {16, 0, 0, 1, 1}, {16, 16, 0, 1, 1}, {16, 32, 0, 1, 1},
    {16, 48, 0, 1, 1}, {}, {}},
  {TEXFMT_A16B16G16R16UI, 8, false, 1, 1, ChannelDType::UINT, {}, {16, 0, 0, 1, 1}, {16, 16, 0, 1, 1}, {16, 32, 0, 1, 1},
    {16, 48, 0, 1, 1}, {}, {}},
  {TEXFMT_A32B32G32R32UI, 16, false, 1, 1, ChannelDType::UINT, {}, {32, 0, 0, 0, 0}, {32, 32, 0, 0, 0}, {32, 64, 0, 0, 0},
    {32, 96, 0, 0, 0}, {}, {}},
  {TEXFMT_ATI1N, 8, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {}, {}, {}, {}, {}},
  {TEXFMT_ATI2N, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {}, {}, {}, {}},
  {TEXFMT_R8G8B8A8, 4, false, 1, 1, ChannelDType::UNORM, {}, {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {8, 16, 0, 0, 0}, {8, 24, 0, 0, 0}, {},
    {}},
  {TEXFMT_R32UI, 4, false, 1, 1, ChannelDType::UINT, {}, {32, 0, 0, 0, 0}, {}, {}, {}, {}, {}},
  {TEXFMT_R32SI, 4, false, 1, 1, ChannelDType::SINT, {}, {32, 0, 0, 0, 0}, {}, {}, {}, {}, {}},
  {TEXFMT_R11G11B10F, 4, false, 1, 1, ChannelDType::UFLOAT, {}, {11, 0, 1, 0, 0}, {11, 11, 1, 0, 0}, {10, 22, 1, 0, 0}, {}, {}, {}},
  {TEXFMT_R9G9B9E5, 4, false, 1, 1, ChannelDType::UFLOAT, {}, {9, 0, 1, 0, 0}, {9, 9, 1, 0, 0}, {9, 18, 1, 0, 0}, {}, {}, {}},
  {TEXFMT_R8G8, 2, false, 1, 1, ChannelDType::UNORM, {}, {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {}, {}, {}, {}},
  {TEXFMT_R8G8S, 2, false, 1, 1, ChannelDType::SNORM, {}, {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {}, {}, {}, {}},
  {TEXFMT_BC6H, 16, true, 4, 4, ChannelDType::UFLOAT, {}, {10, 0}, {10, 0}, {10, 0}, {}, {}, {}},
  {TEXFMT_BC7, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {}, {}},
  {TEXFMT_R8UI, 1, false, 1, 1, ChannelDType::UINT, {}, {8, 0, 0, 0, 1}, {}, {}, {}, {}, {}},
  {TEXFMT_R16UI, 2, false, 1, 1, ChannelDType::UINT, {}, {16, 0, 0, 0, 1}, {}, {}, {}, {}, {}},


  {TEXFMT_DEPTH24, 4, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {}, {24, 0, 1, 0, 1}, {}},
  {TEXFMT_DEPTH16, 2, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {}, {16, 0, 1, 0, 1}, {}},

  {TEXFMT_DEPTH32, 4, false, 1, 1, ChannelDType::SFLOAT, {}, {}, {}, {}, {}, {32, 0, 1, 1, 0}, {}},
  {TEXFMT_DEPTH32_S8, 5, false, 1, 1, ChannelDType::SFLOAT, ChannelDType::UINT, {}, {}, {}, {}, {32, 0, 1, 1, 0}, {8, 32, 0, 0, 1}},

  {TEXFMT_ASTC4, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
  {TEXFMT_ASTC8, 16, true, 8, 8, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
  {TEXFMT_ASTC12, 16, true, 12, 12, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
};

static const TextureFormatDesc unknown_format_desc = {TEXFMT_L8, 1, false, 1, 1, {}, {}, {}, {}, {}, {}, {}, {}};

struct TextureFormatName
{
  uint32_t dagorTextureFormat;
  const char *name;
  const char *synonym;
};

static const TextureFormatName format_names[] = {
  {TEXFMT_A8R8G8B8, "A8R8G8B8", "ARGB8"},
  {TEXFMT_A2R10G10B10, "A2R10G10B10", nullptr},
  {TEXFMT_A2B10G10R10, "A2B10G10R10", nullptr},
  {TEXFMT_A16B16G16R16, "A16B16G16R16", "ARGB16"},
  {TEXFMT_A16B16G16R16F, "A16B16G16R16F", "ARGB16F"},
  {TEXFMT_A32B32G32R32F, "A32B32G32R32F", "ARGB32F"},
  {TEXFMT_G16R16, "G16R16", "RG16"},
  {TEXFMT_G16R16F, "G16R16F", "RG16F"},
  {TEXFMT_G32R32F, "G32R32F", "RG32F"},
  {TEXFMT_R16F, "R16F", nullptr},
  {TEXFMT_R32F, "R32F", nullptr},
  {TEXFMT_DXT1, "DXT1", nullptr},
  {TEXFMT_DXT3, "DXT3", nullptr},
  {TEXFMT_DXT5, "DXT5", nullptr},
  {TEXFMT_R32G32UI, "R32G32UI", nullptr},
  {TEXFMT_L16, "L16", "R16"},
  {TEXFMT_A8, "A8", nullptr},
  {TEXFMT_L8, "L8", "R8"},
  {TEXFMT_A1R5G5B5, "A1R5G5B5", nullptr},
  {TEXFMT_A4R4G4B4, "A4R4G4B4", nullptr},
  {TEXFMT_R5G6B5, "R5G6B5", nullptr},
  {TEXFMT_A16B16G16R16S, "A16B16G16R16S", nullptr},
  {TEXFMT_A16B16G16R16UI, "A16B16G16R16UI", nullptr},
  {TEXFMT_A32B32G32R32UI, "A32B32G32R32UI", nullptr},
  {TEXFMT_ATI1N, "ATI1N", nullptr},
  {TEXFMT_ATI2N, "ATI2N", nullptr},
  {TEXFMT_R8G8B8A8, "R8G8B8A8", nullptr},
  {TEXFMT_R32UI, "R32UI", nullptr},
  {TEXFMT_R32SI, "R32SI", nullptr},
  {TEXFMT_R11G11B10F, "R11G11B10F", nullptr},
  {TEXFMT_R9G9B9E5, "R9G9B9E5", nullptr},
  {TEXFMT_R8G8, "R8G8", nullptr},
  {TEXFMT_R8G8S, "R8G8S", nullptr},
  {TEXFMT_BC6H, "BC6H", nullptr},
  {TEXFMT_BC7, "BC7", nullptr},
  {TEXFMT_R8UI, "R8UI", nullptr},

  {TEXFMT_DEPTH24, "DEPTH24", nullptr},
  {TEXFMT_DEPTH16, "DEPTH16", nullptr},
  {TEXFMT_DEPTH32, "DEPTH32", nullptr},
  {TEXFMT_DEPTH32_S8, "DEPTH32_S8", nullptr},

  {TEXFMT_ASTC4, "ASTC4", nullptr},
  {TEXFMT_ASTC8, "ASTC8", nullptr},
  {TEXFMT_ASTC12, "ASTC12", nullptr},
};


uint32_t parse_tex_format(const char *name, uint32_t default_fmt)
{
  if (!name || !name[0])
    return default_fmt;

  uint32_t srgbFlags = 0;
  if (strstr(name, "sRGB_") == name)
  {
    srgbFlags |= TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
    name += strlen("sRGB_");
  }
  if (strstr(name, "sRGBWR_") == name)
  {
    srgbFlags |= TEXCF_SRGBWRITE;
    name += strlen("sRGBWR_");
  }
  if (strstr(name, "sRGBRD_") == name)
  {
    srgbFlags |= TEXCF_SRGBREAD;
    name += strlen("sRGBRD_");
  }
  for (int i = 0; i < countof(format_names); i++)
    if (!stricmp(name, format_names[i].name) || (format_names[i].synonym && !stricmp(name, format_names[i].synonym)))
    {
      if (get_tex_format_desc(format_names[i].dagorTextureFormat).isBlockFormat)
        srgbFlags &= ~TEXCF_SRGBWRITE;
      return format_names[i].dagorTextureFormat | srgbFlags;
    }

  G_ASSERTF(0, "parse_tex_format: unknown format %s", name);
  return default_fmt;
}

const TextureFormatDesc &get_tex_format_desc(uint32_t fmt)
{
  fmt &= TEXFMT_MASK;
  for (int i = 0; i < countof(format_descs); i++)
    if (format_descs[i].dagorTextureFormat == fmt)
      return format_descs[i];

  G_ASSERTF(0, "unknown format: %d", fmt);
  return unknown_format_desc;
}

const char *get_tex_format_name(uint32_t fmt)
{
  fmt &= TEXFMT_MASK;
  for (int i = 0; i < countof(format_names); i++)
    if (format_names[i].dagorTextureFormat == fmt)
      return format_names[i].synonym ? format_names[i].synonym : format_names[i].name;

  return nullptr;
}

uint32_t get_tex_channel_value(const void *pixel, const TextureChannelFormatDesc &channel)
{
  //     prefix      full bytes      postfix
  //       ___  __________________  _____
  // [.....XXX][XXXXXXXX][XXXXXXXX][XXXXX...]
  const int prefixBits = ((channel.offset + 7) / 8) * 8 - channel.offset;
  const int fullBytes = (channel.bits - prefixBits) / 8;
  const int postfixBits = channel.bits - fullBytes * 8 - prefixBits;

  const uint8_t *bytePtr = reinterpret_cast<const uint8_t *>(pixel);
  bytePtr += channel.offset >> 3;

  uint32_t result = 0;
  int copiedBits = 0;
  if (prefixBits > 0)
  {
    result |= static_cast<uint32_t>(*bytePtr) >> (8 - prefixBits);
    copiedBits += prefixBits;
    ++bytePtr;
  }

  for (int i = 0; i < fullBytes; ++i)
  {
    result |= static_cast<uint32_t>(*bytePtr) << copiedBits;
    copiedBits += 8;
    ++bytePtr;
  }

  if (postfixBits > 0)
    result |= (static_cast<uint32_t>(*bytePtr) & ((1 << postfixBits) - 1)) << copiedBits;

  return result;
}

void set_tex_channel_value(void *pixel, const TextureChannelFormatDesc &channel, uint32_t bits)
{
  //     postfix      full bytes    prefix
  //       ___  __________________  _____
  // [.....XXX][XXXXXXXX][XXXXXXXX][XXXXX...]
  // <------------bit significance-----------
  const int prefixBits = ((channel.offset + 7) / 8) * 8 - channel.offset;
  const int fullBytes = (channel.bits - prefixBits) / 8;
  const int postfixBits = channel.bits - fullBytes * 8 - prefixBits;

  uint8_t *bytePtr = reinterpret_cast<uint8_t *>(pixel);
  bytePtr += channel.offset >> 3;

  int copiedBits = 0;
  if (prefixBits > 0)
  {
    //       mask
    //       ___
    // [XXXXX...]
    const uint8_t mask = (1 << prefixBits) - 1;
    *bytePtr = (*bytePtr & mask) | ((bits << (8 - prefixBits)) & ~mask);
    ++bytePtr;
  }

  for (int i = 0; i < fullBytes; ++i)
  {
    *bytePtr = static_cast<uint8_t>(bits >> copiedBits);
    copiedBits += 8;
    ++bytePtr;
  }

  if (postfixBits > 0)
  {
    //       mask
    //       ___
    // [.....XXX]
    const uint8_t mask = (1 << postfixBits) - 1;
    *bytePtr = (*bytePtr & ~mask) | static_cast<uint8_t>(bits >> copiedBits);
  }
}

static auto decompose_float(uint32_t value, uint32_t exponent_bits, uint32_t mantissa_bits)
{
  const uint32_t sign = (value >> (exponent_bits + mantissa_bits)) & ((1 << 1) - 1);
  const uint32_t exponent = (value >> (mantissa_bits)) & ((1 << exponent_bits) - 1);
  const uint32_t mantissa = (value >> 0) & ((1 << mantissa_bits) - 1);
  return eastl::tuple<uint32_t, uint32_t, uint32_t>{sign, exponent, mantissa};
}

static uint32_t compose_float(uint32_t sign, uint32_t exponent, uint32_t mantissa, uint32_t exponent_bits, uint32_t mantissa_bits)
{
  return (sign << (exponent_bits + mantissa_bits)) | (exponent << mantissa_bits) | mantissa;
}

float channel_bits_to_float(uint32_t bits, ChannelDType type, const TextureChannelFormatDesc &channel)
{
  // Conversion is done as per section 3.9 of vulkan API specification
  // The hope is that no other platform is crazy enough to implement
  // these formats in a fundamentally different way.
  G_ASSERT(channel.bits <= 32);

  const uint32_t mask = (uint64_t{1} << channel.bits) - 1;
  switch (type)
  {
    case ChannelDType::UNORM: return static_cast<float>(bits & mask) / ((uint64_t{1} << channel.bits) - 1);

    case ChannelDType::SNORM:
    {
      const bool sign = bits >> (channel.bits - 1);

      // Extend sign bit to convert into 32bit 2-s compliment
      const int32_t value = bitwise_cast<int32_t>(bits) | (sign ? ~mask : 0);

      return max(static_cast<float>(value) / ((1 << (channel.bits - 1)) - 1), -1.f);
    }

    case ChannelDType::SFLOAT: [[fallthrough]];
    case ChannelDType::UFLOAT:
      if (channel.bits == 32)
      {
        return bitwise_cast<float>(bits);
      }
      else if (channel.bits == 10 || channel.bits == 11 || channel.bits == 16)
      {
        constexpr uint32_t exponentBits = 5;
        const uint32_t mantissaBits = channel.bits - exponentBits - (channel.isSigned ? 1 : 0);

        constexpr uint32_t maxExponent = (1 << exponentBits) - 1;
        constexpr int32_t exponentBias = -static_cast<int32_t>(maxExponent) / 2;
        constexpr int32_t subnormalThresholdPower = exponentBias + 1;

        const auto [sign, exponent, mantissa] = decompose_float(bits, exponentBits, mantissaBits);

        const auto composeFloat32 = [](uint32_t sign, uint32_t exponent, uint32_t mantissa) {
          return bitwise_cast<float>(compose_float(sign, exponent, mantissa, 8, 23));
        };

        if (exponent == 0 && mantissa == 0)
        {
          // Case 1: +-zero
          return composeFloat32(sign, 0, 0);
        }
        else if (exponent == 0)
        {
          // Case 2: the representation is subnormal, but we can represent it as normal
          // Note that we can never return a subnormal representation from this function

          // 2^stp * M/2^mb = 2^(E32 - 127) * (1 + M32/2^23)
          // Find k, the biggest number such that 2^k < M,
          // then let M = 2^k + l
          const uint32_t logMantissa = get_log2i_unsafe(mantissa);
          const uint32_t remainder = mantissa - (1 << logMantissa);
          // 2^(stp - mb) * (2^k + l) = 2^(E32 - 127) * (1 + M32/2^23)
          // 2^(stp - mb + k) * (1 + l/2^k) = 2^(E32 - 127) * (1 + M32/2^23)
          // stp - mb + k = E32 - 127
          // E32 = 127 + stp - mb + k
          const uint32_t exponent32 = 127 + subnormalThresholdPower - mantissaBits + logMantissa;
          // l/2^k = M32/2^23
          // M32 = l * 2^(23 - k)
          const uint32_t mantissa32 = remainder << (23 - logMantissa);
          return composeFloat32(sign, exponent32, mantissa32);
        }
        else if (exponent < 31)
        {
          // Case 3: normal number, just shift the exponent/mantissa and we're done
          // E32 - 127 = E + eb
          const uint32_t exponent32 = 127 + exponent + exponentBias;
          // 1 + M32/2^23 = 1 + M/2^mb
          // M32 = M * 2^(23 - mb)
          const uint32_t mantissa32 = mantissa << (23 - mantissaBits);
          return composeFloat32(sign, exponent32, mantissa32);
        }
        else if (mantissa == 0)
        {
          // Case 4: +-infinity
          return composeFloat32(sign, (1 << 8) - 1, 0);
        }
        else
        {
          // Case 5: NaN (signaling/quit doesn't matter for us)
          return composeFloat32(sign, (1 << 8) - 1, 1);
        }
      }
      G_ASSERTF_RETURN(false, NAN, "Encountered an unsupported signed float format!");

    default: G_ASSERTF_RETURN(false, NAN, "The channel did not have a format convertible to float!");
  }
}

uint32_t float_to_channel_bits(float value, ChannelDType type, const TextureChannelFormatDesc &channel)
{
  // Conversion is done as per section 3.9 of vulkan API specification
  // The hope is that no other platform is crazy enough to implement
  // these formats in a fundamentally different way.

#if DAGOR_DBGLEVEL > 0
  const auto fpClass = fpclassify(value);
  if (!channel.isFloatPoint)
    G_ASSERTF(fpClass == FP_ZERO || fpClass == FP_NORMAL || fpClass == FP_SUBNORMAL,
      "Cannot convert a nan or infinite float to a non-floating point format!");
  if (!channel.isSigned)
    G_ASSERTF(fpClass == FP_NAN || value >= 0, "Cannot convert a negative float to an unsigned format!");
#endif

  // 64-bit channels are not supported on some of our platforms, so we
  // don't even try to handle them
  G_FAST_ASSERT(channel.bits <= 32);

  switch (type)
  {
    case ChannelDType::UNORM:
    {
      const uint32_t maxRepresentable = (uint32_t{1} << channel.bits) - 1;
      const uint32_t ivalue = static_cast<uint32_t>(value * maxRepresentable + 0.5f);
      G_ASSERTF(ivalue <= maxRepresentable, "Value is too big for the format!");
      return ivalue & maxRepresentable;
    }

    case ChannelDType::SNORM:
    {
      const int32_t maxRepresentable = (int32_t{1} << (channel.bits - 1)) - 1;
      const int32_t ivalue = static_cast<int32_t>(value * maxRepresentable + 0.5f);
      G_ASSERTF(ivalue <= maxRepresentable, "Value is too big for the format!");
      G_ASSERTF(-maxRepresentable < ivalue, "Value is too small for the format!");

      return ivalue & maxRepresentable;
    }

    case ChannelDType::SFLOAT:
    case ChannelDType::UFLOAT:
      if (channel.bits == 32 && channel.isSigned)
      {
        return bitwise_cast<uint32_t>(value);
      }
      else if (channel.bits == 10 || channel.bits == 11 || channel.bits == 16)
      {
        const auto [sign32, exponent32, mantissa32] = decompose_float(bitwise_cast<uint32_t>(value), 8, 23);
        const uint32_t sign = sign32;

        static constexpr uint32_t exponentBits = 5;
        // 10, 6 or 5
        const uint32_t mantissaBits = channel.bits - exponentBits - (channel.isSigned ? 1 : 0);

        // -15
        constexpr int32_t exponentBias = 1 - (1 << (exponentBits - 1));

        // -14
        constexpr int32_t subnormalThresholdPower = exponentBias + 1;
        // -24, -20 or -19
        const int32_t minReprPower = subnormalThresholdPower - mantissaBits;
        // 16
        constexpr int32_t maxReprPower = (1 << exponentBits) + exponentBias;

        const auto composeFloat = [mantissaBits](uint32_t sign, uint32_t exponent, uint32_t mantissa) {
          return compose_float(sign, exponent, mantissa, exponentBits, mantissaBits);
        };

        if (exponent32 < 127 + minReprPower)
        {
          // Case 1: exponent too small, representation is zero
          // Note that all subnormal float32 values fall into this case
          return composeFloat(sign, 0, 0);
        }
        else if (exponent32 < 127 + subnormalThresholdPower)
        {
          // Case 2: we can represent value as a subnormal repr
          // here, 127 + mrp <= exponent32 < 127 + stp
          // 0 <= exponent32 - 127 - mrp < stp - mrp
          // stp - mb = mrp
          // stp - mrp = mb
          // 0 <= exponent32 - 127 - mrp < mb

          // 23 + mrp + mrp <= exponent32 - 104 + mrp < 23 + stp + mrp
          // -23 - stp - mrp < 104 - exponent32 - mrp <= -23 - mrp - mrp
          // -9 - mrp < ... <= -23 - mrp - mrp
          // 15 < ... <= 25 // float16
          // 11 < ... <= 17 // float11
          // 10 < ... <= 15 // float10

          // 2^stp * M/2^mb = 2^(E32 - 127) * (1 + M32/2^23)
          // M * 2^(stp - mb) = 2^(E32 - 127) * (1 + M32/2^23)
          // M = 2^(E32 - 127) * (2^(mb - stp) + M32 * 2^(-23 + mb - stp))
          // M = 2^(E32 - 127 + mb - stp) + M32 * 2^(E32 - 150 + mb - stp)
          // M = 2^(E32 - 127 - mrp) + M32 * 2^(E32 - 150 - mrp)
          const auto mantissa = (1 << (exponent32 - minReprPower - 127)) + (mantissa32 >> (150 + minReprPower - exponent32));
          G_FAST_ASSERT(mantissa < 1 << mantissaBits);
          return composeFloat(sign, 0, mantissa);
        }
        else if (exponent32 < 127 + maxReprPower)
        {
          // Case 3: we can represent value as a normal repr
          // here, 127 + stp <= exponent32 < 127 + mrp
          // 0 <= exponent32 - 127 - stp < mrp - stp

          // 2^(E16 + eb) = 2^(E32 - 127)
          // E16 = E32 - 127 - eb
          const auto exponent = exponent32 - 127 - exponentBias;
          G_FAST_ASSERT(exponent < 1 << exponentBits);

          // 1 + M/2^mb = 1 + M32/2^23
          // M = M32*2^(-23 + mb)
          const auto mantissa = mantissa32 >> (23 - mantissaBits);
          G_FAST_ASSERT(mantissa < 1 << mantissaBits);
          return composeFloat(sign, exponent, mantissa);
        }
        else if (mantissa32 == 0)
        {
          // Case 3: exponent too big, representation is inf
          return composeFloat(sign, (1 << exponentBits) - 1, 0);
        }
        else
        {
          // Case 5: NaN (signaling/quit doesn't matter for us)
          return composeFloat(sign, (1 << exponentBits) - 1, 1);
        }
      }
      G_ASSERTF_RETURN(false, 0, "Encountered an unsupported signed float format!");

    default: G_ASSERTF_RETURN(false, 0, "The channel did not have a format convertible from float!");
  }
}
