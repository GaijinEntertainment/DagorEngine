// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "format_store.h"
#include "format_traits.h"

#include <EASTL/algorithm.h>

using namespace drv3d_dx12;

template <typename... Ts>
struct TypePack;


template <typename... Formats>
struct FormatInfoTable;

template <typename T>
struct ExtractIndex
{
  static const uint32_t Value = T::Index;
};

template <typename T>
struct ExtractBaseFormat
{
  static const DXGI_FORMAT Value = T::BaseFormat;
};

template <typename T>
struct ExtractLinearFormat
{
  static const DXGI_FORMAT Value = T::LinearFormat;
};

template <typename T>
struct ExtractSrgbFormat
{
  static const DXGI_FORMAT Value = T::SrgbFormat;
};

template <uint32_t... List>
struct IndexList
{
  typedef IndexList<List...> Type;
};

template <uint32_t N, typename IntList>
struct PependIndexList;

template <uint32_t N, uint32_t... ints>
struct PependIndexList<N, IndexList<ints...>> : IndexList<N, ints...>
{};

template <typename T, T V>
struct IntegralConstant
{
  static constexpr T Value = V;
  typedef T ValueType;
  typedef IntegralConstant<T, V> Type; // using injected-class-name
};

template <DXGI_FORMAT... List>
struct FormatList
{
  static const uint32_t Length = sizeof...(List);
};

template <typename>
struct IsContinousList;

template <uint32_t A, uint32_t B, uint32_t... List>
struct IsContinousList<IndexList<A, B, List...>>
{
  static const bool Value = (A + 1 == B) && IsContinousList<IndexList<B, List...>>::Value;
};

template <uint32_t A>
struct IsContinousList<IndexList<A>>
{
  static const bool Value = true;
};

template <uint32_t, typename>
struct FirstIs;

template <uint32_t V, uint32_t A, uint32_t... List>
struct FirstIs<V, IndexList<A, List...>>
{
  static const bool Value = V == A;
};

template <uint32_t V, typename>
struct LastIs;

template <uint32_t V, uint32_t A, uint32_t B, uint32_t... List>
struct LastIs<V, IndexList<A, B, List...>>
{
  static const bool Value = LastIs<V, IndexList<B, List...>>::Value;
};

template <uint32_t V, uint32_t A>
struct LastIs<V, IndexList<A>>
{
  static const bool Value = V == A;
};

template <uint32_t id, DXGI_FORMAT base, DXGI_FORMAT linear, DXGI_FORMAT srgb,
  uint32_t channel_mask = FormatTrait<linear>::channel_mask, uint32_t block_size = FormatTrait<linear>::bits,
  uint32_t block_x = FormatTrait<linear>::block_width, uint32_t block_y = FormatTrait<linear>::block_height>
struct FormatInfo
{
  static const uint32_t ID = id;
  static const uint32_t Index = id >> FormatStore::CREATE_FLAGS_FORMAT_SHIFT;
  static const DXGI_FORMAT BaseFormat = base;
  static const DXGI_FORMAT LinearFormat = linear;
  static const DXGI_FORMAT SrgbFormat = srgb;
  static const uint32_t BlockBits = block_size;
  static const uint32_t BlockBytes = BlockBits / 8;
  static const uint32_t BlockX = block_x;
  static const uint32_t BlockY = block_y;
  static const uint32_t ChannelMask = channel_mask;
};

template <DXGI_FORMAT base, DXGI_FORMAT linear, DXGI_FORMAT srgb, uint8_t plane_count, uint32_t block_size = FormatTrait<linear>::bits,
  uint32_t block_x = FormatTrait<linear>::block_width, uint32_t block_y = FormatTrait<linear>::block_height>
struct ExtraFormatInfo
{
  template <uint32_t next_index>
  using FormatType =
    FormatInfo<next_index << FormatStore::CREATE_FLAGS_FORMAT_SHIFT, base, linear, srgb, plane_count, block_size, block_x, block_y>;
};

enum class ColorSpace : uint8_t
{
  UNDEFINED,
  LINEAR,
  SRGB,
};

#if _TARGET_PC_WIN
#define CONST_TABLE
#else
#define CONST_TABLE const
#endif

const char *dxgi_format_name(DXGI_FORMAT fmt);

template <typename... Formats>
struct FormatInfoTable
{
public:
  struct IndexInfo
  {
    uint8_t index;
    ColorSpace colorSpace;
  };
  struct BlockInfo
  {
    uint8_t size;
    uint8_t x;
    uint8_t y;
  };

  struct FormatSupport
  {
    D3D12_FORMAT_SUPPORT1 support1;
    D3D12_FORMAT_SUPPORT2 support2;
    uint32_t sampleCountSupportMask;
    uint32_t maxSampleCount;
  };

  typedef FormatInfoTable Type;

