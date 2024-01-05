#pragma once

enum class FormatClass
{
  UNKNOWN,
  BIT_8,
  BIT_16,
  BIT_24,
  BIT_32,
  BIT_32_G8B8G8R8,
  BIT_32_B8G8R8G8,
  BIT_48,
  BIT_64,
  BIT_64_R10G10B10A10,
  BIT_64_G10B10G10R10,
  BIT_64_B10G10R10G10,
  BIT_64_R12G12B12A12,
  BIT_64_G12B12G12R12,
  BIT_64_B12G12R12G12,
  BIT_64_G16B16G16R16,
  BIT_64_B16G16R16G16,
  BIT_96,
  BIT_128,
  BIT_192,
  BIT_256,
  BC1_RGB,
  BC1_RGBA,
  BC2,
  BC3,
  BC4,
  BC5,
  BC6H,
  BC7,
  ETC2_RGB,
  ETC2_RGBA,
  ETC2_EAC_RGBA,
  EAC_R,
  EAC_RG,
  ASTC_4x4,
  ASTC_5x4,
  ASTC_5x5,
  ASTC_6x5,
  ASTC_6x6,
  ASTC_8x5,
  ASTC_8x6,
  ASTC_8x8,
  ASTC_10x5,
  ASTC_10x6,
  ASTC_10x8,
  ASTC_10x10,
  ASTC_12x10,
  ASTC_12x12,
  D16,
  D24,
  D32,
  S8,
  D16S8,
  D24S8,
  D32S8,
  BIT_8_3_PLANE_420,
  BIT_8_2_PLANE_420,
  BIT_8_3_PLANE_422,
  BIT_8_2_PLANE_422,
  BIT_8_3_PLANE_444,
  BIT_10_3_PLANE_420,
  BIT_10_2_PLANE_420,
  BIT_10_3_PLANE_422,
  BIT_10_2_PLANE_422,
  BIT_10_3_PLANE_444,
  BIT_12_3_PLANE_420,
  BIT_12_2_PLANE_420,
  BIT_12_3_PLANE_422,
  BIT_12_2_PLANE_422,
  BIT_12_3_PLANE_444,
  BIT_16_3_PLANE_420,
  BIT_16_2_PLANE_420,
  BIT_16_3_PLANE_422,
  BIT_16_2_PLANE_422,
  BIT_16_3_PLANE_444
};

enum class PlanarType
{
  P420,
  P422,
  P444,
  LINEAR,
};

template <PlanarType>
struct PlanarTraits;

template <>
struct PlanarTraits<PlanarType::P420>
{
  static constexpr bool is_planar = true;
  static constexpr unsigned plane_block_size = 2;
  static constexpr unsigned plane0_width_divider = 1;
  static constexpr unsigned plane0_height_divider = 1;
  static constexpr unsigned plane1_width_divider = 2;
  static constexpr unsigned plane1_height_divider = 2;
  static constexpr unsigned plane2_width_divider = 2;
  static constexpr unsigned plane2_height_divider = 2;
};

template <>
struct PlanarTraits<PlanarType::P422>
{
  static constexpr bool is_planar = true;
  static constexpr unsigned plane_block_size = 2;
  static constexpr unsigned plane0_width_divider = 1;
  static constexpr unsigned plane0_height_divider = 1;
  static constexpr unsigned plane1_width_divider = 2;
  static constexpr unsigned plane1_height_divider = 1;
  static constexpr unsigned plane2_width_divider = 2;
  static constexpr unsigned plane2_height_divider = 1;
};

template <>
struct PlanarTraits<PlanarType::P444>
{
  static constexpr bool is_planar = true;
  static constexpr unsigned plane_block_size = 2;
  static constexpr unsigned plane0_width_divider = 1;
  static constexpr unsigned plane0_height_divider = 1;
  static constexpr unsigned plane1_width_divider = 1;
  static constexpr unsigned plane1_height_divider = 1;
  static constexpr unsigned plane2_width_divider = 1;
  static constexpr unsigned plane2_height_divider = 1;
};

template <>
struct PlanarTraits<PlanarType::LINEAR>
{
  static constexpr bool is_planar = false;
  static constexpr unsigned plane_block_size = 1;
  static constexpr unsigned plane0_width_divider = 1;
  static constexpr unsigned plane0_height_divider = 1;
  static constexpr unsigned plane1_width_divider = 1;
  static constexpr unsigned plane1_height_divider = 1;
  static constexpr unsigned plane2_width_divider = 1;
  static constexpr unsigned plane2_height_divider = 1;
};

