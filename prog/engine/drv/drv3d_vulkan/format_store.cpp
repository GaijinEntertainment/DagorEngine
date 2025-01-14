// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "format_store.h"

#include <debug/dag_assert.h>

#include "format_traits.h"
#include "util/type_lists.h"

using namespace drv3d_vulkan;


template <typename... Formats>
struct FormatInfoTable;

template <typename T>
struct ExtractIndex
{
  static const uint32_t Value = T::Index;
};

template <typename T>
struct ExtractLinearFormat
{
  static const VkFormat Value = T::LinearFormat;
};

template <typename T>
struct ExtractSrgbFormat
{
  static const VkFormat Value = T::SrgbFormat;
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

template <VkFormat... List>
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

template <uint32_t id, VkFormat linear, VkFormat srgb, uint32_t block_size = FormatTrait<linear>::bits,
  uint32_t block_x = FormatTrait<linear>::block_width, uint32_t block_y = FormatTrait<linear>::block_height>
struct FormatInfo
{
  static const uint32_t ID = id;
  static const uint32_t Index = id >> FormatStore::CREATE_FLAGS_FORMAT_SHIFT;
  static const VkFormat LinearFormat = linear;
  static const VkFormat SrgbFormat = srgb;
  static const uint32_t BlockBits = block_size;
  static const uint32_t BlockBytes = BlockBits / 8;
  static const uint32_t BlockX = block_x;
  static const uint32_t BlockY = block_y;
};

template <VkFormat linear, VkFormat srgb, uint32_t block_size = FormatTrait<linear>::bits,
  uint32_t block_x = FormatTrait<linear>::block_width, uint32_t block_y = FormatTrait<linear>::block_height>
struct ExtraFormatInfo
{
  template <uint32_t next_index>
  using FormatType = FormatInfo<next_index << FormatStore::CREATE_FLAGS_FORMAT_SHIFT, linear, srgb, block_size, block_x, block_y>;
};

template <typename... Formats>
struct FormatInfoTable
{
public:
  struct IndexInfo
  {
    uint8_t index;
    bool linear;
  };
  struct BlockInfo
  {
    uint8_t size;
    uint8_t x;
    uint8_t y;
  };

  typedef FormatInfoTable Type;

  typedef IndexList<ExtractIndex<Formats>::Value...> FormatsIndices;
  typedef FormatList<ExtractLinearFormat<Formats>::Value..., ExtractSrgbFormat<Formats>::Value...> VulkanFormatList;

public:
  static VkFormat linearFormatArray[sizeof...(Formats)];
  static VkFormat srgbFormatArray[sizeof...(Formats)];
  static BlockInfo blockList[sizeof...(Formats)];
  static constexpr size_t formatCount = sizeof...(Formats);
  // are the indices currently strictly ordered?
  typedef IsContinousList<FormatsIndices> LinearIndexCheck;
  // check if first index is really 0
  typedef FirstIs<0, FormatsIndices> FirstCheck;
  // check if last index is format count - 1
  typedef LastIs<sizeof...(Formats) - 1, FormatsIndices> LastCheck;

public:
  template <VkFormat FL, VkFormat FS>
  static void remapTo(uint32_t index)
  {
    linearFormatArray[index] = FL;
    srgbFormatArray[index] = FS;
    blockList[index].size = FormatTrait<FL>::bits / 8;
    blockList[index].x = FormatTrait<FL>::block_width;
    blockList[index].y = FormatTrait<FL>::block_height;
  }
  static VkFormat getFormat(uint32_t index, bool srgb)
  {
    G_ASSERTF(index < sizeof...(Formats), "Invalid index %u into format table", index);
    return srgb ? srgbFormatArray[index] : linearFormatArray[index];
  }

  static IndexInfo getIndex(VkFormat format)
  {
    for (uint8_t i = 0; i < sizeof...(Formats); ++i)
      if (linearFormatArray[i] == format)
        return IndexInfo{i, true};

    for (uint8_t i = 0; i < sizeof...(Formats); ++i)
      if (srgbFormatArray[i] == format)
        return IndexInfo{i, false};
    G_ASSERTF(false, "Can not find index for format %u", format);
    return IndexInfo{0, false};
  }

  static BlockInfo getBlockInfo(uint32_t index)
  {
    G_ASSERTF(index < sizeof...(Formats), "Invalid index %u into format table", index);
    return blockList[index];
  }