  typedef IndexList<ExtractIndex<Formats>::Value...> FormatsIndices;
  typedef FormatList<ExtractLinearFormat<Formats>::Value..., ExtractSrgbFormat<Formats>::Value...> DX12FormatList;

public:
  static CONST_TABLE DXGI_FORMAT undefinedFormatArray[sizeof...(Formats)];
  static CONST_TABLE DXGI_FORMAT linearFormatArray[sizeof...(Formats)];
  static CONST_TABLE DXGI_FORMAT srgbFormatArray[sizeof...(Formats)];
  static CONST_TABLE BlockInfo blockList[sizeof...(Formats)];
  static CONST_TABLE uint32_t channelMask[sizeof...(Formats)];

  static uint8_t planeCount[sizeof...(Formats)];
  static FormatSupport linearFormatSupportArray[sizeof...(Formats)];
  static FormatSupport srgbFormatSupportArray[sizeof...(Formats)];

  static inline bool isConfigured = false;

  static constexpr size_t formatCount = sizeof...(Formats);
  // are the indices currently strictly ordered?
  typedef IsContinousList<FormatsIndices> LinearIndexCheck;
  // check if first index is really 0
  typedef FirstIs<0, FormatsIndices> FirstCheck;
  // check if last index is format count - 1
  typedef LastIs<sizeof...(Formats) - 1, FormatsIndices> LastCheck;

public:
#if _TARGET_PC_WIN
  template <DXGI_FORMAT FB, DXGI_FORMAT FL, DXGI_FORMAT FS>
  static void remapTo(uint32_t index)
  {
    undefinedFormatArray[index] = FB;
    linearFormatArray[index] = FL;
    srgbFormatArray[index] = FS;
    blockList[index].size = FormatTrait<FB>::bits / 8;
    blockList[index].x = FormatTrait<FB>::block_width;
    blockList[index].y = FormatTrait<FB>::block_height;
  }
#endif

  static FormatSupport configureFormatSupport(ID3D12Device *device, DXGI_FORMAT dxgi_format)
  {
    D3D12_FEATURE_DATA_FORMAT_SUPPORT supportQuery = {dxgi_format};
    device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &supportQuery, sizeof(supportQuery));

