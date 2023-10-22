#pragma once

#if _TARGET_PC_WIN
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgitype.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#endif
#include <3d/dag_drv3d.h>

#include <atomic>

#include <EASTL/unique_ptr.h>
#include <EASTL/type_traits.h>
#include <EASTL/vector.h>
#include <EASTL/bitset.h>
#include <EASTL/span.h>

#include <debug/dag_assert.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <perfMon/dag_graphStat.h>

#include <util/dag_string.h>

#include "drvCommonConsts.h"

#include <comPtr/comPtr.h>

#include "constants.h"

#include "bitfield.h"
#include "value_range.h"
#include "versioned_com_ptr.h"
#include "drv_log_defs.h"


#if _TARGET_XBOX
#include "driver_xbox.h"
#endif


#if _TARGET_PC_WIN
typedef IDXGISwapChain3 DXGISwapChain;
typedef IDXGIFactory4 DXGIFactory;
typedef IDXGIAdapter4 DXGIAdapter;
typedef ID3D12Device3 D3DDevice;
typedef ID3D12GraphicsCommandList2 D3DGraphicsCommandList;
using D3DCopyCommandList = ID3D12GraphicsCommandList;
// on PC we only lock down the execution mode on release builds
#define FIXED_EXECUTION_MODE               DAGOR_DBGLEVEL == 0
#define DX12_ALLOW_SPLIT_BARRIERS          1
#define DX12_WHATCH_IN_FLIGHT_BARRIERS     DAGOR_DBGLEVEL > 0
#define DX12_VALIDATE_INPUT_LAYOUT_USES    DAGOR_DBGLEVEL > 0
#define DX12_INDIVIDUAL_BARRIER_CHECK      0
#define DX12_REPORT_TRANSITION_INFO        0
#define DX12_TRACK_ACTIVE_DRAW_EVENTS      DAGOR_DBGLEVEL > 0
#define DX12_VALIDATE_USER_BARRIERS        DAGOR_DBGLEVEL > 0
#define DX12_AUTOMATIC_BARRIERS            1
#define DX12_PROCESS_USER_BARRIERS         1
#define DX12_RECORD_TIMING_DATA            1
#define DX12_CAPTURE_AFTER_LONG_FRAMES     (DX12_RECORD_TIMING_DATA && (DAGOR_DBGLEVEL > 0))
#define DX12_REPORT_PIPELINE_CREATE_TIMING 1
// TODO no real gamma control on dx12...
#define DX12_HAS_GAMMA_CONTROL             1

// Possible to run with set to 0, but there is no benefit
#define DX12_USE_AUTO_PROMOTE_AND_DECAY 1

#define DX12_ENABLE_CONST_BUFFER_DESCRIPTORS 1

#define DX12_SELECTABLE_CALL_STACK_CAPTURE 1

#define DX12_VALIDATA_COPY_COMMAND_LIST     1
#define DX12_VALIDATE_COMPUTE_COMMAND_LIST  1
#define DX12_VALIDATE_RAYTRACE_COMMAND_LIST 1
#define DX12_VALIDATE_GRAPHICS_COMMAND_LIST 1

#define DX12_PROCESS_USER_BARRIERS_DEFAULT 0
#endif

#if DX12_USE_AUTO_PROMOTE_AND_DECAY
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET = D3D12_RESOURCE_STATE_COMMON;
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE = D3D12_RESOURCE_STATE_COMMON;
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE = D3D12_RESOURCE_STATE_COMMON;
// Can not be detected with auto promote and decay as it needs
// D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET to be different than D3D12_RESOURCE_STATE_COMMON.
#define DX12_FIX_UNITITALIZED_STATIC_TEXTURE_STATE 0
#else
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET = D3D12_RESOURCE_STATE_COPY_DEST;
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE = D3D12_RESOURCE_STATE_COPY_SOURCE;
static constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE =
  D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;
// Fixes an engine error, where static textures with uninitialized content are used as source for
// draws, dispatches or copies. This bug makes only problems on consoles as there auto promotion and
// the initial state of static textures will lead to problems. NOTE: This has a performance cost in
// the worker thread, as it has to do more barrier work for each draw, dispatch and copy call.
#define DX12_FIX_UNITITALIZED_STATIC_TEXTURE_STATE 1
#endif

#define DX12_FOLD_BATCHED_SPLIT_BARRIERS                  1
#define DX12_REUSE_UNORDERD_ACCESS_VIEW_DESCRIPTOR_RANGES 1
#define DX12_REUSE_SHADER_RESOURCE_VIEW_DESCRIPTOR_RANGES 1
// spams on ever frame! - set this to 1 to enable
#define DX12_REPORT_DISCARD_MEMORY_PER_FRAME              0
#define DX12_VALIDATE_DRAW_CALL_USEFULNESS                0
#define DX12_DROP_NOT_USEFUL_DRAW_CALLS                   1
#define DX12_WARN_EMPTY_CLEARS                            1

#define COMMAND_BUFFER_DEBUG_INFO_DEFINED                     _TARGET_PC_WIN
// Binary search on pipeline variants is substantially slower than linear search (up to 8% worse bench results).
// The probable reason for this is the low average number of variants (majority has less than 3 variants),
// the binary search working against prefetching and increased complexity of the search.
#define DX12_USE_BINARY_SEARCH_FOR_GRAPHICS_PIPELINE_VARIANTS 0

#if DX12_AUTOMATIC_BARRIERS && DX12_PROCESS_USER_BARRIERS
#define DX12_CONFIGUREABLE_BARRIER_MODE 1
#else
#define DX12_CONFIGUREABLE_BARRIER_MODE 0
#endif

#define DX12_REPORT_BUFFER_PADDING 0

#define DX12_PRINT_USER_BUFFER_BARRIERS  0
#define DX12_PRINT_USER_TEXTURE_BARRIERS 0

#if DAGOR_DBGLEVEL > 0 || _TARGET_PC_WIN
#define DX12_DOES_SET_DEBUG_NAMES          1
#define DX12_SET_DEBUG_OBJ_NAME(obj, name) obj->SetName(name)
#else
#define DX12_DOES_SET_DEBUG_NAMES 0
#define DX12_SET_DEBUG_OBJ_NAME(obj, name)
#endif

#if !DX12_AUTOMATIC_BARRIERS && !DX12_PROCESS_USER_BARRIERS
#error "DX12 Driver configured to _not_ generate required barriers and to _ignore_ user barriers, this will crash on execution"
#endif

#if _TARGET_XBOX && !_TARGET_SCARLETT
#define DX12_USE_ESRAM 1
#else
#define DX12_USE_ESRAM 0
#endif

#define DX12_ENABLE_MT_VALIDATION          DAGOR_DBGLEVEL > 0
#define DX12_ENABLE_PEDANTIC_MT_VALIDATION DAGOR_DBGLEVEL > 1

#define DX12_SUPPORT_RESOURCE_MEMORY_METRICS DAGOR_DBGLEVEL > 0
#define DX12_RESOURCE_USAGE_TRACKER          DAGOR_DBGLEVEL > 0

template <uint32_t I>
struct BitsNeeded
{
  static constexpr int VALUE = BitsNeeded<I / 2>::VALUE + 1;
};
template <>
struct BitsNeeded<0>
{
  static constexpr int VALUE = 1;
};
template <>
struct BitsNeeded<1>
{
  static constexpr int VALUE = 1;
};

struct Extent2D
{
  uint32_t width;
  uint32_t height;
};

inline bool operator==(Extent2D l, Extent2D r) { return l.width == r.width && l.height == r.height; }
inline bool operator!=(Extent2D l, Extent2D r) { return !(l == r); }

struct Extent3D
{
  uint32_t width;
  uint32_t height;
  uint32_t depth;

  explicit operator Extent2D() const { return {width, height}; }
};

inline Extent3D operator*(Extent3D l, Extent3D r) { return {l.width * r.width, l.height * r.height, l.depth * r.depth}; }

inline Extent3D operator/(Extent3D l, Extent3D r) { return {l.width / r.width, l.height / r.height, l.depth / r.depth}; }

inline bool operator==(Extent3D l, Extent3D r) { return l.depth == r.depth && static_cast<Extent2D>(l) == static_cast<Extent2D>(r); }

inline bool operator!=(Extent3D l, Extent3D r) { return !(l == r); }

inline Extent3D operator>>(Extent3D value, uint32_t shift)
{
  return {value.width >> shift, value.height >> shift, value.depth >> shift};
}

inline Extent3D max(Extent3D a, Extent3D b) { return {max(a.width, b.width), max(a.height, b.height), max(a.depth, b.depth)}; }

inline Extent3D min(Extent3D a, Extent3D b) { return {min(a.width, b.width), min(a.height, b.height), min(a.depth, b.depth)}; }

inline Extent3D mip_extent(Extent3D value, uint32_t mip) { return max(value >> mip, {1, 1, 1}); }

struct Offset2D
{
  int32_t x;
  int32_t y;
};

inline bool operator==(Offset2D l, Offset2D r) { return l.x == r.x && l.y == r.y; }

inline bool operator!=(Offset2D l, Offset2D r) { return !(l == r); }

struct Offset3D
{
  int32_t x;
  int32_t y;
  int32_t z;

  explicit operator Offset2D() { return {x, y}; }
};

inline bool operator==(Offset3D l, Offset3D r) { return l.z == r.z && static_cast<Offset2D>(l) == static_cast<Offset2D>(r); }

inline bool operator!=(Offset3D l, Offset3D r) { return !(l == r); }

inline Extent3D operator+(Extent3D ext, Offset3D ofs) { return {ext.width + ofs.x, ext.height + ofs.y, ext.depth + ofs.z}; }

inline D3D12_RECT clamp_rect(D3D12_RECT rect, Extent2D ext)
{
  rect.left = clamp<decltype(rect.left)>(rect.left, 0, ext.width);
  rect.right = clamp<decltype(rect.right)>(rect.right, 0, ext.width);
  rect.top = clamp<decltype(rect.top)>(rect.top, 0, ext.height);
  rect.bottom = clamp<decltype(rect.bottom)>(rect.bottom, 0, ext.height);
  return rect;
}

struct ViewportState
{
  int x;
  int y;
  int width;
  int height;
  float minZ;
  float maxZ;

  ViewportState() = default;

  ViewportState(const D3D12_VIEWPORT &vp)
  {
    x = vp.TopLeftX;
    y = vp.TopLeftY;
    width = vp.Width;
    height = vp.Height;
    minZ = vp.MinDepth;
    maxZ = vp.MaxDepth;
  }

  D3D12_RECT asRect() const
  {
    D3D12_RECT result;
    result.left = x;
    result.top = y;
    result.right = x + width;
    result.bottom = y + height;

    return result;
  }

  operator D3D12_VIEWPORT() const
  {
    D3D12_VIEWPORT result;
    result.TopLeftX = x;
    result.TopLeftY = y;
    result.Width = width;
    result.Height = height;
    result.MinDepth = minZ;
    result.MaxDepth = maxZ;
    return result;
  }
};

inline bool operator==(const ViewportState &l, const ViewportState &r)
{
#define CMP_P(n) (l.n == r.n)
  return CMP_P(x) && CMP_P(y) && CMP_P(width) && CMP_P(height) && CMP_P(minZ) && CMP_P(maxZ);
#undef CMP_P
}
inline bool operator!=(const ViewportState &l, const ViewportState &r) { return !(l == r); }
enum class RegionDifference
{
  EQUAL,
  SUBSET,
  SUPERSET
};
inline RegionDifference classify_viewport_diff(const ViewportState &from, const ViewportState &to)
{
  const int32_t dX = to.x - from.x;
  const int32_t dY = to.y - from.y;
  const int32_t dW = (to.width + to.x) - (from.width + from.x);
  const int32_t dH = (to.height + to.y) - (from.height + from.y);

  RegionDifference rectDif = RegionDifference::EQUAL;
  // if all zero, then they are the same
  if (dX | dY | dW | dH)
  {
    // can be either subset or completely different
    if (dX >= 0 && dY >= 0 && dW <= 0 && dH <= 0)
    {
      rectDif = RegionDifference::SUBSET;
    }
    else
    {
      rectDif = RegionDifference::SUPERSET;
    }
  }

  if (RegionDifference::SUPERSET != rectDif)
  {
    // min/max z only affect viewport but not render regions, so it is always a subset if it has
    // changed
    if (to.maxZ != from.maxZ || to.minZ != from.minZ)
    {
      return RegionDifference::SUBSET;
    }
  }
  return rectDif;
}


