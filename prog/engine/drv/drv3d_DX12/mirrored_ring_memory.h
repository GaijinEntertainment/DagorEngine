// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <EASTL/utility.h>
#include <generic/dag_expected.h>
#if _TARGET_XBOX
#include <EASTL/vector.h>
#endif

namespace drv3d_dx12
{
// Which step of the mapping sequence failed. ProbeExhausted is produced only by
// the classic (pre-Win10) PC path; PhysicalPages only by the Xbox path.
enum class MirrorError
{
  ZeroSize,       // a zero-byte allocation was requested
  Reserve,        // address space reservation failed
  Split,          // splitting the placeholder in two failed
  Section,        // pagefile section creation failed
  MapView,        // mapping a view over the reservation failed
  PhysicalPages,  // physical page allocation failed (Xbox)
  ProbeExhausted, // classic path ran out of address-probe attempts
};

// Owns a mirrored virtual memory allocation: 2*capacity of contiguous virtual
// address space where [0, capacity) and [capacity, 2*capacity) alias the same
// physical memory. A linear write that starts in the first half and runs past
// capacity reappears at the start of the physical memory, letting a ring buffer
// store a wrapping object as one contiguous run without a separate tail copy.
class MirroredRingMemory
{
public:
  MirroredRingMemory() = default;
  MirroredRingMemory(MirroredRingMemory &&other) noexcept { other.swap(*this); }
  MirroredRingMemory &operator=(MirroredRingMemory &&other) noexcept
  {
    MirroredRingMemory tmp{eastl::move(other)};
    tmp.swap(*this);
    return *this;
  }
  MirroredRingMemory(const MirroredRingMemory &) = delete;
  MirroredRingMemory &operator=(const MirroredRingMemory &) = delete;
  ~MirroredRingMemory() { reset(); }

  // Creates a mirror sized to at least requested_bytes (rounded up to the
  // system allocation granularity), or the MirrorError describing the step that
  // failed. Does not log; the caller decides how to report a failure.
  static dag::Expected<MirroredRingMemory, MirrorError> create(size_t requested_bytes);

  // Unmaps and releases; safe on an empty instance.
  void reset();

  uint8_t *data() const { return base; }
  // Usable ring size in bytes (page-rounded, == physical bytes backing it).
  size_t size() const { return capacity; }
  explicit operator bool() const { return base != nullptr; }

private:
  // On PC the teardown path is chosen from the (immutable) platform API
  // availability, so nothing extra is stored. On Xbox the backing physical pages
  // must be kept so they can be unmapped and freed on teardown.
  void swap(MirroredRingMemory &o) noexcept
  {
    eastl::swap(base, o.base);
    eastl::swap(capacity, o.capacity);
#if _TARGET_XBOX
    eastl::swap(physicalPages, o.physicalPages);
#endif
  }

  uint8_t *base = nullptr;
  size_t capacity = 0;
#if _TARGET_XBOX
  // 64 KB physical page numbers backing the mapping, mapped into both halves.
  eastl::vector<uint64_t> physicalPages;
#endif
};
} // namespace drv3d_dx12

// to_string overloads live at global scope in the DX12 driver (see d3d12_utils.h).
inline const char *to_string(drv3d_dx12::MirrorError e)
{
  using drv3d_dx12::MirrorError;
  switch (e)
  {
    case MirrorError::ZeroSize: return "zero size requested";
    case MirrorError::Reserve: return "address space reservation failed";
    case MirrorError::Split: return "placeholder split failed";
    case MirrorError::Section: return "section creation failed";
    case MirrorError::MapView: return "view mapping failed";
    case MirrorError::PhysicalPages: return "physical page allocation failed";
    case MirrorError::ProbeExhausted: return "address probe exhausted";
  }
  return "unknown";
}