    uint32_t sampleCountSupportMask = 1u;
    uint32_t maxSampleCount = 1u;
    if ((supportQuery.Support1 & D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET) != 0)
    {
      for (uint32_t sampleCount = get_sample_count(TEXCF_SAMPLECOUNT_MAX); sampleCount > 1; sampleCount /= 2)
      {
        G_ASSERTF(is_pow2(sampleCount), "Sample count should be power-of-two value. %u is provided", sampleCount);

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQualityLevels{dxgi_format};
        multisampleQualityLevels.SampleCount = sampleCount;

        auto result = device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, //
          &multisampleQualityLevels, sizeof(multisampleQualityLevels));

        bool isSampleCountSupported = (SUCCEEDED(result) && multisampleQualityLevels.NumQualityLevels > 0);
        if (isSampleCountSupported)
        {
          sampleCountSupportMask |= sampleCount;
          maxSampleCount = max(maxSampleCount, sampleCount);
        }
      }
    }

    return {supportQuery.Support1, supportQuery.Support2, sampleCountSupportMask, maxSampleCount};
  }

  static void configureTable(ID3D12Device *device, uint32_t vendor)
  {
#if _TARGET_PC_WIN
    static constexpr uint32_t AMD_VENDOR_ID = 0x1002;
    if (AMD_VENDOR_ID == vendor)
    {
      logdbg("DX12: Device vendor is AMD, assuming emulated D24S8, patching D24S8 to D32S8X24...");
      remapTo<DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT>(
        TEXFMT_DEPTH24 >> FormatStore::CREATE_FLAGS_FORMAT_SHIFT);
    }
#else
    G_UNUSED(vendor);
#endif

    logdbg("DX12: Configure planar format info table...");
    // Now patch format table with planar info, most formats have 1 but for depth stencil they can be different
    for (uint32_t i = 0; i < sizeof...(Formats); ++i)
    {
      D3D12_FEATURE_DATA_FORMAT_INFO fmtInfo = {linearFormatArray[i]};
      device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &fmtInfo, sizeof(fmtInfo));
      planeCount[i] = fmtInfo.PlaneCount;
      logdbg("DX12: %s PlaneCount %u", dxgi_format_name(linearFormatArray[i]), fmtInfo.PlaneCount);
    }

    logdbg("DX12: Configure format support info tables...");
    for (uint32_t i = 0; i < sizeof...(Formats); ++i)
    {
      linearFormatSupportArray[i] = configureFormatSupport(device, linearFormatArray[i]);
      srgbFormatSupportArray[i] = configureFormatSupport(device, srgbFormatArray[i]);
    }

    isConfigured = true;
  }

  static inline void checkTableConfigured() { G_ASSERTF(isConfigured, "Trying to access format table before 'configureTable' call"); }

  static inline void checkTableIndex(uint32_t index)
  {
    G_UNUSED(index);
    G_ASSERTF(index < sizeof...(Formats), "Invalid index %u into format table", index);
  }

  static DXGI_FORMAT getFormat(uint32_t index, ColorSpace color_space)
  {
    checkTableIndex(index);
    return color_space == ColorSpace::UNDEFINED ? undefinedFormatArray[index]
           : color_space == ColorSpace::SRGB    ? srgbFormatArray[index]
                                                : linearFormatArray[index];
  }

  static IndexInfo getIndex(DXGI_FORMAT format)
  {
    for (uint8_t i = 0; i < sizeof...(Formats); ++i)
      if (undefinedFormatArray[i] == format)
        return IndexInfo{i, ColorSpace::UNDEFINED};

    for (uint8_t i = 0; i < sizeof...(Formats); ++i)
      if (linearFormatArray[i] == format)
        return IndexInfo{i, ColorSpace::LINEAR};

    for (uint8_t i = 0; i < sizeof...(Formats); ++i)
      if (srgbFormatArray[i] == format)
        return IndexInfo{i, ColorSpace::SRGB};

    G_ASSERTF(false, "Can not find index for format %u", format);
    return IndexInfo{0, ColorSpace::UNDEFINED};
  }

  static BlockInfo getBlockInfo(uint32_t index)
  {
    checkTableIndex(index);
    return blockList[index];
  }

  static uint8_t getPlaneCount(uint32_t index)
  {
    checkTableConfigured();
    checkTableIndex(index);

    return planeCount[index];
  }

  static uint32_t getChannelMask(uint32_t index)
  {
    checkTableIndex(index);
    return channelMask[index];
  }

  static bool isInList(DXGI_FORMAT format)
  {
    for (auto &&fmt : undefinedFormatArray)
      if (format == fmt)
        return true;
    for (auto &&fmt : linearFormatArray)
      if (format == fmt)
        return true;
    for (auto &&fmt : srgbFormatArray)
      if (format == fmt)
        return true;
    return false;
  }

  static bool hasSrgbFormat(uint32_t index)
  {
    checkTableIndex(index);
    return linearFormatArray[index] != srgbFormatArray[index];
  }

  static FormatSupport getFormatSupport(uint32_t index, bool is_srgb)
  {
    checkTableConfigured();
    checkTableIndex(index);

    const FormatSupport &linearFormatSupport = linearFormatSupportArray[index];
    const FormatSupport &srgbFormatSupport = srgbFormatSupportArray[index];

    FormatSupport formatSupport = (is_srgb ? srgbFormatSupport : linearFormatSupport);
    if (is_srgb && hasSrgbFormat(index))
    {
      formatSupport.support1 |= linearFormatSupport.support1;
      formatSupport.support2 |= linearFormatSupport.support2;
    }

    return formatSupport;
  }
};


template <typename... Formats>
CONST_TABLE DXGI_FORMAT FormatInfoTable<Formats...>::undefinedFormatArray[sizeof...(Formats)] = {Formats::BaseFormat...};

template <typename... Formats>
CONST_TABLE DXGI_FORMAT FormatInfoTable<Formats...>::linearFormatArray[sizeof...(Formats)] = {Formats::LinearFormat...};
template <typename... Formats>
CONST_TABLE DXGI_FORMAT FormatInfoTable<Formats...>::srgbFormatArray[sizeof...(Formats)] = {Formats::SrgbFormat...};
template <typename... Formats>
CONST_TABLE typename FormatInfoTable<Formats...>::BlockInfo FormatInfoTable<Formats...>::blockList[sizeof...(Formats)] = {
  typename FormatInfoTable<Formats...>::BlockInfo{Formats::BlockBytes, Formats::BlockX, Formats::BlockY}...};

template <typename... Formats>
CONST_TABLE uint32_t FormatInfoTable<Formats...>::channelMask[sizeof...(Formats)] = //
  {Formats::ChannelMask...};

template <typename... Formats>
uint8_t FormatInfoTable<Formats...>::planeCount[sizeof...(Formats)];

template <typename... Formats>
typename FormatInfoTable<Formats...>::FormatSupport FormatInfoTable<Formats...>::linearFormatSupportArray[sizeof...(Formats)];

template <typename... Formats>
typename FormatInfoTable<Formats...>::FormatSupport FormatInfoTable<Formats...>::srgbFormatSupportArray[sizeof...(Formats)];

template <bool T, typename A, typename B>
struct SelectType
{
  typedef A Type;
};

template <typename A, typename B>
struct SelectType<false, A, B>
{
  typedef B Type;
};