namespace drv3d_dx12
{
struct ConstRegisterType
{
  uint32_t components[SHADER_REGISTER_ELEMENTS];
};
inline bool operator==(const ConstRegisterType &l, const ConstRegisterType &r)
{
  return eastl::equal(eastl::begin(l.components), eastl::end(l.components), eastl::begin(r.components));
}

template <typename I, typename TAG>
class TaggedIndexType
{
  I value{};

  constexpr TaggedIndexType(I v) : value{v} {}

public:
  using ValueType = I;

  constexpr TaggedIndexType() = default;
  ~TaggedIndexType() = default;

  TaggedIndexType(const TaggedIndexType &) = default;
  TaggedIndexType &operator=(const TaggedIndexType &) = default;

  static constexpr TaggedIndexType make(I v) { return {v}; }

  constexpr I index() const { return value; }

  friend bool operator==(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value == r.value; }

  friend bool operator!=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value != r.value; }

  friend bool operator<(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value < r.value; }

  friend bool operator>(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value > r.value; }

  friend bool operator<=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value <= r.value; }

  friend bool operator>=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value >= r.value; }

  friend int operator-(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value - r.value; }

  friend TaggedIndexType operator+(const TaggedIndexType &l, I r) { return {I(l.value + r)}; }

  friend TaggedIndexType operator-(const TaggedIndexType &l, I r) { return {I(l.value - r)}; }

  TaggedIndexType &operator+=(I r)
  {
    value += r;
    return *this;
  }

  TaggedIndexType &operator-=(I r)
  {
    value -= r;
    return *this;
  }

  template <typename C>
  TaggedIndexType &operator+=(C r)
  {
    *this = *this + r;
    return *this;
  }

  template <typename C>
  TaggedIndexType &operator-=(C r)
  {
    *this = *this - r;
    return *this;
  }

  TaggedIndexType &operator++()
  {
    ++value;
    return *this;
  }

  TaggedIndexType &operator--()
  {
    --value;
    return *this;
  }

  TaggedIndexType operator++(int) const
  {
    auto copy = *this;
    return ++copy;
  }

  TaggedIndexType operator--(int) const
  {
    auto copy = *this;
    return --copy;
  }

  operator DagorSafeArg() const { return {index()}; }
};

template <typename IT>
class TaggedRangeType : private ValueRange<IT>
{
  using RangeType = ValueRange<IT>;

public:
  using ValueType = typename ValueRange<IT>::ValueType;
  using RangeType::begin;
  using RangeType::end;
  using RangeType::isInside;
  using RangeType::isValidRange;
  using RangeType::size;
  using RangeType::ValueRange;

  constexpr IT front() const { return RangeType::front(); }
  constexpr IT back() const { return RangeType::back(); }

  constexpr TaggedRangeType front(ValueType offset) const { return {IT(this->start + offset), this->stop}; }
  constexpr TaggedRangeType back(ValueType offset) const { return {IT(this->stop - offset), this->stop}; }

  void resize(uint32_t count) { this->stop = this->start + count; }

  constexpr TaggedRangeType subRange(IT offset, uint32_t count) const { return make(this->start + offset, count); }

  constexpr TaggedRangeType subRange(uint32_t offset, uint32_t count) const { return make(this->start + offset, count); }

  static constexpr TaggedRangeType make(IT base, uint32_t count) { return {base, base + count}; }

  static constexpr TaggedRangeType make(uint32_t base, uint32_t count) { return {IT::make(base), IT::make(base + count)}; }

  static constexpr TaggedRangeType make_invalid() { return {IT::make(1), IT::make(0)}; }
};

template <typename IT>
class TaggedCountType
{
public:
  using ValueType = IT;
  using IndexValueType = typename ValueType::ValueType;
  using RangeType = TaggedRangeType<ValueType>;

private:
  IndexValueType value{};

  constexpr TaggedCountType(IndexValueType v) : value{v} {}

public:
  struct Iterator
  {
    IndexValueType at{};

    constexpr Iterator() = default;
    ~Iterator() = default;
    constexpr Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;
    constexpr Iterator(IndexValueType v) : at(v) {}
    constexpr ValueType operator*() const { return ValueType::make(at); }
    Iterator &operator++()
    {
      ++at;
      return *this;
    }
    Iterator operator++(int) { return at++; }
    Iterator &operator--()
    {
      --at;
      return *this;
    }
    Iterator operator--(int) { return at--; }
    friend constexpr bool operator==(Iterator l, Iterator r) { return l.at == r.at; }
    friend constexpr bool operator!=(Iterator l, Iterator r) { return l.at != r.at; }
  };
  constexpr Iterator begin() const { return {0}; }
  constexpr Iterator end() const { return {value}; }

  constexpr TaggedCountType() = default;
  ~TaggedCountType() = default;

  TaggedCountType(const TaggedCountType &) = default;
  TaggedCountType &operator=(const TaggedCountType &) = default;

  static constexpr TaggedCountType make(IndexValueType v) { return {v}; }

  constexpr IndexValueType count() const { return value; }

  constexpr RangeType asRange() const { return RangeType::make(0, value); }
  // Allow implicit conversion to range as count is a specialized range
  constexpr operator RangeType() const { return asRange(); }

  constexpr RangeType front(ValueType offset) const { return RangeType::make(offset, value - offset.index()); }
  constexpr RangeType back(ValueType offset) const { return RangeType::make(value - offset.index(), offset.index()); }

  operator DagorSafeArg() const { return {count()}; }

  friend bool operator==(const TaggedCountType &l, const TaggedCountType &r) { return l.value == r.value; }

  friend bool operator!=(const TaggedCountType &l, const TaggedCountType &r) { return l.value != r.value; }

  friend bool operator<=(const TaggedCountType &l, const TaggedCountType &r) { return l.value <= r.value; }

  friend bool operator>=(const TaggedCountType &l, const TaggedCountType &r) { return l.value >= r.value; }

  friend bool operator<(const TaggedCountType &l, const TaggedCountType &r) { return l.value < r.value; }

  friend bool operator>(const TaggedCountType &l, const TaggedCountType &r) { return l.value > r.value; }

  friend bool operator==(const RangeType &l, const TaggedCountType &r) { return 0 == l.front().index() && l.size() == r.value; }

  friend bool operator!=(const RangeType &l, const TaggedCountType &r) { return 0 != l.front().index() && l.size() != r.value; }

  friend bool operator==(const TaggedCountType &l, const RangeType &r) { return 0 == r.front().index() && r.size() == l.value; }

  friend bool operator!=(const TaggedCountType &l, const RangeType &r) { return 0 != r.front().index() && r.size() != l.value; }
};

template <typename IT>
inline constexpr bool operator==(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() == r.index();
}

template <typename IT>
inline constexpr bool operator!=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() != r.index();
}

template <typename IT>
inline constexpr bool operator<=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() <= r.index();
}

template <typename IT>
inline constexpr bool operator>=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() >= r.index();
}

template <typename IT>
inline constexpr bool operator<(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() < r.index();
}

template <typename IT>
inline constexpr bool operator>(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() > r.index();
}

template <typename IT>
inline constexpr bool operator==(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() == r.count();
}

template <typename IT>
inline constexpr bool operator!=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() != r.count();
}

template <typename IT>
inline constexpr bool operator<=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() <= r.count();
}

template <typename IT>
inline constexpr bool operator>=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() >= r.count();
}

template <typename IT>
inline constexpr bool operator<(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() < r.count();
}

struct MipMapIndexTag;
using MipMapIndex = TaggedIndexType<uint8_t, MipMapIndexTag>;
using MipMapRange = TaggedRangeType<MipMapIndex>;
using MipMapCount = TaggedCountType<MipMapIndex>;

struct SubresourceIndexTag;
using SubresourceIndex = TaggedIndexType<uint32_t, SubresourceIndexTag>;
using SubresourceRange = TaggedRangeType<SubresourceIndex>;
using SubresourceCount = TaggedCountType<SubresourceIndex>;

struct ArrayLayerIndexTag;
using ArrayLayerIndex = TaggedIndexType<uint16_t, ArrayLayerIndexTag>;
using ArrayLayerRange = TaggedRangeType<ArrayLayerIndex>;
using ArrayLayerCount = TaggedCountType<ArrayLayerIndex>;

struct FormatPlaneIndexTag;
using FormatPlaneIndex = TaggedIndexType<uint8_t, FormatPlaneIndexTag>;
using FormatPlaneRange = TaggedRangeType<FormatPlaneIndex>;
using FormatPlaneCount = TaggedCountType<FormatPlaneIndex>;

class SubresourcePerFormatPlaneCount
{
public:
  using ValueType = SubresourceIndex::ValueType;

private:
  ValueType value{};

  constexpr SubresourcePerFormatPlaneCount(ValueType v) : value{v} {}

public:
  constexpr SubresourcePerFormatPlaneCount() = default;
  ~SubresourcePerFormatPlaneCount() = default;

  SubresourcePerFormatPlaneCount(const SubresourcePerFormatPlaneCount &) = default;
  SubresourcePerFormatPlaneCount &operator=(const SubresourcePerFormatPlaneCount &) = default;

  static constexpr SubresourcePerFormatPlaneCount make(ValueType v) { return {v}; }
  static constexpr SubresourcePerFormatPlaneCount make(MipMapCount mip, ArrayLayerCount layers)
  {
    return {ValueType(mip.count() * layers.count())};
  }
  static constexpr SubresourcePerFormatPlaneCount make(ArrayLayerCount layers, MipMapCount mip)
  {
    return {ValueType(mip.count() * layers.count())};
  }

  constexpr ValueType count() const { return value; }

  operator DagorSafeArg() const { return {count()}; }
};

inline SubresourcePerFormatPlaneCount operator*(const MipMapCount &l, const ArrayLayerCount &r)
{
  return SubresourcePerFormatPlaneCount::make(l, r);
}

inline SubresourcePerFormatPlaneCount operator*(const ArrayLayerCount &l, const MipMapCount &r)
{
  return SubresourcePerFormatPlaneCount::make(l, r);
}

// Per plane subres count times the plane index yields the subres range of the plane index
inline SubresourceRange operator*(const SubresourcePerFormatPlaneCount &l, const FormatPlaneIndex &r)
{
  return SubresourceRange::make(l.count() * r.index(), l.count());
}

// To keep it associative index times per plane yields also the subres range of the plane index
inline SubresourceRange operator*(const FormatPlaneIndex &l, const SubresourcePerFormatPlaneCount &r)
{
  return SubresourceRange::make(r.count() * l.index(), r.count());
}

// Per plane subres count times the plane count yields the total subres count
inline SubresourceCount operator*(const SubresourcePerFormatPlaneCount &l, const FormatPlaneCount &r)
{
  return SubresourceCount::make(l.count() * r.count());
}

inline SubresourceCount operator*(const FormatPlaneCount &r, const SubresourcePerFormatPlaneCount &l)
{
  return SubresourceCount::make(l.count() * r.count());
}

inline SubresourceIndex calculate_subresource_index(MipMapIndex mip, ArrayLayerIndex array, MipMapCount mips_per_array)
{
  return SubresourceIndex::make(mip.index() + (array.index() * mips_per_array.count()));
}

inline SubresourceIndex operator+(const SubresourceIndex &l, const SubresourcePerFormatPlaneCount &r)
{
  return SubresourceIndex::make(l.index() + r.count());
}
} // namespace drv3d_dx12
#include "format_store.h"