template <FormatClass>
struct FormatClassTraits;

inline constexpr unsigned calc_plane_size(unsigned bits, unsigned block_size, unsigned div_x, unsigned div_y)
{
  return ((bits * block_size * block_size) / div_x) / div_y;
}

template <FormatClass C, unsigned Planes, PlanarType PT, unsigned Plane0Bits, unsigned Plane1Bits = 0, unsigned Plane2Bits = 0,
  unsigned BlockX = 1, unsigned BlockY = 1, unsigned BlockZ = 1>
struct FormatClassTraitPlanar : PlanarTraits<PT>
{
  static constexpr FormatClass format_class = C;
  // gcc fails to pull in PlanarTraits<PT> correctly, so help it
  static constexpr bool is_planar = PlanarTraits<PT>::is_planar;
  static constexpr unsigned plane_block_size = PlanarTraits<PT>::plane_block_size;
  static constexpr unsigned plane0_width_divider = PlanarTraits<PT>::plane0_width_divider;
  static constexpr unsigned plane0_height_divider = PlanarTraits<PT>::plane0_height_divider;
  static constexpr unsigned plane1_width_divider = PlanarTraits<PT>::plane1_width_divider;
  static constexpr unsigned plane1_height_divider = PlanarTraits<PT>::plane1_height_divider;
  static constexpr unsigned plane2_width_divider = PlanarTraits<PT>::plane2_width_divider;
  static constexpr unsigned plane2_height_divider = PlanarTraits<PT>::plane2_height_divider;
  static constexpr unsigned planes = Planes;
  static constexpr unsigned plane0_bits = Plane0Bits;
  static constexpr unsigned plane1_bits = Plane1Bits;
  static constexpr unsigned plane2_bits = Plane2Bits;
  // treat planar formats similar to block compressed formats, each block
  // has the combined size of all planes for a pixel group
  // This only currect for caculating the size of one image
  // indexing into each plane has to be done differently.
  static constexpr unsigned block_width = is_planar ? plane_block_size : BlockX;
  static constexpr unsigned block_height = is_planar ? plane_block_size : BlockY;
  static constexpr unsigned block_depth = is_planar ? plane_block_size : BlockZ;
  static constexpr unsigned bits = !is_planar ? plane0_bits
                                   : Planes == 2
                                     ? calc_plane_size(plane0_bits, plane_block_size, plane0_width_divider, plane0_height_divider) +
                                         calc_plane_size(plane1_bits, plane_block_size, plane1_width_divider, plane1_height_divider)
                                     : calc_plane_size(plane0_bits, plane_block_size, plane0_width_divider, plane0_height_divider) +
                                         calc_plane_size(plane1_bits, plane_block_size, plane1_width_divider, plane1_height_divider) +
                                         calc_plane_size(plane2_bits, plane_block_size, plane2_width_divider, plane2_height_divider);
  static constexpr unsigned bytes = bits / 8;
};

template <FormatClass C, unsigned B, unsigned X, unsigned Y, unsigned Z>
struct FormatClassTraitBlock : FormatClassTraitPlanar<C, 1, PlanarType::LINEAR, B, 0, 0, X, Y, Z>
{};

template <FormatClass C, unsigned B>
struct FormatClassTraitLinear : FormatClassTraitBlock<C, B, 1, 1, 1>
{};

template <FormatClass C>
struct FormatClassTraitBC1 : FormatClassTraitBlock<C, 64, 4, 4, 1>
{};
template <FormatClass C>
struct FormatClassTraitBC : FormatClassTraitBlock<C, 128, 4, 4, 1>
{};
template <FormatClass C>
struct FormatClassTraitETC2 : FormatClassTraitBlock<C, 64, 4, 4, 1>
{};
template <FormatClass C>
struct FormatClassTraitETC2_EAC : FormatClassTraitBlock<C, 128, 4, 4, 1>
{};

template <FormatClass C, unsigned X, unsigned Y>
struct FormatClassTraitASTC : FormatClassTraitBlock<C, 128, X, Y, 1>
{};