#pragma warning(push)
// obviously the comparison is always true/false for a type
#pragma warning(disable : 4296)
template <typename A, typename B>
struct MinByIndex
{
  typedef typename SelectType<(A::Index < B::Index), A, B>::Type Type;
};

template <typename A, typename B>
struct MaxByIndex
{
  typedef typename SelectType<(A::Index < B::Index), B, A>::Type Type;
};
#pragma warning(pop)


template <typename...>
struct GetSmallestByIndex;

template <typename A, typename... L>
struct GetSmallestByIndex<A, L...>
{
  typedef typename GetSmallestByIndex<L...>::Type B;
  typedef typename MinByIndex<A, B>::Type Type;
};

template <typename A>
struct GetSmallestByIndex<A>
{
  typedef A Type;
};

template <typename A, typename B>
struct SameType
{
  static const bool Value = false;
};

template <typename A>
struct SameType<A, A>
{
  static const bool Value = true;
};

template <typename A>
struct PopFront;

template <typename A, typename... L>
struct PopFront<FormatInfoTable<A, L...>>
{
  typedef A Front;
  typedef FormatInfoTable<L...> Type;
};

template <typename A, typename B>
struct PushFront;

template <typename A, typename... L>
struct PushFront<A, FormatInfoTable<L...>>
{
  typedef FormatInfoTable<A, L...> Type;
};

template <typename, typename>
struct PushBack;

template <typename A, typename... L>
struct PushBack<A, FormatInfoTable<L...>>
{
  typedef FormatInfoTable<L..., A> Type;
};

template <DXGI_FORMAT base, DXGI_FORMAT linear, DXGI_FORMAT srgb, uint8_t plane_count, uint32_t block_size, uint32_t block_x,
  uint32_t block_y, typename... L>
struct PushBack<ExtraFormatInfo<base, linear, srgb, plane_count, block_size, block_x, block_y>, FormatInfoTable<L...>>
{
  typedef typename PushBack<typename ExtraFormatInfo<base, linear, srgb, plane_count, block_size, block_x,
                              block_y>::template FormatType<FormatInfoTable<L...>::formatCount>,
    FormatInfoTable<L...>>::Type Type;
};

template <typename, typename>
struct Concat;

template <typename A>
struct Concat<A, TypePack<>>
{
  typedef A Type;
};

template <typename A, typename B, typename... L>
struct Concat<A, TypePack<B, L...>>
{
  typedef typename Concat<typename PushBack<B, A>::Type, TypePack<L...>>::Type Type;
};

template <typename, typename>
struct RemoveType;

template <typename A, typename B, typename... L>
struct RemoveType<A, FormatInfoTable<B, L...>>
{
  typedef SameType<A, B> IsSameType;
  typedef typename RemoveType<A, FormatInfoTable<L...>>::Type TrueType;
  typedef typename PushFront<B, TrueType>::Type FalseType;
  typedef typename SelectType<IsSameType::Value, TrueType, FalseType>::Type Type;
};

template <typename A>
struct RemoveType<A, FormatInfoTable<>>
{
  typedef FormatInfoTable<> Type;
};

template <typename>
struct SortedFormatInfoTableImpl;

// sorting is done by finding the smallest element and putting it at pos 0 and then do the same with the rest
template <typename A, typename... Other>
struct SortedFormatInfoTableImpl<FormatInfoTable<A, Other...>>
{
  typedef typename GetSmallestByIndex<A, Other...>::Type BeginOfSet;
  typedef typename RemoveType<BeginOfSet, FormatInfoTable<A, Other...>>::Type SetWithoutBegin;
  typedef typename PushFront<BeginOfSet, typename SortedFormatInfoTableImpl<SetWithoutBegin>::Type>::Type Type;
};

template <typename A>
struct SortedFormatInfoTableImpl<FormatInfoTable<A>>
{
  typedef FormatInfoTable<A> Type;
};

template <typename>
struct SortedFormatInfoTable;

template <typename A, typename... N>
struct SortedFormatInfoTable<FormatInfoTable<A, N...>>
{
  typedef typename SortedFormatInfoTableImpl<FormatInfoTable<A, N...>>::Type Type;
};

// use DummyFormatInfo<index to fill with dummy> to add a dummy for unused slot to make the format table a contiguous list
template <uint32_t F>
using DummyFormatInfo = FormatInfo<F, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, 1, 0, 1, 1>;