namespace drv3d_dx12
{
BEGIN_BITFIELD_TYPE(ImageViewState, uint64_t)
  enum Type
  {
    INVALID, // 0!
    SRV,
    UAV,
    RTV,
    DSV_RW,
    DSV_CONST
  };
  enum
  {
    WORD_SIZE = sizeof(uint64_t) * 8,
    SAMPLE_STENCIL_BITS = 1,
    SAMPLE_STENCIL_SHIFT = 0,
    TYPE_BITS = 3,
    TYPE_SHIFT = SAMPLE_STENCIL_BITS + SAMPLE_STENCIL_SHIFT,
    IS_CUBEMAP_BITS = 1,
    IS_CUBEMAP_SHIFT = TYPE_BITS + TYPE_SHIFT,
    IS_ARRAY_BITS = 1,
    IS_ARRAY_SHIFT = IS_CUBEMAP_BITS + IS_CUBEMAP_SHIFT,
    FORMAT_BITS = FormatStore::BITS + 1,
    FORMAT_SHIFT = IS_ARRAY_SHIFT + IS_ARRAY_BITS,
    MIPMAP_OFFSET_BITS = BitsNeeded<15>::VALUE,
    MIPMAP_OFFSET_SHIFT = FORMAT_SHIFT + FORMAT_BITS,
    MIPMAP_RANGE_OFFSET = 1,
    MIPMAP_RANGE_BITS = BitsNeeded<16 - MIPMAP_RANGE_OFFSET>::VALUE,
    MIPMAP_RANGE_SHIFT = MIPMAP_OFFSET_SHIFT + MIPMAP_OFFSET_BITS,
    // automatic assign left over space to array range def
    ARRAY_DATA_SIZE = WORD_SIZE - MIPMAP_RANGE_SHIFT - MIPMAP_RANGE_BITS,
    ARRAY_OFFSET_BITS = (ARRAY_DATA_SIZE / 2) + (ARRAY_DATA_SIZE % 2),
    ARRAY_OFFSET_SHIFT = MIPMAP_RANGE_SHIFT + MIPMAP_RANGE_BITS,
    ARRAY_RANGE_OFFSET = 1,
    ARRAY_RANGE_BITS = ARRAY_DATA_SIZE / 2,
    ARRAY_RANGE_SHIFT = (ARRAY_OFFSET_SHIFT + ARRAY_OFFSET_BITS)
  };
  ADD_BITFIELD_MEMBER(sampleStencil, SAMPLE_STENCIL_SHIFT, SAMPLE_STENCIL_BITS)
  ADD_BITFIELD_MEMBER(type, TYPE_SHIFT, TYPE_BITS)
  ADD_BITFIELD_MEMBER(isCubemap, IS_CUBEMAP_SHIFT, IS_CUBEMAP_BITS)
  ADD_BITFIELD_MEMBER(isArray, IS_ARRAY_SHIFT, IS_ARRAY_BITS)
  ADD_BITFIELD_MEMBER(format, FORMAT_SHIFT, FORMAT_BITS)
  ADD_BITFIELD_MEMBER(mipmapOffset, MIPMAP_OFFSET_SHIFT, MIPMAP_OFFSET_BITS);
  ADD_BITFIELD_MEMBER(mipmapRange, MIPMAP_RANGE_SHIFT, MIPMAP_RANGE_BITS)
  ADD_BITFIELD_MEMBER(arrayOffset, ARRAY_OFFSET_SHIFT, ARRAY_OFFSET_BITS)
  ADD_BITFIELD_MEMBER(arrayRange, ARRAY_RANGE_SHIFT, ARRAY_RANGE_BITS)
  bool isValid() const
  {
    return uint64_t(*this) != 0;
  } // since type can't be 0/UNKNOWN, all bits can't be 0. comparing whole machine word is faster than extract type
  explicit operator bool() const { return isValid(); }
  void setType(Type tp) { type = tp; }
  Type getType() const { return static_cast<Type>(static_cast<uint32_t>(type)); }
  void setRTV() { setType(RTV); }
  void setDSV(bool as_const) { setType(as_const ? DSV_CONST : DSV_RW); }
  void setSRV() { setType(SRV); }
  void setUAV() { setType(UAV); }
  bool isRTV() const { return static_cast<uint32_t>(type) == RTV; }
  bool isDSV() const { return static_cast<uint32_t>(type) == DSV_RW || static_cast<uint32_t>(type) == DSV_CONST; }
  bool isSRV() const { return static_cast<uint32_t>(type) == SRV; }
  bool isUAV() const { return static_cast<uint32_t>(type) == UAV; }

  // TODO check cube/array handling

  // TODO: is d24/d32 always planar?
  D3D12_SHADER_RESOURCE_VIEW_DESC asSRVDesc(D3D12_RESOURCE_DIMENSION dim) const
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC result;
    const auto fmt = getFormat();
    result.Format = fmt.asDxGiFormat();
    uint32_t planeSlice = 0;
    if (DXGI_FORMAT_D24_UNORM_S8_UINT == result.Format)
    {
      if (0 == sampleStencil)
      {
        result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      }
      else
      {
        result.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        planeSlice = fmt.getPlanes().count() > 1 ? 1 : 0;
      }
    }
    else if (DXGI_FORMAT_D32_FLOAT_S8X24_UINT == result.Format)
    {
      if (0 == sampleStencil)
      {
        result.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      }
      else
      {
        result.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        planeSlice = fmt.getPlanes().count() > 1 ? 1 : 0;
      }
    }
    else if (DXGI_FORMAT_D16_UNORM == result.Format)
    {
      result.Format = DXGI_FORMAT_R16_UNORM;
    }
    else if (DXGI_FORMAT_D32_FLOAT == result.Format)
    {
      result.Format = DXGI_FORMAT_R32_FLOAT;
    }
    result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: fatal("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MostDetailedMip = getMipBase().index();
          target.MipLevels = getMipCount();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
          target.ResourceMinLODClamp = 0.f;
        }
        else
        {
          result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MostDetailedMip = getMipBase().index();
          target.MipLevels = getMipCount();
          target.ResourceMinLODClamp = 0.f;
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubemap)
        {
          if (isArray)
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            auto &target = result.TextureCubeArray;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.First2DArrayFace = getArrayBase().index();
            target.NumCubes = getArrayCount() / 6;
            target.ResourceMinLODClamp = 0.f;
          }
          else
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            auto &target = result.TextureCube;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.ResourceMinLODClamp = 0.f;
          }
        }
        else
        {
          if (isArray)
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            auto &target = result.Texture2DArray;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.FirstArraySlice = getArrayBase().index();
            target.ArraySize = getArrayCount();
            target.PlaneSlice = planeSlice;
            target.ResourceMinLODClamp = 0.f;
          }
          else
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            auto &target = result.Texture2D;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.PlaneSlice = planeSlice;
            target.ResourceMinLODClamp = 0.f;
          }
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MostDetailedMip = getMipBase().index();
        target.MipLevels = getMipCount();
        target.ResourceMinLODClamp = 0.f;
        break;
    }
    return result;
  }

  D3D12_UNORDERED_ACCESS_VIEW_DESC asUAVDesc(D3D12_RESOURCE_DIMENSION dim) const
  {
    D3D12_UNORDERED_ACCESS_VIEW_DESC result;
    result.Format = getFormat().asDxGiFormat();
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: fatal("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        // Array and cube are the same for UAV
        if (isArray || isCubemap)
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
          auto &target = result.Texture2DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
          target.PlaneSlice = 0;
        }
        else
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
          auto &target = result.Texture2D;
          target.MipSlice = getMipBase().index();
          target.PlaneSlice = 0;
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MipSlice = getMipBase().index();
        target.FirstWSlice = getArrayBase().index();
        target.WSize = getArrayCount();
        break;
    }
    return result;
  }

  D3D12_RENDER_TARGET_VIEW_DESC asRTVDesc(D3D12_RESOURCE_DIMENSION dim) const
  {
    D3D12_RENDER_TARGET_VIEW_DESC result;
    result.Format = getFormat().asDxGiFormat();
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: fatal("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isArray || isCubemap)
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
          auto &target = result.Texture2DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
          target.PlaneSlice = 0;
        }
        else
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
          auto &target = result.Texture2D;
          target.MipSlice = getMipBase().index();
          target.PlaneSlice = 0;
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MipSlice = getMipBase().index();
        target.FirstWSlice = getArrayBase().index();
        target.WSize = getArrayCount();
        break;
    }

    return result;
  }

  D3D12_DEPTH_STENCIL_VIEW_DESC asDSVDesc(D3D12_RESOURCE_DIMENSION dim) const
  {
    D3D12_DEPTH_STENCIL_VIEW_DESC result;
    auto fmt = getFormat();
    result.Format = fmt.asDxGiFormat();
    result.Flags = D3D12_DSV_FLAG_NONE;
    if (getType() == DSV_CONST)
    {
      if (fmt.isDepth())
        result.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
      if (fmt.isStencil())
        result.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    }
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: fatal("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isArray || isCubemap)
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
          auto &target = result.Texture2DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
          auto &target = result.Texture2D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D: fatal("DX12: Volume depth stencil view not supported"); break;
    }
    return result;
  }

  void setFormat(FormatStore fmt) { format = fmt.index; }
  FormatStore getFormat() const { return FormatStore(format); }
  void setMipBase(MipMapIndex u) { mipmapOffset = u.index(); }
  MipMapIndex getMipBase() const { return MipMapIndex::make(mipmapOffset); }
  void setMipCount(uint8_t u) { mipmapRange = u - MIPMAP_RANGE_OFFSET; }
  uint8_t getMipCount() const { return MIPMAP_RANGE_OFFSET + mipmapRange; }
  void setSingleMipMapRange(MipMapIndex index)
  {
    setMipBase(index);
    G_STATIC_ASSERT(1 == MIPMAP_RANGE_OFFSET);
    mipmapRange = 0;
  }
  void setMipMapRange(MipMapIndex index, uint32_t count)
  {
    setMipBase(index);
    setMipCount(count);
  }
  void setMipMapRange(MipMapRange range)
  {
    setMipBase(range.front());
    setMipCount(range.size());
  }
  MipMapRange getMipRange() const { return MipMapRange::make(getMipBase(), getMipCount()); }
  void setArrayBase(ArrayLayerIndex u) { arrayOffset = u.index(); }
  ArrayLayerIndex getArrayBase() const { return ArrayLayerIndex::make(arrayOffset); }
  void setArrayCount(uint16_t u) { arrayRange = u - ARRAY_RANGE_OFFSET; }
  uint16_t getArrayCount() const { return ARRAY_RANGE_OFFSET + (uint32_t)arrayRange; }
  void setArrayRange(ArrayLayerRange range)
  {
    setArrayBase(range.front());
    setArrayCount(range.size());
  }
  void setSingleArrayRange(ArrayLayerIndex index)
  {
    setArrayBase(index);
    G_STATIC_ASSERT(1 == ARRAY_RANGE_OFFSET);
    arrayRange = 0;
  }
  ArrayLayerRange getArrayRange() const { return ArrayLayerRange::make(getArrayBase(), getArrayCount()); }
  void setSingleDepthLayer(uint16_t base)
  {
    setArrayBase(ArrayLayerIndex::make(base));
    setArrayCount(1);
  }
  void setDepthLayerRange(uint16_t base, uint16_t count)
  {
    setArrayBase(ArrayLayerIndex::make(base));
    setArrayCount(count);
  }
  FormatPlaneIndex getPlaneIndex() const
  {
    return FormatPlaneIndex::make((sampleStencil && getFormat().getPlanes().count() > 1) ? 1 : 0);
  }
  template <typename T>
  // NOTE: does not take plane index into account
  void iterateSubresources(D3D12_RESOURCE_DIMENSION dim, MipMapCount mip_per_array, T clb)
  {
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == dim)
    {
      for (auto m : getMipRange())
      {
        clb(SubresourceIndex::make(m.index()));
      }
    }
    else
    {
      for (auto a : getArrayRange())
      {
        for (auto m : getMipRange())
        {
          clb(calculate_subresource_index(m, a, mip_per_array));
        }
      }
    }
  }