template <>
struct FormatClassTraits<FormatClass::UNKNOWN> : FormatClassTraitLinear<FormatClass::UNKNOWN, 0>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8> : FormatClassTraitLinear<FormatClass::BIT_8, 8>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16> : FormatClassTraitLinear<FormatClass::BIT_16, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_24> : FormatClassTraitLinear<FormatClass::BIT_24, 24>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_32> : FormatClassTraitLinear<FormatClass::BIT_32, 32>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_32_G8B8G8R8> : FormatClassTraitLinear<FormatClass::BIT_32_G8B8G8R8, 32>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_32_B8G8R8G8> : FormatClassTraitLinear<FormatClass::BIT_32_B8G8R8G8, 32>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_48> : FormatClassTraitLinear<FormatClass::BIT_48, 48>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64> : FormatClassTraitLinear<FormatClass::BIT_64, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_R10G10B10A10> : FormatClassTraitLinear<FormatClass::BIT_64_R10G10B10A10, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_G10B10G10R10> : FormatClassTraitLinear<FormatClass::BIT_64_G10B10G10R10, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_B10G10R10G10> : FormatClassTraitLinear<FormatClass::BIT_64_B10G10R10G10, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_R12G12B12A12> : FormatClassTraitLinear<FormatClass::BIT_64_R12G12B12A12, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_G12B12G12R12> : FormatClassTraitLinear<FormatClass::BIT_64_G12B12G12R12, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_B12G12R12G12> : FormatClassTraitLinear<FormatClass::BIT_64_B12G12R12G12, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_G16B16G16R16> : FormatClassTraitLinear<FormatClass::BIT_64_G16B16G16R16, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_64_B16G16R16G16> : FormatClassTraitLinear<FormatClass::BIT_64_B16G16R16G16, 64>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_96> : FormatClassTraitLinear<FormatClass::BIT_96, 96>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_128> : FormatClassTraitLinear<FormatClass::BIT_128, 128>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_192> : FormatClassTraitLinear<FormatClass::BIT_192, 192>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_256> : FormatClassTraitLinear<FormatClass::BIT_256, 256>
{};
template <>
struct FormatClassTraits<FormatClass::BC1_RGB> : FormatClassTraitBC1<FormatClass::BC1_RGB>
{};
template <>
struct FormatClassTraits<FormatClass::BC1_RGBA> : FormatClassTraitBC1<FormatClass::BC1_RGBA>
{};
template <>
struct FormatClassTraits<FormatClass::BC2> : FormatClassTraitBC<FormatClass::BC2>
{};
template <>
struct FormatClassTraits<FormatClass::BC3> : FormatClassTraitBC<FormatClass::BC3>
{};
template <>
struct FormatClassTraits<FormatClass::BC4> : FormatClassTraitBC1<FormatClass::BC4>
{};
template <>
struct FormatClassTraits<FormatClass::BC5> : FormatClassTraitBC<FormatClass::BC5>
{};
template <>
struct FormatClassTraits<FormatClass::BC6H> : FormatClassTraitBC<FormatClass::BC6H>
{};
template <>
struct FormatClassTraits<FormatClass::BC7> : FormatClassTraitBC<FormatClass::BC7>
{};
template <>
struct FormatClassTraits<FormatClass::ETC2_RGB> : FormatClassTraitETC2<FormatClass::ETC2_RGB>
{};
template <>
struct FormatClassTraits<FormatClass::ETC2_RGBA> : FormatClassTraitETC2<FormatClass::ETC2_RGBA>
{};
template <>
struct FormatClassTraits<FormatClass::ETC2_EAC_RGBA> : FormatClassTraitETC2_EAC<FormatClass::ETC2_EAC_RGBA>
{};
template <>
struct FormatClassTraits<FormatClass::EAC_R> : FormatClassTraitETC2<FormatClass::EAC_R>
{};
template <>
struct FormatClassTraits<FormatClass::EAC_RG> : FormatClassTraitETC2_EAC<FormatClass::EAC_RG>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_4x4> : FormatClassTraitASTC<FormatClass::ASTC_4x4, 4, 4>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_5x4> : FormatClassTraitASTC<FormatClass::ASTC_5x4, 5, 4>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_5x5> : FormatClassTraitASTC<FormatClass::ASTC_5x5, 5, 5>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_6x5> : FormatClassTraitASTC<FormatClass::ASTC_6x5, 6, 5>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_6x6> : FormatClassTraitASTC<FormatClass::ASTC_6x6, 6, 6>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_8x5> : FormatClassTraitASTC<FormatClass::ASTC_8x5, 8, 5>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_8x6> : FormatClassTraitASTC<FormatClass::ASTC_8x6, 8, 6>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_8x8> : FormatClassTraitASTC<FormatClass::ASTC_8x8, 8, 8>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_10x5> : FormatClassTraitASTC<FormatClass::ASTC_10x5, 10, 5>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_10x6> : FormatClassTraitASTC<FormatClass::ASTC_10x6, 10, 6>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_10x8> : FormatClassTraitASTC<FormatClass::ASTC_10x8, 10, 8>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_10x10> : FormatClassTraitASTC<FormatClass::ASTC_10x10, 10, 10>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_12x10> : FormatClassTraitASTC<FormatClass::ASTC_12x10, 12, 10>
{};
template <>
struct FormatClassTraits<FormatClass::ASTC_12x12> : FormatClassTraitASTC<FormatClass::ASTC_12x12, 12, 12>
{};
template <>
struct FormatClassTraits<FormatClass::D16> : FormatClassTraitLinear<FormatClass::D16, 16>
{};
template <>
struct FormatClassTraits<FormatClass::D24> : FormatClassTraitLinear<FormatClass::D24, 24>
{};
template <>
struct FormatClassTraits<FormatClass::D32> : FormatClassTraitLinear<FormatClass::D32, 32>
{};
template <>
struct FormatClassTraits<FormatClass::S8> : FormatClassTraitLinear<FormatClass::S8, 8>
{};
template <>
struct FormatClassTraits<FormatClass::D16S8> : FormatClassTraitLinear<FormatClass::D16S8, 24>
{};
template <>
struct FormatClassTraits<FormatClass::D24S8> : FormatClassTraitLinear<FormatClass::D24S8, 32>
{};
template <>
struct FormatClassTraits<FormatClass::D32S8> : FormatClassTraitLinear<FormatClass::D32S8, 40>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8_3_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_8_3_PLANE_420, 3, PlanarType::P420, 8, 8, 8>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8_2_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_8_2_PLANE_420, 2, PlanarType::P420, 8, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8_3_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_8_3_PLANE_422, 3, PlanarType::P422, 8, 8, 8>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8_2_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_8_2_PLANE_422, 2, PlanarType::P422, 8, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_8_3_PLANE_444>
  : FormatClassTraitPlanar<FormatClass::BIT_8_3_PLANE_444, 3, PlanarType::P444, 8, 8, 8>
{};
template <> // strange format type, only uses upper 10 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_10_3_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_420, 3, PlanarType::P420, 16, 16, 16>
{};
template <> // strange format type, only uses upper 10 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_10_2_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_2_PLANE_420, 2, PlanarType::P420, 16, 32>
{};
template <> // strange format type, only uses upper 10 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_10_3_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 3, PlanarType::P422, 16, 16, 16>
{};
template <> // strange format type, only uses upper 10 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_10_2_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 2, PlanarType::P422, 16, 32>
{};
template <> // strange format type, only uses upper 10 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_10_3_PLANE_444>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_444, 3, PlanarType::P422, 16, 16, 16>
{};
template <> // strange format type, only uses upper 12 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_12_3_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_420, 3, PlanarType::P422, 16, 16, 16>
{};
template <> // strange format type, only uses upper 12 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_12_2_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_2_PLANE_420, 2, PlanarType::P420, 16, 32>
{};
template <> // strange format type, only uses upper 12 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_12_3_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 3, PlanarType::P422, 16, 16, 16>
{};
template <> // strange format type, only uses upper 12 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_12_2_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 2, PlanarType::P422, 16, 32>
{};
template <> // strange format type, only uses upper 12 bits of each 16 bit value...
struct FormatClassTraits<FormatClass::BIT_12_3_PLANE_444>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_444, 3, PlanarType::P422, 16, 16, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16_3_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_420, 3, PlanarType::P422, 16, 16, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16_2_PLANE_420>
  : FormatClassTraitPlanar<FormatClass::BIT_10_2_PLANE_420, 2, PlanarType::P420, 16, 32>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16_3_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 3, PlanarType::P422, 16, 16, 16>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16_2_PLANE_422>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_422, 2, PlanarType::P422, 16, 32>
{};
template <>
struct FormatClassTraits<FormatClass::BIT_16_3_PLANE_444>
  : FormatClassTraitPlanar<FormatClass::BIT_10_3_PLANE_444, 3, PlanarType::P422, 16, 16, 16>
{};