  static bool isInList(VkFormat format)
  {
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
    G_ASSERTF(index < sizeof...(Formats), "Invalid index %u into format table", index);
    return linearFormatArray[index] != srgbFormatArray[index];
  }
};


template <typename... Formats>
VkFormat FormatInfoTable<Formats...>::linearFormatArray[sizeof...(Formats)] = {Formats::LinearFormat...};
template <typename... Formats>
VkFormat FormatInfoTable<Formats...>::srgbFormatArray[sizeof...(Formats)] = {Formats::SrgbFormat...};
template <typename... Formats>
typename FormatInfoTable<Formats...>::BlockInfo FormatInfoTable<Formats...>::blockList[sizeof...(Formats)] = {
  typename FormatInfoTable<Formats...>::BlockInfo{Formats::BlockBytes, Formats::BlockX, Formats::BlockY}...};

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

#ifdef _MSC_VER
#pragma warning(push)
// obviously the comparison is always true/false for a type
#pragma warning(disable : 4296)
#endif
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
#ifdef _MSC_VER
#pragma warning(pop)
#endif


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

template <VkFormat linear, VkFormat srgb, uint32_t block_size, uint32_t block_x, uint32_t block_y, typename... L>
struct PushBack<ExtraFormatInfo<linear, srgb, block_size, block_x, block_y>, FormatInfoTable<L...>>
{
  typedef typename PushBack<
    typename ExtraFormatInfo<linear, srgb, block_size, block_x, block_y>::template FormatType<FormatInfoTable<L...>::formatCount>,
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
using DummyFormatInfo = FormatInfo<F, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, 0, 1, 1>;


// this table can be in any order, SortedFormatInfoTable will sort them by TEXFMT_
// clang-format off
typedef typename SortedFormatInfoTable<FormatInfoTable<
  FormatInfo<TEXFMT_DEFAULT,        VK_FORMAT_B8G8R8A8_UNORM,            VK_FORMAT_B8G8R8A8_SRGB>,
  FormatInfo<TEXFMT_A2R10G10B10,    VK_FORMAT_A2B10G10R10_UNORM_PACK32,  VK_FORMAT_A2B10G10R10_UNORM_PACK32>,
  FormatInfo<TEXFMT_A2B10G10R10,    VK_FORMAT_A2B10G10R10_UNORM_PACK32,  VK_FORMAT_A2B10G10R10_UNORM_PACK32>,
  FormatInfo<TEXFMT_A16B16G16R16,   VK_FORMAT_R16G16B16A16_UNORM,        VK_FORMAT_R16G16B16A16_UNORM>,
  FormatInfo<TEXFMT_A16B16G16R16F,  VK_FORMAT_R16G16B16A16_SFLOAT,       VK_FORMAT_R16G16B16A16_SFLOAT>,
  FormatInfo<TEXFMT_A32B32G32R32F,  VK_FORMAT_R32G32B32A32_SFLOAT,       VK_FORMAT_R32G32B32A32_SFLOAT>,
  FormatInfo<TEXFMT_G16R16,         VK_FORMAT_R16G16_UNORM,              VK_FORMAT_R16G16_UNORM>,
  FormatInfo<TEXFMT_G16R16F,        VK_FORMAT_R16G16_SFLOAT,             VK_FORMAT_R16G16_SFLOAT>,
  FormatInfo<TEXFMT_G32R32F,        VK_FORMAT_R32G32_SFLOAT,             VK_FORMAT_R32G32_SFLOAT>,
  FormatInfo<TEXFMT_R16F,           VK_FORMAT_R16_SFLOAT,                VK_FORMAT_R16_SFLOAT>,
  FormatInfo<TEXFMT_R32F,           VK_FORMAT_R32_SFLOAT,                VK_FORMAT_R32_SFLOAT>,
  FormatInfo<TEXFMT_DXT1,           VK_FORMAT_BC1_RGBA_UNORM_BLOCK,      VK_FORMAT_BC1_RGBA_SRGB_BLOCK>,
  FormatInfo<TEXFMT_DXT3,           VK_FORMAT_BC2_UNORM_BLOCK,           VK_FORMAT_BC2_SRGB_BLOCK>,
  FormatInfo<TEXFMT_DXT5,           VK_FORMAT_BC3_UNORM_BLOCK,           VK_FORMAT_BC3_SRGB_BLOCK>,
  FormatInfo<TEXFMT_R32G32UI,       VK_FORMAT_R32G32_UINT,               VK_FORMAT_R32G32_UINT>,
  DummyFormatInfo<0x0F000000U>,
  FormatInfo<TEXFMT_L16,            VK_FORMAT_R16_UNORM,                 VK_FORMAT_R16_UNORM>,
  FormatInfo<TEXFMT_A8,             VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM>,
  FormatInfo<TEXFMT_R8,             VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM>,
  FormatInfo<TEXFMT_A1R5G5B5,       VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16>,
  FormatInfo<TEXFMT_A4R4G4B4,       VK_FORMAT_B4G4R4A4_UNORM_PACK16,     VK_FORMAT_B4G4R4A4_UNORM_PACK16>,
  FormatInfo<TEXFMT_R5G6B5,         VK_FORMAT_R5G6B5_UNORM_PACK16,       VK_FORMAT_R5G6B5_UNORM_PACK16>,
  DummyFormatInfo<0x16000000U>,
  FormatInfo<TEXFMT_A16B16G16R16S,  VK_FORMAT_R16G16B16A16_SNORM,        VK_FORMAT_R16G16B16A16_SNORM>,
  FormatInfo<TEXFMT_A16B16G16R16UI, VK_FORMAT_R16G16B16A16_UINT,         VK_FORMAT_R16G16B16A16_UINT>,
  FormatInfo<TEXFMT_A32B32G32R32UI, VK_FORMAT_R32G32B32A32_UINT,         VK_FORMAT_R32G32B32A32_UINT>,
  FormatInfo<TEXFMT_ATI1N,          VK_FORMAT_BC4_UNORM_BLOCK,           VK_FORMAT_BC4_UNORM_BLOCK>,
  FormatInfo<TEXFMT_ATI2N,          VK_FORMAT_BC5_UNORM_BLOCK,           VK_FORMAT_BC5_UNORM_BLOCK>,
  FormatInfo<TEXFMT_R8G8B8A8,       VK_FORMAT_A8B8G8R8_UNORM_PACK32,     VK_FORMAT_A8B8G8R8_SRGB_PACK32>,
  FormatInfo<TEXFMT_R32UI,          VK_FORMAT_R32_UINT,                  VK_FORMAT_R32_UINT>,
  FormatInfo<TEXFMT_R32SI,          VK_FORMAT_R32_SINT,                  VK_FORMAT_R32_SINT>,
  FormatInfo<TEXFMT_R11G11B10F,     VK_FORMAT_B10G11R11_UFLOAT_PACK32,   VK_FORMAT_B10G11R11_UFLOAT_PACK32>,
  FormatInfo<TEXFMT_R9G9B9E5,       VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32>,
  FormatInfo<TEXFMT_R8G8,           VK_FORMAT_R8G8_UNORM,                VK_FORMAT_R8G8_UNORM>,
  FormatInfo<TEXFMT_R8G8S,          VK_FORMAT_R8G8_SNORM,                VK_FORMAT_R8G8_SNORM>,
  FormatInfo<TEXFMT_BC6H,           VK_FORMAT_BC6H_UFLOAT_BLOCK,         VK_FORMAT_BC6H_UFLOAT_BLOCK>,
  FormatInfo<TEXFMT_BC7,            VK_FORMAT_BC7_UNORM_BLOCK,           VK_FORMAT_BC7_SRGB_BLOCK>,
  FormatInfo<TEXFMT_R8UI,           VK_FORMAT_R8_UINT,                   VK_FORMAT_R8_UINT>,
  FormatInfo<TEXFMT_R16UI,          VK_FORMAT_R16_UINT,                  VK_FORMAT_R16_UINT>,
  FormatInfo<TEXFMT_DEPTH24,        VK_FORMAT_D24_UNORM_S8_UINT,         VK_FORMAT_D24_UNORM_S8_UINT>,
  FormatInfo<TEXFMT_DEPTH16,        VK_FORMAT_D16_UNORM,                 VK_FORMAT_D16_UNORM>,
  FormatInfo<TEXFMT_DEPTH32,        VK_FORMAT_D32_SFLOAT,                VK_FORMAT_D32_SFLOAT>,
  FormatInfo<TEXFMT_DEPTH32_S8,     VK_FORMAT_D32_SFLOAT_S8_UINT,        VK_FORMAT_D32_SFLOAT_S8_UINT>,
  FormatInfo<TEXFMT_ASTC4,          VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      VK_FORMAT_ASTC_4x4_SRGB_BLOCK>,
  FormatInfo<TEXFMT_ASTC8,          VK_FORMAT_ASTC_8x8_UNORM_BLOCK,      VK_FORMAT_ASTC_8x8_SRGB_BLOCK>,
  FormatInfo<TEXFMT_ASTC12,         VK_FORMAT_ASTC_12x12_UNORM_BLOCK,    VK_FORMAT_ASTC_12x12_SRGB_BLOCK>,
  /*stub*/ DummyFormatInfo<0x2B000000U>, DummyFormatInfo<0x2C000000U>, DummyFormatInfo<0x2D000000U>,
  FormatInfo<TEXFMT_ETC2_RG,        VK_FORMAT_EAC_R11G11_UNORM_BLOCK,    VK_FORMAT_EAC_R11G11_SNORM_BLOCK>,
  FormatInfo<TEXFMT_ETC2_RGBA,      VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK>
  >>::Type BaseFormatInfoTableSet;
// clang-format on

// add here all formats that the d3d stuff does not need to know but we need to map it back
// to our format table somehow
// prime examples are formats for the swapchain that are not covered by TEXFMT_*
typedef TypePack<
  // abgr8 for mobile/nswitch
  ExtraFormatInfo<VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB>>
  SwapchainExtraFormats;

typedef typename Concat<BaseFormatInfoTableSet, SwapchainExtraFormats>::Type FormatInfoTableSet;

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

VkFormat FormatStore::asVkFormat() const
{
  check<FormatInfoTableSet>();
  return FormatInfoTableSet::getFormat(linearFormat, isSrgb != 0);
}

FormatStore FormatStore::fromVkFormat(VkFormat fmt)
{
  FormatStore result;

  FormatInfoTableSet::IndexInfo ii = FormatInfoTableSet::getIndex(fmt);
  result.isSrgb = !ii.linear;
  result.linearFormat = ii.index;
  return result;
}

VkFormat FormatStore::getLinearFormat() const { return FormatInfoTableSet::getFormat(linearFormat, false); }

VkFormat FormatStore::getSRGBFormat() const { return FormatInfoTableSet::getFormat(linearFormat, true); }

bool FormatStore::isSrgbCapableFormatType() const { return FormatInfoTableSet::hasSrgbFormat(linearFormat); }

bool FormatStore::isBlockFormat() const
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

const char *FormatStore::getNameString() const
{
  // a switch over every format vulkan supports,
  // so that we only need to update if vulkan supports
  // new formats. This is fine, this function is for
  // debug only and can be slow.
  switch (asVkFormat())
  {
#define FORMAT_TO_NAME(fmt) \
  case fmt: return #fmt
    FORMAT_TO_NAME(VK_FORMAT_UNDEFINED);
    FORMAT_TO_NAME(VK_FORMAT_R4G4_UNORM_PACK8);
    FORMAT_TO_NAME(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_R5G6B5_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_B5G6R5_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
    FORMAT_TO_NAME(VK_FORMAT_R8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R8G8B8A8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_SINT);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8A8_SRGB);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_UINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_SINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_UINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2R10G10B10_SINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_UINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_A2B10G10R10_SINT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_R16_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R16_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R16_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_SNORM);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_USCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_SSCALED);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R16G16B16A16_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R32_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R32_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R32_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32A32_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32A32_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R32G32B32A32_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R64_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R64_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R64_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64A64_UINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64A64_SINT);
    FORMAT_TO_NAME(VK_FORMAT_R64G64B64A64_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_D16_UNORM);
    FORMAT_TO_NAME(VK_FORMAT_X8_D24_UNORM_PACK32);
    FORMAT_TO_NAME(VK_FORMAT_D32_SFLOAT);
    FORMAT_TO_NAME(VK_FORMAT_S8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_D16_UNORM_S8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_D24_UNORM_S8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_D32_SFLOAT_S8_UINT);
    FORMAT_TO_NAME(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC2_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC2_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC3_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC3_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC4_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC4_SNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC5_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC5_SNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC6H_UFLOAT_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC6H_SFLOAT_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC7_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_BC7_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_EAC_R11_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_EAC_R11_SNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
    FORMAT_TO_NAME(VK_FORMAT_G8B8G8R8_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_B8G8R8G8_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R10X6_UNORM_PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R12X4_UNORM_PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16B16G16R16_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_B16G16R16G16_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR);
    FORMAT_TO_NAME(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR);
#undef FORMAT_TO_NAME
    default: return "missing format to name";
  }
}

bool FormatStore::canBeStored(VkFormat fmt) { return FormatInfoTableSet::isInList(fmt); }

void FormatStore::setRemapDepth24BitsToDepth32Bits(bool enable)
{
  if (enable)
  {
    FormatInfoTableSet::remapTo<VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT>(
      TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT);
  }
  else
  {
    FormatInfoTableSet::remapTo<VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT>(TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT);
  }
}