END_BITFIELD_TYPE()
inline bool operator==(ImageViewState l, ImageViewState r) { return l.wrapper.value == r.wrapper.value; }
inline bool operator!=(ImageViewState l, ImageViewState r) { return l.wrapper.value != r.wrapper.value; }
inline bool operator<(ImageViewState l, ImageViewState r) { return l.wrapper.value < r.wrapper.value; }

// We have our own half float converter here, as we may want to tweak it a bit in the future
namespace half_float
{
static constexpr uint32_t float_sign_mask = 0x80000000U;
static constexpr uint32_t float_exponent_mask = 0x7F800000U;
static constexpr uint32_t float_mantissa_mask = 0x7FFFFFU;
static constexpr uint32_t float_mantiassa_size = 23;
static constexpr int32_t float_mantiassa_bias = 127;
static constexpr int32_t bias = 15;
static constexpr int32_t exponent_size = 5;
static constexpr int32_t mantissa_size = 10;
static constexpr int32_t max_exponent = (1 << exponent_size) - 1;

inline float convert_to_float(uint16_t v)
{
  int32_t exponent = int32_t(((v & 0x7FFFU)) >> mantissa_size);
  bool isNan = exponent >= max_exponent;
  uint32_t exponentPart =
    exponent <= 0 ? 0U : (isNan ? float_exponent_mask : ((exponent - bias + float_mantiassa_bias) << float_mantiassa_size));
  uint32_t signPart = uint32_t(v & 0x8000U) << 16;
  uint32_t fractionPart = isNan ? float_mantissa_mask : (uint32_t(v) << (float_mantiassa_size - mantissa_size)) & float_mantissa_mask;
  uint32_t floatBits = signPart | exponentPart | fractionPart;
  float floatValue;
  memcpy(&floatValue, &floatBits, sizeof(float));
  return floatValue;
}

inline uint16_t convert_from_float(float v)
{
  uint32_t floatBits;
  memcpy(&floatBits, &v, sizeof(float));

  int32_t exponent = ((floatBits & float_exponent_mask) >> float_mantiassa_size) - float_mantiassa_bias + bias;
  uint32_t exponentPart = clamp(exponent, 0, max_exponent) << mantissa_size;
  uint32_t signPart = ((floatBits & float_sign_mask) >> 16);
  uint32_t fractionPart = exponent >= 0 ? ((floatBits & float_mantissa_mask) >> (float_mantiassa_size - mantissa_size)) : 0U;

  return signPart | exponentPart | fractionPart;
}
} // namespace half_float

BEGIN_BITFIELD_TYPE(SamplerState, uint32_t)
  enum
  {
    BIAS_BITS = 16,
    BIAS_OFFSET = 0,
    MIP_BITS = 1,
    MIP_SHIFT = BIAS_OFFSET + BIAS_BITS,
    FILTER_BITS = 1,
    FILTER_SHIFT = MIP_SHIFT + MIP_BITS,
    // Instead of using N bits per coord, we store all coords in one value, this safes 2 bits
    COORD_VALUE_COUNT = (D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE - D3D12_TEXTURE_ADDRESS_MODE_WRAP) + 1,
    COORD_MAX_VALUE = COORD_VALUE_COUNT * COORD_VALUE_COUNT * COORD_VALUE_COUNT,
    COORD_BITS = BitsNeeded<COORD_MAX_VALUE>::VALUE,
    COORD_SHIFT = FILTER_SHIFT + FILTER_BITS,
    ANISO_BITS = 3,
    ANISO_SHIFT = COORD_SHIFT + COORD_BITS,
    BORDER_BITS = 2,
    BORDER_SHIFT = ANISO_SHIFT + ANISO_BITS,
    IS_COMPARE_BITS = 1,
    IS_COMPARE_SHIFT = BORDER_BITS + BORDER_SHIFT,
    FLOAT_EXP_BASE = 127,
    FLOAT_EXP_SHIFT = 23,
  };
  ADD_BITFIELD_MEMBER(mipMapMode, MIP_SHIFT, MIP_BITS)
  ADD_BITFIELD_MEMBER(filterMode, FILTER_SHIFT, FILTER_BITS)
  ADD_BITFIELD_MEMBER(borderColorMode, BORDER_SHIFT, BORDER_BITS)
  ADD_BITFIELD_MEMBER(anisotropicValue, ANISO_SHIFT, ANISO_BITS)
  ADD_BITFIELD_MEMBER(coordModes, COORD_SHIFT, COORD_BITS)
  ADD_BITFIELD_MEMBER(biasBits, BIAS_OFFSET, BIAS_BITS)
  ADD_BITFIELD_MEMBER(isCompare, IS_COMPARE_SHIFT, IS_COMPARE_BITS)

  BEGIN_BITFIELD_TYPE(iee754Float, uint32_t)
    float asFloat;
    uint32_t asUint;
    int32_t asInt;
    ADD_BITFIELD_MEMBER(mantissa, 0, 23)
    ADD_BITFIELD_MEMBER(exponent, 23, 8)
    ADD_BITFIELD_MEMBER(sign, 31, 1)
  END_BITFIELD_TYPE()

  D3D12_SAMPLER_DESC asDesc() const
  {
    D3D12_SAMPLER_DESC result;

    result.MaxAnisotropy = getAnisoInt();
    if (result.MaxAnisotropy > 1)
      result.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(static_cast<uint32_t>(isCompare));
    else
      result.Filter = D3D12_ENCODE_BASIC_FILTER(getFilter(), getFilter(), getMip(), static_cast<uint32_t>(isCompare));

    result.AddressU = getU();
    result.AddressV = getV();
    result.AddressW = getW();
    result.MipLODBias = getBias();
    result.ComparisonFunc = isCompare ? D3D12_COMPARISON_FUNC_LESS_EQUAL : D3D12_COMPARISON_FUNC_ALWAYS;
    result.MinLOD = 0;
    result.MaxLOD = FLT_MAX;
    result.BorderColor[0] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[1] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[2] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[3] = static_cast<uint32_t>(borderColorMode) & 2 ? 1.f : 0.f;

    return result;
  }

  void setMip(D3D12_FILTER_TYPE mip) { mipMapMode = (uint32_t)mip; }
  D3D12_FILTER_TYPE getMip() const { return static_cast<D3D12_FILTER_TYPE>(static_cast<uint32_t>(mipMapMode)); }
  void setFilter(D3D12_FILTER_TYPE filter) { filterMode = filter; }
  D3D12_FILTER_TYPE getFilter() const { return static_cast<D3D12_FILTER_TYPE>(static_cast<uint32_t>(filterMode)); }
  void setCoordModes(D3D12_TEXTURE_ADDRESS_MODE u, D3D12_TEXTURE_ADDRESS_MODE v, D3D12_TEXTURE_ADDRESS_MODE w)
  {
    auto rawU = static_cast<uint32_t>(u) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    auto rawV = static_cast<uint32_t>(v) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    auto rawW = static_cast<uint32_t>(w) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes = rawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT + rawV * COORD_VALUE_COUNT + rawU;
  }
  void setU(D3D12_TEXTURE_ADDRESS_MODE u)
  {
    auto oldRawU = coordModes % COORD_VALUE_COUNT;
    auto newRawU = static_cast<uint32_t>(u) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawU;
    coordModes += newRawU;
  }
  void setV(D3D12_TEXTURE_ADDRESS_MODE v)
  {
    auto oldRawV = (coordModes / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    auto newRawV = static_cast<uint32_t>(v) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawV * COORD_VALUE_COUNT;
    coordModes += newRawV * COORD_VALUE_COUNT;
  }
  void setW(D3D12_TEXTURE_ADDRESS_MODE w)
  {
    auto oldRawW = (coordModes / COORD_VALUE_COUNT / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    auto newRawW = static_cast<uint32_t>(w) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT;
    coordModes += newRawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT;
  }
  D3D12_TEXTURE_ADDRESS_MODE getU() const
  {
    auto rawValue = coordModes % COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  D3D12_TEXTURE_ADDRESS_MODE getV() const
  {
    auto rawValue = (coordModes / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  D3D12_TEXTURE_ADDRESS_MODE getW() const
  {
    auto rawValue = (coordModes / COORD_VALUE_COUNT) / COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  void setBias(float b) { biasBits = half_float::convert_from_float(b); }
  float getBias() const { return half_float::convert_to_float(biasBits); }
  void setAniso(float a)
  {
    // some float magic, falls flat on its face if it is not ieee-754
    // extracts exponent and subtracts the base
    // clamps the result into range from  0 to 4 which represents 1,2,4,8 and 16 as floats
    // negative values are treated as positive
    // values from range 0 - 1 are rounded up
    // everything else is rounded down
    iee754Float f;
    f.asFloat = a;
    int32_t value = f.exponent - FLOAT_EXP_BASE;
    // clamp from 1 to 16
    value = clamp(value, 0, 4);
    anisotropicValue = value;
  }
  float getAniso() const
  {
    iee754Float f;
    f.exponent = FLOAT_EXP_BASE + anisotropicValue;
    return f.asFloat;
  }
  uint32_t getAnisoInt() const { return 1u << static_cast<uint32_t>(anisotropicValue); }
  // Same restrictions as with vulkan, either color is white or black and its either fully
  // transparent or opaque
  void setBorder(E3DCOLOR color) { borderColorMode = ((color.r || color.g || color.b) ? 1 : 0) | (color.a ? 2 : 0); }
  E3DCOLOR getBorder() const
  {
    E3DCOLOR result;
    result.r = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.g = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.b = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.a = static_cast<uint32_t>(borderColorMode) & 2 ? 0xFF : 0;
    return result;
  }

  bool needsBorderColor() const
  {
    return (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getU()) || (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getV()) ||
           (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getW());
  }

  bool normalizeSelf()
  {
    bool wasNormalized = false;
    if (!needsBorderColor())
    {
      setBorder(0);
      wasNormalized = true;
    }
    return wasNormalized;
  }

  SamplerState normalize() const
  {
    // normalization is when border color is not needed we default to color 0
    SamplerState copy = *this;
    copy.normalizeSelf();
    return copy;
  }

  static SamplerState fromSamplerInfo(const d3d::SamplerInfo &info);
END_BITFIELD_TYPE()

struct ImageViewInfo
{
  D3D12_CPU_DESCRIPTOR_HANDLE handle;
  ImageViewState state;
};


template <typename T, size_t N>
constexpr inline size_t array_size(T (&)[N])
{
  return N;
}

inline D3D12_PRIMITIVE_TOPOLOGY translate_primitive_topology_to_dx12(int value)
{
  // G_ASSERTF(value < PRIM_TRIFAN, "primitive topology was %u", value);
#if _TARGET_XBOX
  if (value == PRIM_QUADLIST)
    return D3D_PRIMITIVE_TOPOLOGY_QUADLIST;
#endif
  return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(value);
}

template <typename H, H NullValue, typename T>
class TaggedHandle
{
  H h; // -V730_NOINIT

public:
  bool interlockedIsNull() const { return interlocked_acquire_load(h) == NullValue; }
  void interlockedSet(H v) { interlocked_release_store(h, v); }
  H get() const { return h; }
  explicit TaggedHandle(H a) : h(a) {}
  TaggedHandle() {}
  bool operator!() const { return h == NullValue; }
  friend bool operator==(TaggedHandle l, TaggedHandle r) { return l.get() == r.get(); }
  friend bool operator!=(TaggedHandle l, TaggedHandle r) { return l.get() != r.get(); }
  friend bool operator<(TaggedHandle l, TaggedHandle r) { return l.get() < r.get(); }
  friend bool operator>(TaggedHandle l, TaggedHandle r) { return l.get() > r.get(); }
  friend H operator-(TaggedHandle l, TaggedHandle r) { return l.get() - r.get(); }
  static TaggedHandle Null() { return TaggedHandle(NullValue); }
  static TaggedHandle make(H value) { return TaggedHandle{value}; }
};

template <typename H, H NullValue, typename T>
inline TaggedHandle<H, NullValue, T> genereate_next_id(TaggedHandle<H, NullValue, T> last_id)
{
  auto value = last_id.get() + 1;
  if (value == NullValue)
    ++value;
  G_ASSERT(value != NullValue);
  return TaggedHandle<H, NullValue, T>{value};
}

struct GraphicsProgramIDTag
{};
struct InputLayoutIDTag
{};
struct StaticRenderStateIDTag
{};
struct InternalInputLayoutIDTag
{};
struct FramebufferLayoutIDTag
{};
struct BindlessSetIdTag;

typedef TaggedHandle<int, -1, InputLayoutIDTag> InputLayoutID;
typedef TaggedHandle<int, -1, StaticRenderStateIDTag> StaticRenderStateID;
using InternalInputLayoutID = TaggedHandle<int, -1, InternalInputLayoutIDTag>;
using FramebufferLayoutID = TaggedHandle<int, -1, FramebufferLayoutIDTag>;
using BindlessSetId = TaggedHandle<uint32_t, 0, BindlessSetIdTag>;

// comes from ShaderID::group member having 3 bits -> 0-7
static constexpr uint32_t max_scripted_shaders_bin_groups = 8;

class ShaderByteCodeId
{
  union
  {
    uint32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  // ShaderByteCodeId() = default;
  // explicit ShaderByteCodeId(uint32_t value) : value{value} {}
  explicit operator bool() const { return -1 != value; }
  bool operator!() const { return -1 == value; }

  uint32_t getGroup() const { return group; }
  uint32_t getIndex() const { return index; }

  int32_t exportValue() const { return value; }

  static ShaderByteCodeId Null()
  {
    ShaderByteCodeId result;
    result.value = -1;
    return result;
  }

  static ShaderByteCodeId make(uint32_t group, uint32_t index)
  {
    ShaderByteCodeId result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value < r.value; }
  friend bool operator>(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value > r.value; }
  friend bool operator!=(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value != r.value; }
  friend bool operator==(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value == r.value; }
};

class ShaderID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  // ShaderID() = default;
  // explicit ShaderID(uint32_t value) : value{value} {}
  explicit operator bool() const { return -1 != value; }
  bool operator!() const { return -1 == value; }

  uint32_t getGroup() const { return group; }
  uint32_t getIndex() const { return index; }

  int32_t exportValue() const { return value; }

  static ShaderID Null()
  {
    ShaderID result;
    result.value = -1;
    return result;
  }

  static ShaderID importValue(int32_t value)
  {
    ShaderID result;
    result.value = value;
    return result;
  }

  static ShaderID make(uint32_t group, uint32_t index)
  {
    ShaderID result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ShaderID l, const ShaderID r) { return l.value < r.value; }

  friend bool operator>(const ShaderID l, const ShaderID r) { return l.value > r.value; }

  friend bool operator!=(const ShaderID l, const ShaderID r) { return l.value != r.value; }

  friend bool operator==(const ShaderID l, const ShaderID r) { return l.value == r.value; }
};

class ProgramID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t type : 2;
      uint32_t group : 3;
      uint32_t index : 27;
    };
  };

public:
  bool operator!() const { return -1 == value; }
  explicit operator bool() const { return -1 == value; }

  uint32_t getType() const { return type; }
  uint32_t getIndex() const { return index; }
  uint32_t getGroup() const { return group; }

  int32_t exportValue() const { return value; }

  static constexpr uint32_t type_graphics = 0;
  static constexpr uint32_t type_compute = 1;
  static constexpr uint32_t type_raytrace = 2;

  bool isGraphics() const { return type_graphics == type; }
  bool isCompute() const { return type_compute == type; }
  bool isRaytrace() const { return type_raytrace == type; }

  static ProgramID Null()
  {
    ProgramID result;
    result.value = -1;
    return result;
  }

  static ProgramID importValue(int32_t value)
  {
    ProgramID result;
    result.value = value;
    return result;
  }

  static ProgramID asGraphicsProgram(uint32_t group, uint32_t index)
  {
    ProgramID result;
    result.type = type_graphics;
    result.group = group;
    result.index = index;
    return result;
  }

  static ProgramID asComputeProgram(uint32_t group, uint32_t index)
  {
    ProgramID result;
    result.type = type_compute;
    result.group = group;
    result.index = index;
    return result;
  }

  static ProgramID asRaytraceProgram(uint32_t group, uint32_t index)
  {
    ProgramID result;
    result.type = type_raytrace;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ProgramID l, const ProgramID r) { return l.value < r.value; }

  friend bool operator>(const ProgramID l, const ProgramID r) { return l.value > r.value; }

  friend bool operator!=(const ProgramID l, const ProgramID r) { return l.value != r.value; }

  friend bool operator==(const ProgramID l, const ProgramID r) { return l.value == r.value; }
};

class GraphicsProgramID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  bool operator!() const { return -1 == value; }
  explicit operator bool() const { return -1 == value; }

  uint32_t getIndex() const { return index; }
  uint32_t getGroup() const { return group; }

  int32_t exportValue() const { return value; }

  static GraphicsProgramID Null()
  {
    GraphicsProgramID result;
    result.value = -1;
    return result;
  }

  static GraphicsProgramID importValue(int32_t value)
  {
    GraphicsProgramID result;
    result.value = value;
    return result;
  }

  static GraphicsProgramID make(uint32_t group, uint32_t index)
  {
    GraphicsProgramID result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value < r.value; }

  friend bool operator>(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value > r.value; }

  friend bool operator!=(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value != r.value; }

  friend bool operator==(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value == r.value; }
};


// TODO rename some of the names to better resemble what they are intended for
enum class DeviceMemoryClass
{
  DEVICE_RESIDENT_IMAGE,
  DEVICE_RESIDENT_BUFFER,
  // linear cpu cached textures
  HOST_RESIDENT_HOST_READ_WRITE_IMAGE,
  // linear cpu non-cached textures
  HOST_RESIDENT_HOST_WRITE_ONLY_IMAGE,
  HOST_RESIDENT_HOST_READ_WRITE_BUFFER,

  HOST_RESIDENT_HOST_READ_ONLY_BUFFER,
  HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER,
  // special AMD memory type,
  // a portion of gpu mem is host
  // visible (256mb).
  DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER,
  // we handle memory for push ring buffer differently than any other
  PUSH_RING_BUFFER,
  TEMPORARY_UPLOAD_BUFFER,

  READ_BACK_BUFFER,
  BIDIRECTIONAL_BUFFER,

  RESERVED_RESOURCE,

#if DX12_USE_ESRAM
  ESRAM_RESOURCE,
#endif

#if D3D_HAS_RAY_TRACING
  DEVICE_RESIDENT_ACCELERATION_STRUCTURE,
#endif

  COUNT,
  INVALID = COUNT
};

inline constexpr UINT calculate_subresource_index(UINT mip_slice, UINT array_slice, UINT plane_slice, UINT mip_size, UINT array_size)
{
  return mip_slice + (array_slice * mip_size) + (plane_slice * mip_size * array_size);
}

inline constexpr UINT calculate_mip_slice_from_index(UINT index, UINT mip_size) { return index % mip_size; }

inline constexpr UINT calculate_array_slice_from_index(UINT index, UINT mip_size, UINT array_size)
{
  return (index / mip_size) % array_size;
}

inline constexpr UINT calculate_plane_slice_from_index(UINT index, UINT mip_size, UINT array_size)
{
  return (index / mip_size) / array_size;
}

inline D3D12_COMPARISON_FUNC translate_compare_func_to_dx12(int cmp) { return static_cast<D3D12_COMPARISON_FUNC>(cmp); }
inline D3D12_STENCIL_OP translate_stencil_op_to_dx12(int so) { return static_cast<D3D12_STENCIL_OP>(so); }
inline D3D12_BLEND translate_alpha_blend_mode_to_dx12(int b) { return static_cast<D3D12_BLEND>(b); }
inline D3D12_BLEND translate_rgb_blend_mode_to_dx12(int b) { return static_cast<D3D12_BLEND>(b); }
inline D3D12_BLEND_OP translate_blend_op_to_dx12(int bo) { return static_cast<D3D12_BLEND_OP>(bo); }
inline D3D12_TEXTURE_ADDRESS_MODE translate_texture_address_mode_to_dx12(int mode)
{
  return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(mode);
}
inline int translate_texture_address_mode_to_engine(D3D12_TEXTURE_ADDRESS_MODE mode) { return static_cast<int>(mode); }
inline D3D12_FILTER_TYPE translate_filter_type_to_dx12(int ft)
{
  return (ft == TEXFILTER_POINT || ft == TEXFILTER_NONE) ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
}
inline D3D12_FILTER_TYPE translate_mip_filter_type_to_dx12(int ft)
{
  return (ft == TEXMIPMAP_POINT || ft == TEXMIPMAP_NONE) ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
}

inline SamplerState SamplerState::fromSamplerInfo(const d3d::SamplerInfo &info)
{
  SamplerState state;
  state.isCompare = info.filter_mode == d3d::FilterMode::Compare;
  state.setFilter(translate_filter_type_to_dx12(static_cast<int>(info.filter_mode)));
  state.setMip(translate_mip_filter_type_to_dx12(static_cast<int>(info.mip_map_mode)));
  state.setU(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_u)));
  state.setV(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_v)));
  state.setW(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_w)));
  state.setBias(info.mip_map_bias);
  state.setAniso(info.anisotropic_max);
  state.setBorder(info.border_color);
  return state;
}