template <VkFormat>
struct FormatTrait;

#define DECL_FORMAT_TRAIT(type, fmt_class)                \
  template <>                                             \
  struct FormatTrait<type> : FormatClassTraits<fmt_class> \
  {                                                       \
    static constexpr VkFormat format = type;              \
  }

DECL_FORMAT_TRAIT(VK_FORMAT_R4G4_UNORM_PACK8, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_UNORM, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_SNORM, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_USCALED, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_SSCALED, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_UINT, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_SINT, FormatClass::BIT_8);
DECL_FORMAT_TRAIT(VK_FORMAT_R8_SRGB, FormatClass::BIT_8);

DECL_FORMAT_TRAIT(VK_FORMAT_R4G4B4A4_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_B4G4R4A4_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R5G6B5_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_B5G6R5_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R5G5B5A1_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_B5G5R5A1_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_A1R5G5B5_UNORM_PACK16, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_UNORM, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_SNORM, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_USCALED, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_SSCALED, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_UINT, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_SINT, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8_SRGB, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_UNORM, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_SNORM, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_USCALED, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_SSCALED, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_UINT, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_SINT, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R16_SFLOAT, FormatClass::BIT_16);

DECL_FORMAT_TRAIT(VK_FORMAT_R10X6_UNORM_PACK16_KHR, FormatClass::BIT_16);
DECL_FORMAT_TRAIT(VK_FORMAT_R12X4_UNORM_PACK16_KHR, FormatClass::BIT_16);

DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_UNORM, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_SNORM, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_USCALED, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_SSCALED, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_UINT, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_SINT, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8_SRGB, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_UNORM, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_SNORM, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_USCALED, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_SSCALED, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_UINT, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_SINT, FormatClass::BIT_24);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8_SRGB, FormatClass::BIT_24);

DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_UNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_SNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_USCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_SSCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_UINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_SINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R8G8B8A8_SRGB, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_UNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_SNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_USCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_SSCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_UINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_SINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8A8_SRGB, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_UNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_SNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_USCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_UINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_SINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A8B8G8R8_SRGB_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_UNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_SNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_USCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_UINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2R10G10B10_SINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_UNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_SNORM_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_USCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_UINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_A2B10G10R10_SINT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_UNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_SNORM, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_USCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_SSCALED, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_UINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_SINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16_SFLOAT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R32_UINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R32_SINT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R32_SFLOAT, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_B10G11R11_UFLOAT_PACK32, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, FormatClass::BIT_32);

DECL_FORMAT_TRAIT(VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR, FormatClass::BIT_32);
DECL_FORMAT_TRAIT(VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR, FormatClass::BIT_32);

DECL_FORMAT_TRAIT(VK_FORMAT_G8B8G8R8_422_UNORM_KHR, FormatClass::BIT_32_G8B8G8R8);
DECL_FORMAT_TRAIT(VK_FORMAT_B8G8R8G8_422_UNORM_KHR, FormatClass::BIT_32_B8G8R8G8);

DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_UNORM, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_SNORM, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_USCALED, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_SSCALED, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_UINT, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_SINT, FormatClass::BIT_48);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16_SFLOAT, FormatClass::BIT_48);

DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_UNORM, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_SNORM, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_USCALED, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_SSCALED, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_UINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_SINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R16G16B16A16_SFLOAT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32_UINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32_SINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32_SFLOAT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R64_UINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R64_SINT, FormatClass::BIT_64);
DECL_FORMAT_TRAIT(VK_FORMAT_R64_SFLOAT, FormatClass::BIT_64);

DECL_FORMAT_TRAIT(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR, FormatClass::BIT_64_R10G10B10A10);

DECL_FORMAT_TRAIT(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR, FormatClass::BIT_64_G10B10G10R10);

DECL_FORMAT_TRAIT(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR, FormatClass::BIT_64_B10G10R10G10);

DECL_FORMAT_TRAIT(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR, FormatClass::BIT_64_R12G12B12A12);

DECL_FORMAT_TRAIT(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR, FormatClass::BIT_64_G12B12G12R12);

DECL_FORMAT_TRAIT(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR, FormatClass::BIT_64_B12G12R12G12);