// clang-format off
// this table can be in any order, SortedFormatInfoTable will sort them by TEXFMT_
typedef typename SortedFormatInfoTable<FormatInfoTable<
//          TEXFMT_*              Typeless format                     Linear format                     SRGB format
FormatInfo<TEXFMT_DEFAULT,        DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM_SRGB>,
FormatInfo<TEXFMT_A2R10G10B10,    DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM>,
FormatInfo<TEXFMT_A2B10G10R10,    DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM>,
FormatInfo<TEXFMT_A16B16G16R16,   DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UNORM,   DXGI_FORMAT_R16G16B16A16_UNORM>,
FormatInfo<TEXFMT_A16B16G16R16F,  DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT>,
FormatInfo<TEXFMT_A32B32G32R32F,  DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT>,
FormatInfo<TEXFMT_G16R16,         DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UNORM,         DXGI_FORMAT_R16G16_UNORM>,
FormatInfo<TEXFMT_G16R16F,        DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,         DXGI_FORMAT_R16G16_FLOAT>,
FormatInfo<TEXFMT_G32R32F,        DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,         DXGI_FORMAT_R32G32_FLOAT>,
FormatInfo<TEXFMT_R16F,           DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,            DXGI_FORMAT_R16_FLOAT>,
FormatInfo<TEXFMT_R32F,           DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_R32_FLOAT>,
FormatInfo<TEXFMT_DXT1,           DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM,            DXGI_FORMAT_BC1_UNORM_SRGB>,
FormatInfo<TEXFMT_DXT3,           DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM,            DXGI_FORMAT_BC2_UNORM_SRGB>,
FormatInfo<TEXFMT_DXT5,           DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM,            DXGI_FORMAT_BC3_UNORM_SRGB>,
FormatInfo<TEXFMT_R32G32UI,       DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,          DXGI_FORMAT_R32G32_UINT>,
DummyFormatInfo<0x0F000000U>,
FormatInfo<TEXFMT_L16,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,            DXGI_FORMAT_R16_UNORM>,
FormatInfo<TEXFMT_A8,             DXGI_FORMAT_A8_UNORM,               DXGI_FORMAT_A8_UNORM,             DXGI_FORMAT_A8_UNORM>,
FormatInfo<TEXFMT_R8,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UNORM>,
FormatInfo<TEXFMT_A1R5G5B5,       DXGI_FORMAT_B5G5R5A1_UNORM,         DXGI_FORMAT_B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM>,
FormatInfo<TEXFMT_A4R4G4B4,       DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM>,
FormatInfo<TEXFMT_R5G6B5,         DXGI_FORMAT_B5G6R5_UNORM,           DXGI_FORMAT_B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM>,
DummyFormatInfo<0x16000000U>,
FormatInfo<TEXFMT_A16B16G16R16S,  DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SNORM,   DXGI_FORMAT_R16G16B16A16_SNORM>,
FormatInfo<TEXFMT_A16B16G16R16UI, DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT>,
FormatInfo<TEXFMT_A32B32G32R32UI, DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT>,
FormatInfo<TEXFMT_ATI1N,          DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_UNORM,            DXGI_FORMAT_BC4_UNORM>,
FormatInfo<TEXFMT_ATI2N,          DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_UNORM,            DXGI_FORMAT_BC5_UNORM>,
FormatInfo<TEXFMT_R8G8B8A8,       DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM_SRGB>,
FormatInfo<TEXFMT_R32UI,          DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT>,
FormatInfo<TEXFMT_R32SI,          DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_SINT,             DXGI_FORMAT_R32_SINT>,
FormatInfo<TEXFMT_R11G11B10F,     DXGI_FORMAT_R11G11B10_FLOAT,        DXGI_FORMAT_R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT>,
FormatInfo<TEXFMT_R9G9B9E5,       DXGI_FORMAT_R9G9B9E5_SHAREDEXP,     DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   DXGI_FORMAT_R9G9B9E5_SHAREDEXP>,
FormatInfo<TEXFMT_R8G8,           DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,           DXGI_FORMAT_R8G8_UNORM>,
FormatInfo<TEXFMT_R8G8S,          DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SNORM,           DXGI_FORMAT_R8G8_SNORM>,
FormatInfo<TEXFMT_BC6H,           DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_UF16,            DXGI_FORMAT_BC6H_UF16>,
FormatInfo<TEXFMT_BC7,            DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM,            DXGI_FORMAT_BC7_UNORM_SRGB>,
FormatInfo<TEXFMT_R8UI,           DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_R8_UINT>,
FormatInfo<TEXFMT_R16UI,          DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,             DXGI_FORMAT_R16_UINT>,
FormatInfo<TEXFMT_DEPTH16,        DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_D16_UNORM,            DXGI_FORMAT_D16_UNORM>,
FormatInfo<TEXFMT_DEPTH32,        DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_D32_FLOAT,            DXGI_FORMAT_D32_FLOAT>,
FormatInfo<TEXFMT_DEPTH32_S8,     DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT>,
#if _TARGET_PC_WIN
FormatInfo<TEXFMT_DEPTH24,        DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_D24_UNORM_S8_UINT,    DXGI_FORMAT_D24_UNORM_S8_UINT>,
#else
// xbox is amd chip, no native d24 formats
FormatInfo<TEXFMT_DEPTH24,        DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT>,
#endif
DummyFormatInfo<0x2B000000U>,
DummyFormatInfo<0x2C000000U>,
DummyFormatInfo<0x2D000000U>,
DummyFormatInfo<TEXFMT_ASTC4>,
DummyFormatInfo<TEXFMT_ASTC8>,
DummyFormatInfo<TEXFMT_ASTC12>,
DummyFormatInfo<TEXFMT_ETC2_RG>,
DummyFormatInfo<TEXFMT_ETC2_RGBA>
>>::Type FormatInfoTableSet;
// clang-format on