class Device;
Device &get_device();

inline const Offset3D &toOffset(const Extent3D &ext)
{
  // sanity checks
  G_STATIC_ASSERT(offsetof(Extent3D, width) == offsetof(Offset3D, x));
  G_STATIC_ASSERT(offsetof(Extent3D, height) == offsetof(Offset3D, y));
  G_STATIC_ASSERT(offsetof(Extent3D, depth) == offsetof(Offset3D, z));
  return reinterpret_cast<const Offset3D &>(ext);
}
} // namespace drv3d_dx12

inline uint32_t nextPowerOfTwo(uint32_t u)
{
  --u;
  u |= u >> 1;
  u |= u >> 2;
  u |= u >> 4;
  u |= u >> 8;
  u |= u >> 16;
  return ++u;
}

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE l, D3D12_CPU_DESCRIPTOR_HANDLE r) { return l.ptr == r.ptr; }

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE l, D3D12_CPU_DESCRIPTOR_HANDLE r) { return !(l == r); }

inline D3D12_PRIMITIVE_TOPOLOGY pimitive_type_to_primtive_topology(D3D_PRIMITIVE pt, D3D12_PRIMITIVE_TOPOLOGY initial)
{
  if (pt >= D3D_PRIMITIVE_1_CONTROL_POINT_PATCH && pt <= D3D_PRIMITIVE_32_CONTROL_POINT_PATCH)
    return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(
      D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + pt - D3D_PRIMITIVE_1_CONTROL_POINT_PATCH);
  return initial;
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_to_topology_type(D3D12_PRIMITIVE_TOPOLOGY top)
{
  if (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST <= top && D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ >= top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  if (D3D_PRIMITIVE_TOPOLOGY_POINTLIST == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  if (D3D_PRIMITIVE_TOPOLOGY_LINELIST == top || D3D_PRIMITIVE_TOPOLOGY_LINESTRIP == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  if (D3D_PRIMITIVE_TOPOLOGY_UNDEFINED == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
#if _TARGET_XBOX
  if (D3D_PRIMITIVE_TOPOLOGY_QUADLIST == top)
    return PRIMITIVE_TOPOLOGY_TYPE_QUAD;
#endif
  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE operator+(D3D12_GPU_DESCRIPTOR_HANDLE l, uint64_t r) { return {l.ptr + r}; }

inline D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE l, size_t r) { return {l.ptr + r}; }

inline wchar_t *lazyToWchar(const char *str, wchar_t *buf, size_t max_len)
{
  auto ed = str + max_len - 1;
  auto at = buf;
  for (; *str && str != ed; ++str, ++at)
    *at = *str;
  *at = L'\0';
  return buf;
}


inline bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE l, D3D12_GPU_DESCRIPTOR_HANDLE r) { return l.ptr == r.ptr; }

inline bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE l, D3D12_GPU_DESCRIPTOR_HANDLE r) { return !(l == r); }

#define D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION _HRESULT_TYPEDEF_(0x887E0003L)

namespace drv3d_dx12
{
void report_oom_info();
void set_last_error(HRESULT error);
HRESULT get_last_error_code();
inline const char *dxgi_error_code_to_string(HRESULT ec)
{
#define ENUM_CASE(Name) \
  case Name: return #Name
  switch (ec)
  {
    ENUM_CASE(E_FAIL); // returned by init code if a step fails in a fatal way
    ENUM_CASE(DXGI_ERROR_INVALID_CALL);
    ENUM_CASE(DXGI_ERROR_NOT_FOUND);
    ENUM_CASE(DXGI_ERROR_MORE_DATA);
    ENUM_CASE(DXGI_ERROR_UNSUPPORTED);
    ENUM_CASE(DXGI_ERROR_DEVICE_REMOVED);
    ENUM_CASE(DXGI_ERROR_DEVICE_HUNG);
    ENUM_CASE(DXGI_ERROR_DEVICE_RESET);
    ENUM_CASE(DXGI_ERROR_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_FRAME_STATISTICS_DISJOINT);
    ENUM_CASE(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE);
    ENUM_CASE(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    ENUM_CASE(DXGI_ERROR_NONEXCLUSIVE);
    ENUM_CASE(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
    ENUM_CASE(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_REMOTE_OUTOFMEMORY);
    ENUM_CASE(DXGI_ERROR_ACCESS_LOST);
    ENUM_CASE(DXGI_ERROR_WAIT_TIMEOUT);
    ENUM_CASE(DXGI_ERROR_SESSION_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE);
    ENUM_CASE(DXGI_ERROR_CANNOT_PROTECT_CONTENT);
    ENUM_CASE(DXGI_ERROR_ACCESS_DENIED);
    ENUM_CASE(DXGI_ERROR_NAME_ALREADY_EXISTS);
    ENUM_CASE(DXGI_STATUS_UNOCCLUDED);
    ENUM_CASE(DXGI_STATUS_DDA_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_MODE_CHANGE_IN_PROGRESS);
    ENUM_CASE(E_INVALIDARG);
    ENUM_CASE(E_OUTOFMEMORY);
#if _TARGET_PC_WIN
    ENUM_CASE(D3D12_ERROR_ADAPTER_NOT_FOUND);
    ENUM_CASE(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
    ENUM_CASE(D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION);
#endif
  }
#undef ENUM_CASE

  return "";
}

inline HRESULT dx12_check_result_no_oom_report(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

inline bool is_oom_error_code(HRESULT result) { return E_OUTOFMEMORY == result; }

inline HRESULT dx12_check_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  if (is_oom_error_code(result))
  {
    report_oom_info();
  }

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

inline bool is_recoverable_error(HRESULT error)
{
  switch (error)
  {
    default: return true;
    // any device error is not recoverable
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_RESET: return false;
  }
}

inline HRESULT dx12_debug_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    debug("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    debug("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}
} // namespace drv3d_dx12

#define DX12_DEBUG_RESULT(r) drv3d_dx12::dx12_debug_result(r, #r, __FILE__, __LINE__)
#define DX12_DEBUG_OK(r)     SUCCEEDED(DX12_DEBUG_RESULT(r))
#define DX12_DEBUG_FAIL(r)   FAILED(DX12_DEBUG_RESULT(r))

#define DX12_CHECK_RESULT(r) drv3d_dx12::dx12_check_result(r, #r, __FILE__, __LINE__)
#define DX12_CHECK_OK(r)     SUCCEEDED(DX12_CHECK_RESULT(r))
#define DX12_CHECK_FAIL(r)   FAILED(DX12_CHECK_RESULT(r))
#define DX12_EXIT_ON_FAIL(r) \
  if (DX12_CHECK_FAIL(r))    \
  {                          \
    /* no-op */              \
  }

#define DX12_CHECK_RESULT_NO_OOM_CHECK(r) drv3d_dx12::dx12_check_result_no_oom_report(r, #r, __FILE__, __LINE__)

struct UnloadLibHandler
{
  typedef HMODULE pointer;
  void operator()(HMODULE lib)
  {
    if (lib)
    {
      FreeLibrary(lib);
    }
  }
};

using LibPointer = eastl::unique_ptr<HMODULE, UnloadLibHandler>;

struct GenericHandleHandler
{
  typedef HANDLE pointer;
  void operator()(pointer h)
  {
    if (h != nullptr && h != INVALID_HANDLE_VALUE)
      CloseHandle(h);
  }
};

using HandlePointer = eastl::unique_ptr<HANDLE, GenericHandleHandler>;
using EventPointer = eastl::unique_ptr<HANDLE, GenericHandleHandler>;

struct VirtaulAllocMemoryHandler
{
  void operator()(void *ptr) { VirtualFree(ptr, 0, MEM_RELEASE); }
};

template <typename T>
using VirtualAllocPtr = eastl::unique_ptr<T, VirtaulAllocMemoryHandler>;

// TODO: move this to utils
inline D3D12_RECT asRect(const D3D12_VIEWPORT &vp)
{
  D3D12_RECT rect;
  rect.left = vp.TopLeftX;
  rect.top = vp.TopLeftY;
  rect.right = vp.TopLeftX + vp.Width;
  rect.bottom = vp.TopLeftY + vp.Height;
  return rect;
}
#if !_TARGET_XBOXONE
inline bool operator==(const D3D12_RECT &l, const D3D12_RECT &r)
{
  return l.left == r.left && l.top == r.top && l.right == r.right && l.bottom == r.bottom;
}
inline bool operator!=(const D3D12_RECT &l, const D3D12_RECT &r) { return !(l == r); }
#endif

template <typename T>
inline D3D12_RANGE asDx12Range(ValueRange<T> range)
{
  return {static_cast<SIZE_T>(range.front()), static_cast<SIZE_T>(range.back() + 1)};
}

struct MemoryStatus
{
  size_t allocatedBytes = 0;
  size_t freeBytes = 0;
  // memory still allocated that is kept for fast allocation or freed when another chunk is
  // freed. eg. zombie heaps.
  size_t reservedBytes = 0;
};

inline MemoryStatus operator+(const MemoryStatus &l, const MemoryStatus &r)
{
  MemoryStatus f = l;
  f.allocatedBytes += r.allocatedBytes;
  f.freeBytes += r.freeBytes;
  f.reservedBytes += r.reservedBytes;
  return f;
}

#if !_TARGET_XBOXONE
inline D3D12_SHADING_RATE_COMBINER map_shading_rate_combiner_to_dx12(VariableRateShadingCombiner combiner)
{
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_PASSTHROUGH == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_PASSTHROUGH));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_OVERRIDE == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_OVERRIDE));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_MIN == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_MIN));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_MAX == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_MAX));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_SUM == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_SUM));
  return static_cast<D3D12_SHADING_RATE_COMBINER>(combiner);
}