DECL_FORMAT_TRAIT(VK_FORMAT_G16B16G16R16_422_UNORM_KHR, FormatClass::BIT_64_G16B16G16R16);

DECL_FORMAT_TRAIT(VK_FORMAT_B16G16R16G16_422_UNORM_KHR, FormatClass::BIT_64_B16G16R16G16);

DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32_UINT, FormatClass::BIT_96);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32_SINT, FormatClass::BIT_96);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32_SFLOAT, FormatClass::BIT_96);

DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32A32_UINT, FormatClass::BIT_128);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32A32_SINT, FormatClass::BIT_128);
DECL_FORMAT_TRAIT(VK_FORMAT_R32G32B32A32_SFLOAT, FormatClass::BIT_128);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64_UINT, FormatClass::BIT_128);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64_SINT, FormatClass::BIT_128);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64_SFLOAT, FormatClass::BIT_128);

DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64_UINT, FormatClass::BIT_192);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64_SINT, FormatClass::BIT_192);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64_SFLOAT, FormatClass::BIT_192);

DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64A64_UINT, FormatClass::BIT_256);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64A64_SINT, FormatClass::BIT_256);
DECL_FORMAT_TRAIT(VK_FORMAT_R64G64B64A64_SFLOAT, FormatClass::BIT_256);

DECL_FORMAT_TRAIT(VK_FORMAT_BC1_RGB_UNORM_BLOCK, FormatClass::BC1_RGB);
DECL_FORMAT_TRAIT(VK_FORMAT_BC1_RGB_SRGB_BLOCK, FormatClass::BC1_RGB);

DECL_FORMAT_TRAIT(VK_FORMAT_BC1_RGBA_UNORM_BLOCK, FormatClass::BC1_RGBA);
DECL_FORMAT_TRAIT(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, FormatClass::BC1_RGBA);

DECL_FORMAT_TRAIT(VK_FORMAT_BC2_UNORM_BLOCK, FormatClass::BC2);
DECL_FORMAT_TRAIT(VK_FORMAT_BC2_SRGB_BLOCK, FormatClass::BC2);

DECL_FORMAT_TRAIT(VK_FORMAT_BC3_UNORM_BLOCK, FormatClass::BC3);
DECL_FORMAT_TRAIT(VK_FORMAT_BC3_SRGB_BLOCK, FormatClass::BC3);

DECL_FORMAT_TRAIT(VK_FORMAT_BC4_UNORM_BLOCK, FormatClass::BC4);
DECL_FORMAT_TRAIT(VK_FORMAT_BC4_SNORM_BLOCK, FormatClass::BC4);

DECL_FORMAT_TRAIT(VK_FORMAT_BC5_UNORM_BLOCK, FormatClass::BC5);
DECL_FORMAT_TRAIT(VK_FORMAT_BC5_SNORM_BLOCK, FormatClass::BC5);

DECL_FORMAT_TRAIT(VK_FORMAT_BC6H_UFLOAT_BLOCK, FormatClass::BC6H);
DECL_FORMAT_TRAIT(VK_FORMAT_BC6H_SFLOAT_BLOCK, FormatClass::BC6H);

DECL_FORMAT_TRAIT(VK_FORMAT_BC7_UNORM_BLOCK, FormatClass::BC7);
DECL_FORMAT_TRAIT(VK_FORMAT_BC7_SRGB_BLOCK, FormatClass::BC7);

DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, FormatClass::ETC2_RGB);
DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, FormatClass::ETC2_RGB);

DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, FormatClass::ETC2_RGBA);
DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, FormatClass::ETC2_RGBA);

DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, FormatClass::ETC2_EAC_RGBA);
DECL_FORMAT_TRAIT(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, FormatClass::ETC2_EAC_RGBA);

DECL_FORMAT_TRAIT(VK_FORMAT_EAC_R11_UNORM_BLOCK, FormatClass::EAC_R);
DECL_FORMAT_TRAIT(VK_FORMAT_EAC_R11_SNORM_BLOCK, FormatClass::EAC_R);

