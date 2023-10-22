#include <3d/dag_tex3d.h>
#include <debug/dag_debug.h>

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
  {TEXFMT_V16U16, 4, false, 1, 1, ChannelDType::SNORM, {}, {16, 0, 0, 0, 1}, {16, 16, 0, 0, 1}, {}, {}, {}, {}},
  {TEXFMT_L16, 2, false, 1, 1, ChannelDType::UNORM, {}, {16, 0, 0, 0, 1}, {}, {}, {}, {}, {}},
  {TEXFMT_A8, 1, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {8, 0, 0, 0, 1}, {}, {}},
  {TEXFMT_L8, 1, false, 1, 1, ChannelDType::UNORM, {}, {8, 0, 0, 0, 1}, {}, {}, {}, {}, {}},
  {TEXFMT_A1R5G5B5, 2, false, 1, 1, ChannelDType::UNORM, {}, {5, 10, 0, 0, 1}, {5, 5, 0, 0, 1}, {5, 0, 0, 0, 1}, {1, 15, 0, 0, 1}, {},
    {}},
  {TEXFMT_A4R4G4B4, 2, false, 1, 1, ChannelDType::UNORM, {}, {4, 8, 0, 0, 1}, {4, 4, 0, 0, 1}, {4, 0, 0, 0, 1}, {4, 12, 0, 0, 1}, {},
    {}},
  {TEXFMT_R5G6B5, 2, false, 1, 1, ChannelDType::UNORM, {}, {5, 11, 0, 0, 1}, {6, 5, 0, 0, 1}, {5, 0, 0, 0, 1}, {}, {}, {}},
  {TEXFMT_A8L8, 2, false, 1, 1, ChannelDType::UNORM, {},
    // NOTE: support for this was dropped in dx10. Both dx12 and vulkan
    // drivers actually use TEXFMT_R8G8 for this.
    {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {}, {}, {}, {}},
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
  {TEXFMT_R11G11B10F, 4, false, 1, 1, ChannelDType::UFLOAT, {}, {11, 0, 1, 0, 0}, {11, 11, 1, 0, 0}, {10, 22, 1, 0, 0}, {}, {}, {}},
  {TEXFMT_R9G9B9E5, 4, false, 1, 1, ChannelDType::UFLOAT, {}, {9, 0, 1, 0, 0}, {9, 9, 1, 0, 0}, {9, 18, 1, 0, 0}, {}, {}, {}},
  {TEXFMT_R8G8, 2, false, 1, 1, ChannelDType::UNORM, {}, {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {}, {}, {}, {}},
  {TEXFMT_R8G8S, 2, false, 1, 1, ChannelDType::SNORM, {}, {8, 0, 0, 0, 0}, {8, 8, 0, 0, 0}, {}, {}, {}, {}},
  {TEXFMT_BC6H, 16, true, 4, 4, ChannelDType::UFLOAT, {}, {10, 0}, {10, 0}, {10, 0}, {}, {}, {}},
  {TEXFMT_BC7, 16, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {}, {}},
  {TEXFMT_R8UI, 1, false, 1, 1, ChannelDType::UINT, {}, {8, 0, 0, 0, 1}, {}, {}, {}, {}, {}},


  {TEXFMT_DEPTH24, 4, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {}, {24, 0, 1, 0, 1}, {}},
  {TEXFMT_DEPTH16, 2, false, 1, 1, ChannelDType::UNORM, {}, {}, {}, {}, {}, {16, 0, 1, 0, 1}, {}},

  {TEXFMT_DEPTH32, 4, false, 1, 1, ChannelDType::SFLOAT, {}, {}, {}, {}, {}, {32, 0, 1, 1, 0}, {}},
  {TEXFMT_DEPTH32_S8, 5, false, 1, 1, ChannelDType::SFLOAT, ChannelDType::UINT, {}, {}, {}, {}, {32, 0, 1, 1, 0}, {8, 32, 0, 0, 1}},

  {TEXFMT_PVRTC2X, 8, true, 8, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
  {TEXFMT_PVRTC2A, 8, true, 8, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {}, {}},
  {TEXFMT_PVRTC4X, 8, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {}, {}, {}},
  {TEXFMT_PVRTC4A, 8, true, 4, 4, ChannelDType::UNORM, {}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {}, {}},

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
  {TEXFMT_V16U16, "V16U16", nullptr},
  {TEXFMT_L16, "L16", "R16"},
  {TEXFMT_A8, "A8", nullptr},
  {TEXFMT_L8, "L8", "R8"},
  {TEXFMT_A1R5G5B5, "A1R5G5B5", nullptr},
  {TEXFMT_A4R4G4B4, "A4R4G4B4", nullptr},
  {TEXFMT_R5G6B5, "R5G6B5", nullptr},
  {TEXFMT_A8L8, "A8L8", nullptr},
  {TEXFMT_A16B16G16R16S, "A16B16G16R16S", nullptr},
  {TEXFMT_A16B16G16R16UI, "A16B16G16R16UI", nullptr},
  {TEXFMT_A32B32G32R32UI, "A32B32G32R32UI", nullptr},
  {TEXFMT_ATI1N, "ATI1N", nullptr},
  {TEXFMT_ATI2N, "ATI2N", nullptr},
  {TEXFMT_R8G8B8A8, "R8G8B8A8", nullptr},
  {TEXFMT_R32UI, "R32UI", nullptr},
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

  {TEXFMT_PVRTC2X, "PVRTC2X", nullptr},
  {TEXFMT_PVRTC2A, "PVRTC2A", nullptr},
  {TEXFMT_PVRTC4X, "PVRTC4X", nullptr},
  {TEXFMT_PVRTC4A, "PVRTC4A", nullptr},

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
    result |= *(bytePtr) >> (8 - prefixBits);
    copiedBits += prefixBits;
    ++bytePtr;
  }

  for (int i = 0; i < fullBytes; ++i)
  {
    result |= *bytePtr << copiedBits;
    copiedBits += 8;
    ++bytePtr;
  }

  if (postfixBits > 0)
    result |= (*bytePtr & ((1 << postfixBits) - 1)) << copiedBits;

  return result;
}

float channel_bits_to_float(uint32_t bits, ChannelDType type, const TextureChannelFormatDesc &channel)
{
  // Conversion is done as per section 3.9 of vulkan API specification
  // The hope is that no other platform is crazy enough to implement
  // these formats in a fundamentally different way.

  const uint32_t mask = ((1 << channel.bits) - 1);
  switch (type)
  {
    case ChannelDType::UNORM: return static_cast<float>(bits & mask) / ((1 << channel.bits) - 1);

    case ChannelDType::SNORM:
    {
      const bool sign = bits >> (channel.bits - 1);

      // Extend sign bit to convert into 32bit 2-s compliment
      const int32_t value = bitwise_cast<int32_t>(bits) | (sign ? ~mask : 0);

      return max(static_cast<float>(value) / ((1 << (channel.bits - 1)) - 1), -1.f);
    }

    case ChannelDType::SFLOAT:
      if (channel.bits == 32)
      {
        return bitwise_cast<float>(bits);
      }
      else if (channel.bits == 16)
      {
        // TODO: verify whether this does not introduce more error
        // than necessary
        const uint32_t sign = (bits & ((1 << 16) - 1)) >> 15;
        const uint32_t exponent = (bits & ((1 << 15) - 1)) >> 10;
        const uint32_t mantissa = (bits & ((1 << 10) - 1)) >> 0;

        const float fSign = sign ? -1.f : 1.f;
        const float fMantissa = static_cast<float>(mantissa) / (1 << 10);

        if (exponent == 0 && mantissa == 0)
          return fSign * 0.f;
        else if (exponent == 0)
          return fSign * exp2f(-14) * fMantissa;
        else if (exponent < 31)
          return fSign * exp2f(static_cast<int>(exponent) - 15) * (1 + fMantissa);
        else if (mantissa == 0)
          return fSign * INFINITY;
        else
          return NAN;
      }
      G_ASSERTF_RETURN(false, NAN, "Encountered an unsupported signed float format!");

    case ChannelDType::UFLOAT:
      if (channel.bits == 11 || channel.bits == 10)
      {
        const uint32_t exponentBits = 5;
        const uint32_t mantissaBits = channel.bits - exponentBits;

        const uint32_t exponent = (bits & ((1 << channel.bits) - 1)) >> mantissaBits;
        const uint32_t mantissa = (bits & ((1 << mantissaBits) - 1)) >> 0;

        const float fMantissa = static_cast<float>(mantissa) / (1 << mantissaBits);

        if (exponent == 0 && mantissa == 0)
          return 0.f;
        else if (exponent == 0)
          return exp2f(-14) * fMantissa;
        else if (exponent < 31)
          return exp2f(static_cast<int>(exponent) - 15) * (1 + fMantissa);
        else if (mantissa == 0)
          return INFINITY;
        else
          return NAN;
      }
      G_ASSERTF_RETURN(false, NAN, "Encountered an unsupported unsigned float format!");

    default: G_ASSERTF_RETURN(false, NAN, "The channel did not have a format convertible to float!");
  }
}