inline D3D12_SHADING_RATE make_shading_rate_from_int_values(unsigned x, unsigned y)
{
  G_ASSERTF_RETURN(x <= 4 && y <= 4, D3D12_SHADING_RATE_1X1, "Variable Shading Rate can not exceed 4");
  G_ASSERTF_RETURN(x != 3 && y != 3, D3D12_SHADING_RATE_1X1, "Variable Shading Rate can not be 3");
  G_ASSERTF_RETURN(abs(int(x / 2) - int(y / 2)) < 2, D3D12_SHADING_RATE_1X1,
    "Variable Shading Rate invalid combination of x=%u and y=%u shading rates", x, y);
  G_STATIC_ASSERT(D3D12_SHADING_RATE_X_AXIS_SHIFT == 2);
  G_STATIC_ASSERT(D3D12_SHADING_RATE_VALID_MASK == 3);
  // simple formula (x-rate / 2) << 2 | (y-rage / 2)
  // valid range for x and y are 1, 2 and 4
  return static_cast<D3D12_SHADING_RATE>(((x >> 1) << D3D12_SHADING_RATE_X_AXIS_SHIFT) | (y >> 1));
}
#endif

template <size_t N>
inline eastl::bitset<N> &or_bit(eastl::bitset<N> &s, size_t i, bool b = true)
{
  return s.set(i, s.test(i) || b);
}

// NOTE: This is intended for debug only, this is possibly slow, so use with care!
template <size_t N>
inline char *get_resource_name(ID3D12Resource *res, char (&cbuf)[N])
{
#if !_TARGET_XBOXONE
  wchar_t wcbuf[N];
  UINT cnt = sizeof(wcbuf);
  res->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
  eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
  cbuf[min<uint32_t>(cnt, N - 1)] = '\0';
#else
  G_UNUSED(res);
  cbuf[0] = 0;
#endif
  return cbuf;
}

template <size_t N>
inline char *append_literal(char *at, char *ed, const char (&lit)[N])
{
  auto d = static_cast<size_t>(ed - at);
  auto c = min(d, N - 1);
  memcpy(at, lit, c);
  return at + c;
}

template <size_t N>
inline char *append_or_mask_value_name(char *beg, char *at, char *ed, const char (&name)[N])
{
  if (beg != at)
  {
    at = append_literal(at, ed, " | ");
  }
  return append_literal(at, ed, name);
}