template <typename T>
void check()
{
  // consistency check, if one of them complain, either the compiler does something wrong,
  // a index is not unique, or you have a set of formats that can not translated into an
  //  array where TEXFMT_ can be used as an index. Then you either need to rethink the
  // changes to TEXFMT_ or add a dummy for the unused slot.
  G_STATIC_ASSERT(T::LinearIndexCheck::Value);
  G_STATIC_ASSERT(T::FirstCheck::Value);
  G_STATIC_ASSERT(T::LastCheck::Value);
  G_STATIC_ASSERT(T::formatCount < (size_t(1) << (sizeof(FormatStore) * 8)));
}

FormatStore FormatStore::fromDXGIFormat(DXGI_FORMAT fmt)
{
  check<FormatInfoTableSet>();
  FormatStore result;

  FormatInfoTableSet::IndexInfo ii = FormatInfoTableSet::getIndex(fmt);
  result.isSrgb = ii.colorSpace == ColorSpace::SRGB;
  result.linearFormat = ii.index;
  return result;
}

FormatStore FormatStore::fromDXGIDepthFormat(DXGI_FORMAT fmt)
{
  FormatStore result;

  switch (fmt)
  {
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT: result.setFromFlagTexFlags(TEXFMT_DEPTH32); break;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM: result.setFromFlagTexFlags(TEXFMT_DEPTH16); break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24G8_TYPELESS: result.setFromFlagTexFlags(TEXFMT_DEPTH24); break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS: result.setFromFlagTexFlags(TEXFMT_DEPTH32_S8); break;
    default:
      DAG_FATAL("A texture with D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL set has a format of %s which we can't map to TEXFMT_*!",
        dxgi_format_name(fmt));
  }

  return result;
}

DXGI_FORMAT FormatStore::asDxGiTextureCreateFormat() const
{
  // R32F can be used both as float and uint, so it should be created typeless.
  if (linearFormat == (TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT))
    return asDxGiBaseFormat();
  // For block compressed, we use the requested specific format
  if (isBlockCompressed())
    return asDxGiFormat();
  // For depth (and stencil) or when srgb is requested and the format has a
  // srgb variation, we use the base format to allow views of different formats.
  if ((isSrgbCapableFormatType() && isSrgb) || isDepth())
    return asDxGiBaseFormat();
  // For any other format we use linear.
  return asLinearDxGiFormat();
}

DXGI_FORMAT FormatStore::asDxGiBaseFormat() const
{
  check<FormatInfoTableSet>();
  return FormatInfoTableSet::getFormat(linearFormat, ColorSpace::UNDEFINED);
}

DXGI_FORMAT FormatStore::asDxGiFormat() const
{
  return FormatInfoTableSet::getFormat(linearFormat, isSrgb ? ColorSpace::SRGB : ColorSpace::LINEAR);
}

DXGI_FORMAT FormatStore::asLinearDxGiFormat() const { return FormatInfoTableSet::getFormat(linearFormat, ColorSpace::LINEAR); }

DXGI_FORMAT FormatStore::asSrgbDxGiFormat() const { return FormatInfoTableSet::getFormat(linearFormat, ColorSpace::SRGB); }

bool FormatStore::isSrgbCapableFormatType() const { return FormatInfoTableSet::hasSrgbFormat(linearFormat); }

bool FormatStore::isHDRFormat() const
{
  switch (linearFormat)
  {
    // HDR
    case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    default:
    // has no full RGB channel set
    case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32G32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32SI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
    // is compressed
    case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC6H >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC7 >> CREATE_FLAGS_FORMAT_SHIFT:
    // SDR
    case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    // depth / stencil
    case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return false;
  }
}

bool FormatStore::isColor() const
{
  switch (linearFormat)
  {
    default: return false;
    case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32G32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32SI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC6H >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC7 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16UI >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return false;
  }
}

bool FormatStore::isDepth() const
{
  switch (linearFormat)
  {
    default: return false;
    case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32G32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32SI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC6H >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC7 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16UI >> CREATE_FLAGS_FORMAT_SHIFT: return false;
    case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
  }
}

