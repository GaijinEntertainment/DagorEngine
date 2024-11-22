// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "tagged_types.h"

#include <debug/dag_assert.h>
#include <value_range.h>


namespace drv3d_dx12
{

class ImageGlobalSubresourceId
{
protected:
  static constexpr uint32_t invalid_id = 0x00FFFFFF;

  uint32_t value = invalid_id;

  constexpr ImageGlobalSubresourceId(uint32_t v) : value{v} {}

  friend class ExtendedImageGlobalSubresourceId;

public:
  constexpr ImageGlobalSubresourceId() = default;
  ~ImageGlobalSubresourceId() = default;
  constexpr ImageGlobalSubresourceId(const ImageGlobalSubresourceId &) = default;
  ImageGlobalSubresourceId &operator=(const ImageGlobalSubresourceId &) = default;

  constexpr bool isValid() const { return invalid_id != value; }
  constexpr uint32_t index() const { return value; }

  static ImageGlobalSubresourceId make(uint32_t value)
  {
    G_ASSERT(value == (value & invalid_id));
    return {value};
  }

  static constexpr ImageGlobalSubresourceId makec(uint32_t value) { return {value}; }

  static constexpr ImageGlobalSubresourceId make_invalid() { return {}; }

  ImageGlobalSubresourceId &operator+=(uint32_t r)
  {
    G_ASSERT(isValid());
    value += r;
    return *this;
  }

  ImageGlobalSubresourceId &operator-=(uint32_t r)
  {
    G_ASSERT(isValid());
    value -= r;
    return *this;
  }

  ImageGlobalSubresourceId &operator++()
  {
    G_ASSERT(isValid());
    ++value;
    return *this;
  }

  ImageGlobalSubresourceId operator++(int) const
  {
    G_ASSERT(isValid());
    auto copy = *this;
    return ++copy;
  }

  ImageGlobalSubresourceId &operator--()
  {
    G_ASSERT(isValid());
    --value;
    return *this;
  }

  ImageGlobalSubresourceId operator--(int) const
  {
    G_ASSERT(isValid());
    auto copy = *this;
    return --copy;
  }

  operator DagorSafeArg() const { return {index()}; }

  constexpr SubresourceIndex toSubresouceIndex(ImageGlobalSubresourceId base) const
  {
    return SubresourceIndex::make(index() - base.index());
  }
};

inline constexpr ImageGlobalSubresourceId swapchain_color_texture_global_id = ImageGlobalSubresourceId::makec(0);
inline constexpr ImageGlobalSubresourceId swapchain_secondary_color_texture_global_id = ImageGlobalSubresourceId::makec(1);
inline constexpr ImageGlobalSubresourceId first_dynamic_texture_global_id = ImageGlobalSubresourceId::makec(2);

inline constexpr ImageGlobalSubresourceId operator+(const ImageGlobalSubresourceId &l, uint32_t r)
{
  return ImageGlobalSubresourceId::makec(l.index() + r);
}

inline constexpr ImageGlobalSubresourceId operator+(const ImageGlobalSubresourceId &l, SubresourceIndex r)
{
  return ImageGlobalSubresourceId::makec(l.index() + r.index());
}

inline constexpr ImageGlobalSubresourceId operator-(const ImageGlobalSubresourceId &l, uint32_t r)
{
  return ImageGlobalSubresourceId::makec(l.index() - r);
}

inline constexpr ImageGlobalSubresourceId operator-(const ImageGlobalSubresourceId &l, SubresourceIndex r)
{
  return ImageGlobalSubresourceId::makec(l.index() - r.index());
}

inline constexpr size_t operator-(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r)
{
  return l.index() - r.index();
}

inline constexpr bool operator==(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r)
{
  return l.index() == r.index();
}

inline constexpr bool operator!=(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r)
{
  return l.index() != r.index();
}

inline constexpr bool operator<(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r) { return l.index() < r.index(); }

inline constexpr bool operator<=(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r)
{
  return l.index() <= r.index();
}

inline constexpr bool operator>(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r) { return l.index() > r.index(); }

inline constexpr bool operator>=(const ImageGlobalSubresourceId &l, const ImageGlobalSubresourceId &r)
{
  return l.index() >= r.index();
}

using BareBoneImageGlobalSubresourceIdRange = ValueRange<ImageGlobalSubresourceId>;

class ExtendedImageGlobalSubresourceId
{
  using BareBoneType = ImageGlobalSubresourceId;
  static constexpr uint32_t invalid_id = BareBoneType::invalid_id;
  static constexpr uint32_t static_texture_bit = 1u << 31;
  static constexpr uint32_t report_transitions_bit = 1u << 30;
  static constexpr uint32_t index_mask = invalid_id;
  static constexpr uint32_t status_mask = ~index_mask;

  uint32_t value = invalid_id;

  constexpr ExtendedImageGlobalSubresourceId(uint32_t v) : value{v} {}

public:
  constexpr ExtendedImageGlobalSubresourceId() = default;
  ~ExtendedImageGlobalSubresourceId() = default;
  constexpr ExtendedImageGlobalSubresourceId(const ExtendedImageGlobalSubresourceId &) = default;
  ExtendedImageGlobalSubresourceId &operator=(const ExtendedImageGlobalSubresourceId &) = default;

  constexpr ImageGlobalSubresourceId asBareBone() const { return {index()}; }

  constexpr operator ImageGlobalSubresourceId() const { return asBareBone(); }

  static constexpr ExtendedImageGlobalSubresourceId make(ImageGlobalSubresourceId v) { return {v.index()}; }

  static ExtendedImageGlobalSubresourceId make(uint32_t v)
  {
    G_ASSERT(0 == (v & index_mask));
    return {v};
  }

  static ExtendedImageGlobalSubresourceId make_static(uint32_t v)
  {
    G_ASSERT(0 == (v & index_mask));
    return {v | static_texture_bit};
  }

  void setStatic() { value |= static_texture_bit; }
  void setNonStatic() { value &= ~static_texture_bit; }

  void enableTransitionReporting() { value |= report_transitions_bit; }
  void disableTransitionReporting() { value &= ~report_transitions_bit; }

  constexpr bool isValid() const { return invalid_id != (value & index_mask); }
  constexpr uint32_t index() const { return value & index_mask; }
  constexpr bool isStatic() const { return 0 != (value & static_texture_bit); }
  constexpr bool shouldReportTransitions() const { return 0 != (value & report_transitions_bit); }

  constexpr ExtendedImageGlobalSubresourceId add(uint32_t v) const { return {value + v}; }

  constexpr ExtendedImageGlobalSubresourceId add(SubresourceCount v) const { return {value + v.count()}; }

  constexpr ExtendedImageGlobalSubresourceId sub(uint32_t v) const { return {value - v}; }

  operator DagorSafeArg() const { return {index()}; }

  constexpr SubresourceIndex toSubresouceIndex(ImageGlobalSubresourceId base) const
  {
    return SubresourceIndex::make(index() - base.index());
  }
};

inline constexpr ExtendedImageGlobalSubresourceId operator+(const ExtendedImageGlobalSubresourceId &l, uint32_t r) { return l.add(r); }

inline constexpr ExtendedImageGlobalSubresourceId operator-(const ExtendedImageGlobalSubresourceId &l, uint32_t r) { return l.sub(r); }

using ExtendedImageGlobalSubresourceIdRange = ValueRange<ExtendedImageGlobalSubresourceId>;

} // namespace drv3d_dx12