template <size_t N>
inline char *resource_state_mask_as_string(D3D12_RESOURCE_STATES mask, char (&cbuf)[N])
{
  auto at = cbuf;
  auto ed = cbuf + N - 1;
  if (mask == D3D12_RESOURCE_STATE_COMMON)
  {
    at = append_literal(at, ed, "COMMON");
  }
  else
  {
#define CHECK_MASK(name)                                                   \
  if (D3D12_RESOURCE_STATE_##name == (mask & D3D12_RESOURCE_STATE_##name)) \
  {                                                                        \
    at = append_or_mask_value_name(cbuf, at, ed, #name);                   \
    mask ^= D3D12_RESOURCE_STATE_##name;                                   \
  }
    // combined state, has to be first
    CHECK_MASK(GENERIC_READ)
    // single state
    CHECK_MASK(VERTEX_AND_CONSTANT_BUFFER)
    CHECK_MASK(INDEX_BUFFER)
    CHECK_MASK(RENDER_TARGET)
    CHECK_MASK(UNORDERED_ACCESS)
    CHECK_MASK(DEPTH_WRITE)
    CHECK_MASK(DEPTH_READ)
    CHECK_MASK(NON_PIXEL_SHADER_RESOURCE)
    CHECK_MASK(PIXEL_SHADER_RESOURCE)
    CHECK_MASK(STREAM_OUT)
    CHECK_MASK(INDIRECT_ARGUMENT)
    CHECK_MASK(COPY_DEST)
    CHECK_MASK(COPY_SOURCE)
    CHECK_MASK(RESOLVE_DEST)
    CHECK_MASK(RESOLVE_SOURCE)
#if !_TARGET_XBOXONE
    CHECK_MASK(RAYTRACING_ACCELERATION_STRUCTURE)
    CHECK_MASK(SHADING_RATE_SOURCE)
#endif
    CHECK_MASK(PREDICATION)
    CHECK_MASK(VIDEO_DECODE_READ)
    CHECK_MASK(VIDEO_DECODE_WRITE)
    CHECK_MASK(VIDEO_PROCESS_READ)
    CHECK_MASK(VIDEO_PROCESS_WRITE)
    CHECK_MASK(VIDEO_ENCODE_READ)
    CHECK_MASK(VIDEO_ENCODE_WRITE)
#undef CHECK_MASK
  }
  *at = '\0';
  return cbuf;
}

inline const char *to_string(D3D12_RESOURCE_DIMENSION dim)
{
  switch (dim)
  {
    default: return "<unknown>";
    case D3D12_RESOURCE_DIMENSION_UNKNOWN: return "unknown";
    case D3D12_RESOURCE_DIMENSION_BUFFER: return "buffer";
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D: return "texture 1D";
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D: return "texture 2D";
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return "texture 3D";
  }
}

inline const char *to_string(D3D12_HEAP_TYPE type)
{
  switch (type)
  {
    case D3D12_HEAP_TYPE_DEFAULT: return "default";
    case D3D12_HEAP_TYPE_UPLOAD: return "upload";
    case D3D12_HEAP_TYPE_READBACK: return "read back";
    case D3D12_HEAP_TYPE_CUSTOM: return "custom";
  }
  return "??";
}

inline const char *to_string(D3D12_CPU_PAGE_PROPERTY property)
{
  switch (property)
  {
    case D3D12_CPU_PAGE_PROPERTY_UNKNOWN: return "unknown";
    case D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE: return "not available";
    case D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE: return "write combine";
    case D3D12_CPU_PAGE_PROPERTY_WRITE_BACK: return "write back";
  }
  return "??";
}

inline const char *to_string(D3D12_MEMORY_POOL pool)
{
  switch (pool)
  {
    case D3D12_MEMORY_POOL_UNKNOWN: return "unknown";
    case D3D12_MEMORY_POOL_L0: return "L0";
    case D3D12_MEMORY_POOL_L1: return "L1";
  }
  return "??";
}

inline const char *get_unit_name(uint32_t index)
{
  static const char *unitTable[] = {"Bytes", "KiBytes", "MiBytes", "GiBytes"};
  return unitTable[index];
}

inline uint32_t size_to_unit_table(uint64_t sz)
{
  uint32_t unitIndex = 0;
  unitIndex += sz >= (1024 * 1024 * 1024);
  unitIndex += sz >= (1024 * 1024);
  unitIndex += sz >= (1024);
  return unitIndex;
}

inline float compute_unit_type_size(uint64_t sz, uint32_t unit_index) { return static_cast<float>(sz) / (powf(1024, unit_index)); }

class ByteUnits
{
  uint64_t size = 0;

public:
  ByteUnits() = default;

  ByteUnits(const ByteUnits &) = default;
  ByteUnits &operator=(const ByteUnits &) = default;


  ByteUnits(uint64_t v) : size{v} {}
  ByteUnits &operator=(uint64_t v)
  {
    size = v;
    return *this;
  }

  ByteUnits &operator+=(ByteUnits o)
  {
    size += o.size;
    return *this;
  }
  ByteUnits &operator-=(ByteUnits o)
  {
    size -= o.size;
    return *this;
  }

  friend ByteUnits operator+(ByteUnits l, ByteUnits r) { return {l.size + r.size}; }
  friend ByteUnits operator-(ByteUnits l, ByteUnits r) { return {l.size - r.size}; }

  uint64_t value() const { return size; }
  float units() const { return compute_unit_type_size(size, size_to_unit_table(size)); }
  const char *name() const { return get_unit_name(size_to_unit_table(size)); }
};

template <typename T>
inline T align_value(T value, T alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

inline Extent3D align_value(const Extent3D &value, const Extent3D &alignment)
{
  return //
    {align_value(value.width, alignment.width), align_value(value.height, alignment.height),
      align_value(value.depth, alignment.depth)};
}

namespace drv3d_dx12
{
union HeapID
{
  using ValueType = uint32_t;
  static constexpr uint32_t alias_bits = 1;
  static constexpr uint32_t group_bits = 4;
  static constexpr uint32_t index_bits = (8 * sizeof(ValueType)) - group_bits - alias_bits;

  ValueType raw = 0;
  struct
  {
    ValueType isAlias : alias_bits;
    ValueType group : group_bits;
    ValueType index : index_bits;
  };
};

#if _TARGET_XBOX
struct VirtualFreeCaller
{
  void operator()(void *pointer) { VirtualFree(pointer, 0, MEM_RELEASE); }
};

class ResourceMemory
{
  uint8_t *heap = nullptr;
  uint32_t sz = 0;
  HeapID heapID;

public:
  ResourceMemory() = default;
  ~ResourceMemory() = default;

  ResourceMemory(const ResourceMemory &) = default;
  ResourceMemory &operator=(const ResourceMemory &) = default;

  ResourceMemory(ResourceMemory &&) = default;
  ResourceMemory &operator=(ResourceMemory &&) = default;

  ResourceMemory(uint8_t *h, uint32_t s, HeapID heap_id) : heap{h}, sz{s}, heapID{heap_id} {}

  explicit operator bool() const { return heap != nullptr; }

  uint32_t size() const { return sz; }

  uintptr_t getAddress() const { return reinterpret_cast<uintptr_t>(heap); }

  uint8_t *asPointer() const { return heap; }

  ResourceMemory subRange(uint32_t offset, uint32_t o_size) const
  {
    G_ASSERT(offset + o_size <= size());
    return {heap + offset, o_size, heapID};
  }

  ResourceMemory aliasSubRange(uint32_t new_index, uint32_t offset, uint32_t o_size) const
  {
    G_ASSERT(offset + o_size <= size());
    HeapID newHeapID = heapID;
    newHeapID.isAlias = 1;
    newHeapID.index = new_index;
    return {heap + offset, o_size, newHeapID};
  }

  bool isSubRangeOf(const ResourceMemory &mem) const
  {
    // NOTE: this can not check heapID as aliasing may change the heap id (from a real heap to a aliasing heap).
    return make_value_range(heap, size()).isSubRangeOf(make_value_range(mem.heap, mem.size()));
  }

  bool intersectsWith(const ResourceMemory &mem) const
  {
    return make_value_range(heap, size()).overlaps(make_value_range(mem.heap, mem.size()));
  }

  uint32_t calculateOffset(const ResourceMemory &sub) const { return sub.heap - heap; }

  HeapID getHeapID() const { return heapID; }
};
#else
class ResourceMemory
{
  ID3D12Heap *heap = nullptr;
  ValueRange<uint32_t> range;
  HeapID heapID;

public:
  ResourceMemory() = default;
  ~ResourceMemory() = default;

  ResourceMemory(const ResourceMemory &) = default;
  ResourceMemory &operator=(const ResourceMemory &) = default;

  ResourceMemory(ResourceMemory &&) = default;
  ResourceMemory &operator=(ResourceMemory &&) = default;

  ResourceMemory(ID3D12Heap *h, ValueRange<uint32_t> r, HeapID heap_id) : heap{h}, range{r}, heapID{heap_id} {}

  ID3D12Heap *getHeap() const { return heap; }

  ValueRange<uint32_t> getRange() const { return range; }

  explicit operator bool() const { return heap != nullptr; }

  uint32_t size() const { return range.size(); }

  uintptr_t getOffset() const { return range.front(); }

  ResourceMemory subRange(uint32_t offset, uint32_t o_size) const
  {
    G_ASSERT(offset + o_size <= range.size());
    ResourceMemory r;
    r.heap = heap;
    r.range = make_value_range(getOffset() + offset, o_size);
    r.heapID = heapID;
    return r;
  }

  ResourceMemory aliasSubRange(uint32_t new_index, uint32_t offset, uint32_t o_size) const
  {
    G_ASSERT(offset + o_size <= range.size());
    ResourceMemory r;
    r.heap = heap;
    r.range = make_value_range(getOffset() + offset, o_size);
    r.heapID = heapID;
    r.heapID.isAlias = 1;
    r.heapID.index = new_index;
    return r;
  }

  bool isSubRangeOf(const ResourceMemory &mem) const
  {
    // NOTE: this can not check heapID as aliasing may change the heap id (from a real heap to a aliasing heap).
    if (mem.heap != heap)
    {
      return false;
    }
    return range.isSubRangeOf(mem.range);
  }

  bool intersectsWith(const ResourceMemory &mem) const { return (heap == mem.heap) && range.overlaps(mem.range); }

  uint32_t calculateOffset(const ResourceMemory &sub) const { return sub.range.start - range.start; }

  HeapID getHeapID() const { return heapID; }
};
#endif

#if DX12_ENABLE_MT_VALIDATION
class DriverMutex
{
  WinCritSec mutex;
  bool enabled = false;
  uint32_t recursionCount = 0;

public:
  void lock()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    // NOTE: During startup this will fire at least once as the work cycle just takes the lock to check
    //       the d3d context device state.
    G_ASSERTF(true == enabled, "DX12: Trying to lock the driver context without enabling "
                               "multithreading first");
#endif
    mutex.lock();
    ++recursionCount;
  }

  void unlock()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    G_ASSERTF(true == enabled, "DX12: Trying to unlock the driver context without enabling "
                               "multithreading first");
#endif
    --recursionCount;
    mutex.unlock();
  }

  bool validateOwnership()
  {
    if (mutex.tryLock())
    {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
      bool owns = false;
      if (enabled)
      {
        // If we end up here the two thing can either be true:
        // 1) this thread has taken the lock before this, then recustionCount is greater than 0
        // 2) no thread has taken the lock before this, then recursionCount is equal to 0
        owns = recursionCount > 0;
      }
      else
      {
        // if MT is not enabled, only the main thread can access it without taking a lock.
        owns = owns || (0 == recursionCount) && is_main_thread();
      }
#else
      // DX11 behavior replicating expression, it has a flaw, which allows a race between the main
      // thread and a different thread as long as MT is disabled.
      bool owns = (!enabled && (0 == recursionCount) && is_main_thread()) || (recursionCount > 0);
#endif
      mutex.unlock();
      return owns;
    }
    // Failed to take the mutex, some other thread has it, so we don't own it.
    return false;
  }

  void enableMT()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    // NOTE: This locking will show ordering issues during startup, esp with splash screen.
    mutex.lock();
    G_ASSERT(false == enabled);
#endif
    enabled = true;
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.unlock();
#endif
  }

  void disableMT()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.lock();
    G_ASSERT(true == enabled);
#endif
    enabled = false;
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.unlock();
#endif
  }
};
#else
class DriverMutex
{
  WinCritSec mutex;

public:
  void lock() { mutex.lock(); }

  void unlock() { mutex.unlock(); }

  bool validateOwnership() { return true; }

  void enableMT() {}

  void disableMT() {}
};
#endif

struct HostDeviceSharedMemoryRegion
{
  enum class Source
  {
    TEMPORARY, // TODO: may split into temporary upload and readback (bidirectional makes no sense)
    PERSISTENT_UPLOAD,
    PERSISTENT_READ_BACK,
    PERSISTENT_BIDIRECTIONAL,
    PUSH_RING, // illegal to free
  };
  // buffer object that supplies the memory
  ID3D12Resource *buffer = nullptr;
#if _TARGET_XBOX
  // on xbox gpu and cpu pointer are the same
  union
  {
    D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
    uint8_t *pointer;
  };
#else
  // offset into gpu virtual memory, including offset!
  D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
  // pointer into cpu visible memory, offset is already applied (pointer - offset yields address
  // base of the buffer)
  uint8_t *pointer = nullptr;
#endif
  // offset range of this allocation
  // gpuPointer and pointer already have the start of the range added to it
  ValueRange<size_t> range;
  Source source = Source::TEMPORARY;

  explicit constexpr operator bool() const { return nullptr != buffer; }
  constexpr bool isTemporary() const { return Source::TEMPORARY == source; }
  void flushRegion(ValueRange<uint32_t> sub_range) const
  {
    D3D12_RANGE r = {};
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r = asDx12Range(sub_range.shiftBy(range.front()));
    buffer->Unmap(0, &r);
  }
  void invalidateRegion(ValueRange<uint32_t> sub_range) const
  {
    D3D12_RANGE r = asDx12Range(sub_range.shiftBy(range.front()));
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(pointer + sub_range.front() == ptr + range.front());
    r.Begin = r.End = 0;
    buffer->Unmap(0, &r);
  }
  void flush() const
  {
    D3D12_RANGE r = {};
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r = asDx12Range(range);
    buffer->Unmap(0, &r);
  }
  void invalidate() const
  {
    D3D12_RANGE r = asDx12Range(range);
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r.Begin = r.End = 0;
    buffer->Unmap(0, &r);
  }
  template <typename T>
  T *as() const
  {
    return reinterpret_cast<T *>(pointer);
  }
};
} // namespace drv3d_dx12