bool FormatStore::isStencil() const
{
  //-V::1037
  switch (linearFormat)
  {
    default: return false;
    case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32G32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A16B16G16R16UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_A32B32G32R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R32SI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC6H >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_BC7 >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R8UI >> CREATE_FLAGS_FORMAT_SHIFT:
    case TEXFMT_R16UI >> CREATE_FLAGS_FORMAT_SHIFT: return false;
    case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT: return false;
    case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT: return false;
    case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
  }
}

bool FormatStore::isBlockCompressed() const
{
  FormatInfoTableSet::BlockInfo bi = FormatInfoTableSet::getBlockInfo(linearFormat);
  return (bi.x > 1) || (bi.y > 1);
}

uint32_t FormatStore::getBytesPerPixelBlock(uint32_t *block_x, uint32_t *block_y) const
{
  FormatInfoTableSet::BlockInfo bi = FormatInfoTableSet::getBlockInfo(linearFormat);

  if (block_x)
    *block_x = bi.x;
  if (block_y)
    *block_y = bi.y;

  return bi.size;
}

const char *FormatStore::getNameString() const { return dxgi_format_name(asDxGiFormat()); }

bool FormatStore::canBeStored(DXGI_FORMAT fmt) { return FormatInfoTableSet::isInList(fmt); }

void FormatStore::configureFormatTable(ID3D12Device *device, uint32_t vendor) { FormatInfoTableSet::configureTable(device, vendor); }

FormatPlaneCount FormatStore::getPlanes() const { return FormatPlaneCount::make(FormatInfoTableSet::getPlaneCount(linearFormat)); }

uint32_t FormatStore::getChannelMask() const { return FormatInfoTableSet::getChannelMask(linearFormat); }

bool FormatStore::isSupported(D3D12_FORMAT_SUPPORT1 flags) const
{
  return (FormatInfoTableSet::getFormatSupport(linearFormat, isSrgb).support1 & flags) == flags;
}

bool FormatStore::isSupported(D3D12_FORMAT_SUPPORT2 flags) const
{
  return (FormatInfoTableSet::getFormatSupport(linearFormat, isSrgb).support2 & flags) == flags;
}

bool FormatStore::isSupportedTexture2D() const { return isSupported(D3D12_FORMAT_SUPPORT1_TEXTURE2D); }

bool FormatStore::isSupportedTexture3D() const { return isSupported(D3D12_FORMAT_SUPPORT1_TEXTURE3D); }

bool FormatStore::isSupportedTextureCube() const { return isSupported(D3D12_FORMAT_SUPPORT1_TEXTURECUBE); }

bool FormatStore::isSupportedDepthStencil() const { return isSupported(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL); }

bool FormatStore::isSupportedRenderTarget() const { return isSupported(D3D12_FORMAT_SUPPORT1_RENDER_TARGET); }

bool FormatStore::isSupportedMultisampleRenderTarget() const { return isSupported(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET); }

bool FormatStore::isSupportedMultisampleResolve() const { return isSupported(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE); }

bool FormatStore::isSupportedMultisampleLoad() const { return isSupported(D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD); }

bool FormatStore::isSupportedBlendable() const { return isSupported(D3D12_FORMAT_SUPPORT1_BLENDABLE); }

bool FormatStore::isSupportedShaderSample() const { return isSupported(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE); }

bool FormatStore::isSupportedTypedUav() const { return isSupported(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW); }

bool FormatStore::isSupportedUavTypedLoad() const { return isSupported(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD); }

bool FormatStore::isSupportedTiled() const { return isSupported(D3D12_FORMAT_SUPPORT2_TILED); }

bool FormatStore::isSampleCountSupported(int32_t sampleCount) const
{
  G_ASSERTF(is_pow2(sampleCount), "Sample count should be power-of-two value. %u is provided", sampleCount);

  uint32_t sampleCountSupportMask = FormatInfoTableSet::getFormatSupport(linearFormat, isSrgb).sampleCountSupportMask;
  return (sampleCountSupportMask & (uint32_t)sampleCount) != 0;
}

int32_t FormatStore::getMaxSampleCount() const
{
  return (int32_t)FormatInfoTableSet::getFormatSupport(linearFormat, isSrgb).maxSampleCount;
}