DECL_FORMAT_TRAIT(VK_FORMAT_EAC_R11G11_UNORM_BLOCK, FormatClass::EAC_RG);
DECL_FORMAT_TRAIT(VK_FORMAT_EAC_R11G11_SNORM_BLOCK, FormatClass::EAC_RG);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_4x4_UNORM_BLOCK, FormatClass::ASTC_4x4);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, FormatClass::ASTC_4x4);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_5x4_UNORM_BLOCK, FormatClass::ASTC_5x4);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_5x4_SRGB_BLOCK, FormatClass::ASTC_5x4);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_5x5_UNORM_BLOCK, FormatClass::ASTC_5x5);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_5x5_SRGB_BLOCK, FormatClass::ASTC_5x5);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_6x5_UNORM_BLOCK, FormatClass::ASTC_6x5);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_6x5_SRGB_BLOCK, FormatClass::ASTC_6x5);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_6x6_UNORM_BLOCK, FormatClass::ASTC_6x6);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_6x6_SRGB_BLOCK, FormatClass::ASTC_6x6);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x5_UNORM_BLOCK, FormatClass::ASTC_8x5);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x5_SRGB_BLOCK, FormatClass::ASTC_8x5);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x6_UNORM_BLOCK, FormatClass::ASTC_8x6);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x6_SRGB_BLOCK, FormatClass::ASTC_8x6);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x8_UNORM_BLOCK, FormatClass::ASTC_8x8);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_8x8_SRGB_BLOCK, FormatClass::ASTC_8x8);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x5_UNORM_BLOCK, FormatClass::ASTC_10x5);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x5_SRGB_BLOCK, FormatClass::ASTC_10x5);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x6_UNORM_BLOCK, FormatClass::ASTC_10x6);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x6_SRGB_BLOCK, FormatClass::ASTC_10x6);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x8_UNORM_BLOCK, FormatClass::ASTC_10x8);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x8_SRGB_BLOCK, FormatClass::ASTC_10x8);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x10_UNORM_BLOCK, FormatClass::ASTC_10x10);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_10x10_SRGB_BLOCK, FormatClass::ASTC_10x10);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_12x10_UNORM_BLOCK, FormatClass::ASTC_12x10);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_12x10_SRGB_BLOCK, FormatClass::ASTC_12x10);

DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_12x12_UNORM_BLOCK, FormatClass::ASTC_12x12);
DECL_FORMAT_TRAIT(VK_FORMAT_ASTC_12x12_SRGB_BLOCK, FormatClass::ASTC_12x12);

DECL_FORMAT_TRAIT(VK_FORMAT_D16_UNORM, FormatClass::D16);
DECL_FORMAT_TRAIT(VK_FORMAT_X8_D24_UNORM_PACK32, FormatClass::D24);
DECL_FORMAT_TRAIT(VK_FORMAT_D32_SFLOAT, FormatClass::D32);
DECL_FORMAT_TRAIT(VK_FORMAT_S8_UINT, FormatClass::S8);
DECL_FORMAT_TRAIT(VK_FORMAT_D16_UNORM_S8_UINT, FormatClass::D16S8);
DECL_FORMAT_TRAIT(VK_FORMAT_D24_UNORM_S8_UINT, FormatClass::D24S8);
DECL_FORMAT_TRAIT(VK_FORMAT_D32_SFLOAT_S8_UINT, FormatClass::D32S8);

DECL_FORMAT_TRAIT(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR, FormatClass::BIT_8_3_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR, FormatClass::BIT_8_2_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR, FormatClass::BIT_8_3_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR, FormatClass::BIT_8_2_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR, FormatClass::BIT_8_3_PLANE_444);
DECL_FORMAT_TRAIT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR, FormatClass::BIT_10_3_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR, FormatClass::BIT_10_2_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR, FormatClass::BIT_10_3_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR, FormatClass::BIT_10_2_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR, FormatClass::BIT_10_3_PLANE_444);
DECL_FORMAT_TRAIT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR, FormatClass::BIT_12_3_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR, FormatClass::BIT_12_2_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR, FormatClass::BIT_12_3_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR, FormatClass::BIT_12_2_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR, FormatClass::BIT_12_3_PLANE_444);
DECL_FORMAT_TRAIT(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR, FormatClass::BIT_16_3_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR, FormatClass::BIT_16_2_PLANE_420);
DECL_FORMAT_TRAIT(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR, FormatClass::BIT_16_3_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR, FormatClass::BIT_16_2_PLANE_422);
DECL_FORMAT_TRAIT(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR, FormatClass::BIT_16_3_PLANE_444);

#undef DECL_FORMAT_TRAIT