template <typename T, size_t BIT_COUNT = static_cast<size_t>(T::COUNT)>
class TypedBitSet : private eastl::bitset<BIT_COUNT>
{
  using BaseType = eastl::bitset<BIT_COUNT>;

public:
  using BaseType::all;
  using BaseType::any;
  using BaseType::count;
  using BaseType::flip;
  using BaseType::none;
  // using BaseType::to_string;
  using BaseType::to_ulong;
  // using BaseType::to_ullong;
  using BaseType::size;
  using typename BaseType::reference;

  TypedBitSet() = default;
  TypedBitSet(const TypedBitSet &) = default;
  ~TypedBitSet() = default;

  TypedBitSet &operator=(const TypedBitSet &) = default;

  bool operator[](T index) const { return (*this)[static_cast<size_t>(index)]; }
  typename BaseType::reference operator[](T index) { return (*this)[static_cast<size_t>(index)]; }
  bool test(T index) const { return BaseType::test(static_cast<size_t>(index)); }

  TypedBitSet &set()
  {
    BaseType::set();
    return *this;
  }

  TypedBitSet &set(T index, bool value = true)
  {
    BaseType::set(static_cast<size_t>(index), value);
    return *this;
  }

  TypedBitSet &reset()
  {
    BaseType::reset();
    return *this;
  }

  TypedBitSet &reset(T index)
  {
    BaseType::reset(static_cast<size_t>(index));
    return *this;
  }

  TypedBitSet operator~() const
  {
    auto cpy = *this;
    cpy.flip();
    return cpy;
  }

  bool operator==(const TypedBitSet &other) const { return BaseType::operator==(other); }

  bool operator!=(const TypedBitSet &other) const { return BaseType::operator!=(other); }

  // extended stuff
  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &set(T0 v0, T1 v1, Ts... vs)
  {
    set(v0);
    set(v1, vs...);
    return *this;
  }

  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &reset(T0 v0, T1 v1, Ts... vs)
  {
    reset(v0);
    reset(v1, vs...);
    return *this;
  }

  template <typename T0, typename... Ts>
  TypedBitSet(T0 v0, Ts... vs)
  {
    set(v0, vs...);
  }
};

#if _TARGET_PC_WIN
inline DXGI_QUERY_VIDEO_MEMORY_INFO max(const DXGI_QUERY_VIDEO_MEMORY_INFO &l, const DXGI_QUERY_VIDEO_MEMORY_INFO &r)
{
  DXGI_QUERY_VIDEO_MEMORY_INFO result;
  result.Budget = max(l.Budget, r.Budget);
  result.CurrentUsage = max(l.CurrentUsage, r.CurrentUsage);
  result.AvailableForReservation = max(l.AvailableForReservation, r.AvailableForReservation);
  result.CurrentReservation = max(l.CurrentReservation, r.CurrentReservation);
  return result;
}
#endif

template <size_t N>
inline eastl::span<const char> string_literal_span(const char (&sl)[N])
{
  return {sl, N - 1};
}

template <size_t N>
inline eastl::span<const wchar_t> string_literal_span(const wchar_t (&sl)[N])
{
  return {sl, N - 1};
}

inline bool is_valid_allocation_info(const D3D12_RESOURCE_ALLOCATION_INFO &info)
{
  // On error DX12 returns ~0 in the SizeInBytes member.
  return 0 != ~info.SizeInBytes;
}

inline uint64_t get_next_resource_alignment(uint64_t alignment, uint32_t samples)
{
  if (D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT == alignment)
  {
    return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }
  if (samples > 1 && (D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT == alignment))
  {
    return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
  }

  return alignment;
}

// NOTE: may adjust desc.Alignment if the requested alignment could not be used
inline D3D12_RESOURCE_ALLOCATION_INFO get_resource_allocation_info(ID3D12Device *device, D3D12_RESOURCE_DESC &desc)
{
  G_ASSERTF(desc.Alignment != 0, "DX12: desc.Alignment should not be 0!");
  auto result = device->GetResourceAllocationInfo(0, 1, &desc);
  if (!is_valid_allocation_info(result))
  {
    auto nextAlignment = get_next_resource_alignment(desc.Alignment, desc.SampleDesc.Count);
    if (nextAlignment != desc.Alignment)
    {
      desc.Alignment = nextAlignment;
      result = device->GetResourceAllocationInfo(0, 1, &desc);
    }
  }
  return result;
}

inline void report_resource_alloc_info_error(const D3D12_RESOURCE_DESC &desc)
{
  logerr("DX12: Error while querying resource allocation info, resource desc: %s, %u, %u x %u x "
         "%u, %u, %s, %u by %u, %u, %08X",
    to_string(desc.Dimension), desc.Alignment, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels,
    drv3d_dx12::dxgi_format_name(desc.Format), desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Layout, desc.Flags);
}

inline uint64_t calculate_texture_alignment(uint64_t width, uint32_t height, uint32_t depth, uint32_t samples,
  D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flags, drv3d_dx12::FormatStore format)
{
  if (D3D12_TEXTURE_LAYOUT_UNKNOWN != layout)
  {
    if (samples > 1)
    {
      return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }

  if ((1 == samples) && ((D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) & flags))
  {
    return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }

  uint32_t blockSizeX = 1, blockSizeY = 1;
  auto bytesPerBlock = format.getBytesPerPixelBlock(&blockSizeX, &blockSizeY);
  const uint32_t textureWidthInBlocks = (width + blockSizeX - 1) / blockSizeX;
  const uint32_t textureHeightInBlocks = (height + blockSizeY - 1) / blockSizeY;

  const uint32_t TILE_MEM_SIZE = 4 * 1024;
  const uint32_t blocksInTile = TILE_MEM_SIZE / bytesPerBlock;
  // MSDN documentation says about "near-equilateral" size for the tile
  const uint32_t blocksInTileX = nextPowerOfTwo(sqrt(blocksInTile));
  const uint32_t blocksInTileY = nextPowerOfTwo(blocksInTile / blocksInTileX);
  const uint32_t MAX_TILES_COUNT_FOR_SMALL_RES = 16;
  const uint32_t tilesCount = ((textureWidthInBlocks + blocksInTileX - 1) / blocksInTileX) *
                              ((textureHeightInBlocks + blocksInTileY - 1) / blocksInTileY) * depth;
  // This check is neccessary according to debug layer and dx12 documentation:
  // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
  const bool smallAmountOfTiles = tilesCount <= MAX_TILES_COUNT_FOR_SMALL_RES;

  if (samples > 1)
  {
    if (smallAmountOfTiles)
    {
      return D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }
  else
  {
    if (smallAmountOfTiles)
    {
      return D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }
}

// Applies function object 'f' to each bit index of each set bit in bit mask 'm'.
template <typename F>
inline void for_each_set_bit(uint32_t m, F f)
{
  while (0 != m)
  {
    uint32_t i = __bsf_unsafe(m);
    m ^= 1u << i;
    f(i);
  }
}

// Very simple wrapper to make a non thread safe container thread safe with the help of a paired mutex.
// Access is grated with a AccessToken type, which grants access to the containers interface.
template <typename T, typename MTX>
class ContainerMutexWrapper
{
  MTX mtx;
  T container;

  void lock() { mtx.lock(); }

  void unlock() { mtx.unlock(); }

  T &data() { return container; }

public:
  ContainerMutexWrapper() = default;
  ~ContainerMutexWrapper() = default;

  ContainerMutexWrapper(const ContainerMutexWrapper &) = delete;
  ContainerMutexWrapper &operator=(const ContainerMutexWrapper &) = delete;

  ContainerMutexWrapper(ContainerMutexWrapper &&) = delete;
  ContainerMutexWrapper &operator=(ContainerMutexWrapper &&) = delete;

  class AccessToken
  {
    ContainerMutexWrapper *parent = nullptr;

  public:
    AccessToken() = default;
    ~AccessToken()
    {
      if (parent)
      {
        parent->unlock();
      }
    }

    AccessToken(ContainerMutexWrapper &p) : parent{&p} { parent->lock(); }

    AccessToken(const AccessToken &) = delete;
    AccessToken &operator=(const AccessToken &) = delete;

    AccessToken(AccessToken &&other) : parent{other.parent} { other.parent = nullptr; }
    AccessToken &operator=(AccessToken &&other)
    {
      eastl::swap(parent, other.parent);
      return *this;
    }

    T &operator*() { return parent->data(); }
    T *operator->() { return &parent->data(); }
  };

  AccessToken access() { return {*this}; }
};

// Input is 4x8 bits color channel mask and output will be a 8bit mask of render targets
inline uint32_t color_channel_mask_to_render_target_mask(uint32_t mask)
{
  // For each color chanel generate the used bit
  const uint32_t channel0 = mask >> 0;
  const uint32_t channel1 = mask >> 1;
  const uint32_t channel2 = mask >> 2;
  const uint32_t channel3 = mask >> 3;
  // At this point the lower bit of each 4 bit block is now indicating if a target is used or not
  const uint32_t channelsToSpacedTargetMask = channel0 | channel1 | channel2 | channel3;
  // This erases the top 3 bits of each 4 bit block to compress from 4x8 bits to 8 bits.
  const uint32_t t0 = (channelsToSpacedTargetMask >> 0) & 0x00000001;
  const uint32_t t1 = (channelsToSpacedTargetMask >> 3) & 0x00000002;
  const uint32_t t2 = (channelsToSpacedTargetMask >> 6) & 0x00000004;
  const uint32_t t3 = (channelsToSpacedTargetMask >> 9) & 0x00000008;
  const uint32_t t4 = (channelsToSpacedTargetMask >> 12) & 0x00000010;
  const uint32_t t5 = (channelsToSpacedTargetMask >> 15) & 0x00000020;
  const uint32_t t6 = (channelsToSpacedTargetMask >> 18) & 0x00000040;
  const uint32_t t7 = (channelsToSpacedTargetMask >> 21) & 0x00000080;
  const uint32_t combinedTargetMask = t0 | t1 | t2 | t3 | t4 | t5 | t6 | t7;
  return combinedTargetMask;
}

// Inputs a 8 bit mask of render targets and outputs a 4x8 channel mask, where if a target bit is
// set all corresponding channel bits will be set
inline uint32_t render_target_mask_to_color_channel_mask(uint32_t mask)
{
  // Spread out the individual target bits into the lowest bit of each corresponding 4 bit block,
  // which is the indicator bit for the first channel (r)
  const uint32_t t0 = (mask & 0x00000001) << 0;
  const uint32_t t1 = (mask & 0x00000002) << 3;
  const uint32_t t2 = (mask & 0x00000004) << 6;
  const uint32_t t3 = (mask & 0x00000008) << 9;
  const uint32_t t4 = (mask & 0x00000010) << 12;
  const uint32_t t5 = (mask & 0x00000020) << 15;
  const uint32_t t6 = (mask & 0x00000040) << 18;
  const uint32_t t7 = (mask & 0x00000080) << 21;
  const uint32_t r = t0 | t1 | t2 | t3 | t4 | t5 | t6 | t7;
  // Replicate indicator bits from first channel (r) to all others (g, b and a)
  const uint32_t g = r << 1;
  const uint32_t b = r << 2;
  const uint32_t a = r << 3;
  return r | g | b | a;
}

// Takes a 4x8 bit render target output channel mask and turns it into a 4x8 render target ouput mask
// where if any channel of a target is enabled all channels of the result are enabled.
// Simply speaking it turns all non 0 hex digits in the mask into F and all 0 are keept as 0.
inline uint32_t spread_color_chanel_mask_to_render_target_color_channel_mask(uint32_t mask)
{
  const uint32_t r = mask & 0x11111111;
  const uint32_t g = mask & 0x22222222;
  const uint32_t b = mask & 0x44444444;
  const uint32_t a = mask & 0x88888888;
  const uint32_t r1 = r | (r << 1) | (r << 2) | (r << 3);
  const uint32_t g1 = g | (g << 1) | (g << 2) | (g >> 1);
  const uint32_t b1 = b | (b << 1) | (b >> 1) | (b >> 2);
  const uint32_t a1 = a | (a >> 1) | (a >> 2) | (a >> 3);
  return r1 | g1 | b1 | a1;
}