bool FormatStore::isCopyConvertible(FormatStore other) const
{
  // Same formats can always be copied (even if the differ in SRGB or not).
  if (linearFormat == other.linearFormat)
  {
    return true;
  }
  auto thisBaseFormat = asDxGiBaseFormat();
  auto otherBaseFormat = other.asDxGiBaseFormat();
  // When the base format is the same we can always safely copy it
  if (thisBaseFormat == otherBaseFormat)
  {
    return true;
  }

  auto thisLinearFormat = asLinearDxGiFormat();
  auto otherLinearFormat = other.asLinearDxGiFormat();

  struct FormatCopyConversionPair
  {
    std::initializer_list<const DXGI_FORMAT> from;
    std::initializer_list<const DXGI_FORMAT> to;
  };

  // Table from:
  // https://learn.microsoft.com/en-us/windows/win32/direct3d10/
  // d3d10-graphics-programming-guide-resources-block-compression#format-conversion-using-direct3d-101
  //
  // Added BC6H and BC7 formats to 128 block
  static const FormatCopyConversionPair format_copy_conversion_pair_table[] = //
    {
      // 32 block
      {{DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_SINT}, {DXGI_FORMAT_R9G9B9E5_SHAREDEXP}}, //
                                                                                        // 64 block
      {{DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_SINT},
        {DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB, DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM}}, //
                                                                                                            // 128 block
      {{DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_SINT},
        {DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB, DXGI_FORMAT_BC5_UNORM,
          DXGI_FORMAT_BC5_SNORM, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16, DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB}}, //
    };

  for (auto &e : format_copy_conversion_pair_table)
  {
    if (end(e.from) != eastl::find(begin(e.from), end(e.from), thisLinearFormat))
    {
      if (end(e.to) != eastl::find(begin(e.to), end(e.to), otherLinearFormat))
      {
        return true;
      }
    }
    else if (end(e.to) != eastl::find(begin(e.to), end(e.to), thisLinearFormat))
    {
      if (end(e.from) != eastl::find(begin(e.from), end(e.from), otherLinearFormat))
      {
        return true;
      }
    }
  }

  return false;
}

const char *dxgi_format_name(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
#define FORMAT_TO_NAME(fmt) \
  case fmt: return #fmt
    FORMAT_TO_NAME(DXGI_FORMAT_UNKNOWN);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32A32_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32A32_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32A32_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32A32_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32B32_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16B16A16_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G32_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32G8X24_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R10G10B10A2_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R10G10B10A2_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R10G10B10A2_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R11G11B10_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8B8A8_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16G16_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_D32_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R32_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R24G8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_D24_UNORM_S8_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_FLOAT);
    FORMAT_TO_NAME(DXGI_FORMAT_D16_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R16_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_R8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8_UINT);
    FORMAT_TO_NAME(DXGI_FORMAT_R8_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R8_SINT);
    FORMAT_TO_NAME(DXGI_FORMAT_A8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R1_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
    FORMAT_TO_NAME(DXGI_FORMAT_R8G8_B8G8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_G8R8_G8B8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC1_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC1_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC1_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_BC2_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC2_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC2_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_BC3_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC3_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC3_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_BC4_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC4_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC4_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC5_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC5_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC5_SNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_B5G6R5_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_B5G5R5A1_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8A8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8X8_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8A8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8X8_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
    FORMAT_TO_NAME(DXGI_FORMAT_BC6H_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC6H_UF16);
    FORMAT_TO_NAME(DXGI_FORMAT_BC6H_SF16);
    FORMAT_TO_NAME(DXGI_FORMAT_BC7_TYPELESS);
    FORMAT_TO_NAME(DXGI_FORMAT_BC7_UNORM);
    FORMAT_TO_NAME(DXGI_FORMAT_BC7_UNORM_SRGB);
/*
FORMAT_TO_NAME(DXGI_FORMAT_AYUV);
FORMAT_TO_NAME(DXGI_FORMAT_Y410);
FORMAT_TO_NAME(DXGI_FORMAT_Y416);
FORMAT_TO_NAME(DXGI_FORMAT_NV12);
FORMAT_TO_NAME(DXGI_FORMAT_P010);
FORMAT_TO_NAME(DXGI_FORMAT_P016);
FORMAT_TO_NAME(DXGI_FORMAT_420_OPAQUE);
FORMAT_TO_NAME(DXGI_FORMAT_YUY2);
FORMAT_TO_NAME(DXGI_FORMAT_Y210);
FORMAT_TO_NAME(DXGI_FORMAT_Y216);
FORMAT_TO_NAME(DXGI_FORMAT_NV11);
FORMAT_TO_NAME(DXGI_FORMAT_AI44);
FORMAT_TO_NAME(DXGI_FORMAT_IA44);
FORMAT_TO_NAME(DXGI_FORMAT_P8);
FORMAT_TO_NAME(DXGI_FORMAT_A8P8);
FORMAT_TO_NAME(DXGI_FORMAT_B4G4R4A4_UNORM);
FORMAT_TO_NAME(DXGI_FORMAT_P208);
FORMAT_TO_NAME(DXGI_FORMAT_V208);
FORMAT_TO_NAME(DXGI_FORMAT_V408);
*/
#undef FORMAT_TO_NAME
    default: return "missing format to name";
  }
}
