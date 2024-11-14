// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "tagged_types.h"

#include <debug/dag_assert.h>
#include <value_range.h>


namespace drv3d_dx12
{

class ImageGlobalSubresouceId
{
protected:
  static constexpr uint32_t invalid_id = 0x00FFFFFF;

  uint32_t value = invalid_id;

  constexpr ImageGlobalSubresouceId(uint32_t v) : value{v} {}

  friend class ExtendedImageGlobalSubresouceId;

public:
  constexpr ImageGlobalSubresouceId() = default;
  ~ImageGlobalSubresouceId() = default;
  constexpr ImageGlobalSubresouceId(const ImageGlobalSubresouceId &) = default;
  ImageGlobalSubresouceId &operator=(const ImageGlobalSubresouceId &) = default;

  constexpr bool isValid() const { return invalid_id != value; }
  constexpr uint32_t index() const { return value; }

  static ImageGlobalSubresouceId make(uint32_t value)
  {
    G_ASSERT(value == (value & invalid_id));
    return {value};
  }

  static constexpr ImageGlobalSubresouceId makec(uint32_t value) { return {value}; }

  static constexpr ImageGlobalSubresouceId make_invalid() { return {}; }

  ImageGlobalSubresouceId &operator+=(uint32_t r)
  {
    G_ASSERT(isValid());
    value += r;
    return *this;
  }

  ImageGlobalSubresouceId &operator-=(uint32_t r)
  {
    G_ASSERT(isValid());
    value -= r;
    return *this;
  }

  ImageGlobalSubresouceId &operator++()
  {
    G_ASSERT(isValid());
    ++value;
    return *this;
  }

  ImageGlobalSubresouceId operator++(int) const
  {
    G_ASSERT(isValid());
    auto copy = *this;
    return ++copy;
  }

  ImageGlobalSubresouceId &operator--()
  {
    G_ASSERT(isValid());
    --value;
    return *this;
  }

  ImageGlobalSubresouceId operator--(int) const
  {
    G_ASSERT(isValid());
    auto copy = *this;
    return --copy;
  }

  operator DagorSafeArg() const { return {index()}; }

  constexpr SubresourceIndex toSubresouceIndex(ImageGlobalSubresouceId base) const
  {
    return SubresourceIndex::make(index() - base.index());
  }
};

inline constexpr ImageGlobalSubresouceId swapchain_color_texture_global_id = ImageGlobalSubresouceId::makec(0);
inline constexpr ImageGlobalSubresouceId swapchain_secondary_color_texture_global_id = ImageGlobalSubresouceId::makec(1);
inline constexpr ImageGlobalSubresouceId first_dynamic_texture_global_id = ImageGlobalSubresouceId::makec(2);

inline constexpr ImageGlobalSubresouceId operator+(const ImageGlobalSubresouceId &l, uint32_t r)
{
  return ImageGlobalSubresouceId::makec(l.index() + r);
}

inline constexpr ImageGlobalSubresouceId operator+(const ImageGlobalSubresouceId &l, SubresourceIndex r)
{
  return ImageGlobalSubresouceId::makec(l.index() + r.index());
}

inline constexpr ImageGlobalSubresouceId operator-(const ImageGlobalSubresouceId &l, uint32_t r)
{
  return ImageGlobalSubresouceId::makec(l.index() - r);
}

inline constexpr ImageGlobalSubresouceId operator-(const ImageGlobalSubresouceId &l, SubresourceIndex r)
{
  return ImageGlobalSubresouceId::makec(l.index() - r.index());
}

inline constexpr size_t operator-(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() - r.index(); }

inline constexpr bool operator==(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() == r.index(); }

inline constexpr bool operator!=(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() != r.index(); }

inline constexpr bool operator<(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() < r.index(); }

inline constexpr bool operator<=(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() <= r.index(); }

inline constexpr bool operator>(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() > r.index(); }

inline constexpr bool operator>=(const ImageGlobalSubresouceId &l, const ImageGlobalSubresouceId &r) { return l.index() >= r.index(); }

using BareBoneImageGlobalSubresouceIdRange = ValueRange<ImageGlobalSubresouceId>;

class ExtendedImageGlobalSubresouceId
{
  using BareBoneType = ImageGlobalSubresouceId;
  static constexpr uint32_t invalid_id = BareBoneType::invalid_id;
  static constexpr uint32_t static_texture_bit = 1u << 31;
  static constexpr uint32_t report_transitions_bit = 1u << 30;
  static constexpr uint32_t index_mask = invalid_id;
  static constexpr uint32_t status_mask = ~index_mask;

  uint32_t value = invalid_id;

  constexpr ExtendedImageGlobalSubresouceId(uint32_t v) : value{v} {}

public:
  constexpr ExtendedImageGlobalSubresouceId() = default;
  ~ExtendedImageGlobalSubresouceId() = default;
  constexpr ExtendedImageGlobalSubresouceId(const ExtendedImageGlobalSubresouceId &) = default;
  ExtendedImageGlobalSubresouceId &operator=(const ExtendedImageGlobalSubresouceId &) = default;

  constexpr ImageGlobalSubresouceId asBareBone() const { return {index()}; }

  constexpr operator ImageGlobalSubresouceId() const { return asBareBone(); }

  static constexpr ExtendedImageGlobalSubresouceId make(ImageGlobalSubresouceId v) { return {v.index()}; }

  static ExtendedImageGlobalSubresouceId make(uint32_t v)
  {
    G_ASSERT(0 == (v & index_mask));
    return {v};
  }

  static ExtendedImageGlobalSubresouceId make_static(uint32_t v)
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

  constexpr ExtendedImageGlobalSubresouceId add(uint32_t v) const { return {value + v}; }

  constexpr ExtendedImageGlobalSubresouceId add(SubresourceCount v) const { return {value + v.count()}; }

  constexpr ExtendedImageGlobalSubresouceId sub(uint32_t v) const { return {value - v}; }

  operator DagorSafeArg() const { return {index()}; }

  constexpr SubresourceIndex toSubresouceIndex(ImageGlobalSubresouceId base) const
  {
    return SubresourceIndex::make(index() - base.index());
  }
};

inline constexpr ExtendedImageGlobalSubresouceId operator+(const ExtendedImageGlobalSubresouceId &l, uint32_t r) { return l.add(r); }

inline constexpr ExtendedImageGlobalSubresouceId operator-(const ExtendedImageGlobalSubresouceId &l, uint32_t r) { return l.sub(r); }

using ExtendedImageGlobalSubresouceIdRange = ValueRange<ExtendedImageGlobalSubresouceId>;

} // namespace drv3d_dx12
