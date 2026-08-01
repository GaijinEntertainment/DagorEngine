// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <common/cycAlloc.h>

#include <cstdint>

// Unique Tag per case gives its own static state; wipe() resets between cases.
namespace
{
struct CorruptionTag
{};
struct PaddingTag
{};
struct ResizeTag
{};
struct ResizeRejectTag
{};
struct DeallocEdgeTag
{};
struct FlipFreeTag
{};
struct ZeroSizeTag
{};

// Fuse two freed cold blocks into one roomy 128-byte block so two later allocations
// share it and the second's offset is observable. flip() is what merges freed blocks:
// the first swaps the empty oldState in, the second fuses the pair.
template <class Alloc>
static void makeRoomyBlock()
{
  void *w0 = Alloc::allocate(64, 4, 0);
  void *w1 = Alloc::allocate(64, 4, 0);
  Alloc::deallocate(w1, 64);
  Alloc::deallocate(w0, 64);
  Alloc::flip();
  Alloc::flip();
}
} // namespace

// A cold block is filled by its triggering allocation, so consecutive allocs land
// in separate blocks; freeing the older block's item must not roll back the tail.
TEST_CASE("CycAlloc: freeing an older block's item does not corrupt the tail", "[cycAlloc]")
{
  using Alloc = CycAlloc<CorruptionTag>;
  Alloc::wipe();

  const size_t sz = 64;
  void *older = Alloc::allocate(sz, 8, 0); // fills its own block
  void *top = Alloc::allocate(sz, 8, 0);   // spills into the tail block
  REQUIRE(older != nullptr);
  REQUIRE(top != nullptr);

  Alloc::deallocate(older, sz); // non-tail free: must leave the tail untouched

  void *next = Alloc::allocate(sz, 8, 0);
  REQUIRE(next != nullptr);
  CHECK(next != top);
  const char *t = static_cast<const char *>(top);
  const char *n = static_cast<const char *>(next);
  CHECK((n + sz <= t || t + sz <= n)); // disjoint from the still-live top

  Alloc::deallocate(next, sz);
  Alloc::deallocate(top, sz);
  Alloc::wipe();
}

// flip() merges prior blocks into one, the only way two allocs share a block and
// padding folds into the earlier entry; a LIFO unwind of the pair must fully reclaim.
TEST_CASE("CycAlloc: padded top allocation reclaims fully", "[cycAlloc]")
{
  using Alloc = CycAlloc<PaddingTag>;
  Alloc::wipe();

  makeRoomyBlock<Alloc>();

  void *a = Alloc::allocate(12, 4, 0);  // at offset 0
  void *b = Alloc::allocate(16, 16, 0); // needs 4 bytes of padding after a
  REQUIRE(a != nullptr);
  REQUIRE(b != nullptr);
  REQUIRE(reinterpret_cast<uintptr_t>(b) % 16 == 0);

  Alloc::deallocate(b, 16);
  Alloc::deallocate(a, 12); // caller size 12 < recorded 16; must still reclaim

  // Full unwind means the next allocation reuses a's slot at offset 0.
  void *c = Alloc::allocate(12, 4, 0);
  CHECK(c == a);

  Alloc::deallocate(c, 12);
  Alloc::wipe();
}

// resizeInplace may only touch the current tail top; grow/shrink must move the
// used mark so the next allocation lands right after the resized block.
TEST_CASE("CycAlloc: resizeInplace grows and shrinks the tail top", "[cycAlloc]")
{
  using Alloc = CycAlloc<ResizeTag>;
  Alloc::wipe();

  // A cold block is sized to its triggering allocation, so spare capacity for a grow
  // only exists in a merged block.
  makeRoomyBlock<Alloc>();

  void *a = Alloc::allocate(32, 4, 0);
  REQUIRE(a != nullptr);

  REQUIRE(Alloc::resizeInplace(a, 64));
  void *b = Alloc::allocate(16, 4, 0);
  CHECK(b == static_cast<char *>(a) + 64); // grow pushed the used mark to 64
  Alloc::deallocate(b, 16);

  REQUIRE(Alloc::resizeInplace(a, 16));
  void *c = Alloc::allocate(8, 4, 0);
  CHECK(c == static_cast<char *>(a) + 16); // shrink pulled the used mark to 16
  Alloc::deallocate(c, 8);

  CHECK(Alloc::resizeInplace(a, 16));        // equal size: no-op success
  CHECK_FALSE(Alloc::resizeInplace(a, 200)); // over the 128-byte block capacity

  Alloc::deallocate(a, 16);
  Alloc::wipe();
}

// resizeInplace must refuse anything that is not the current tail top so it never
// reads sizeStack.back() for the wrong allocation.
TEST_CASE("CycAlloc: resizeInplace rejects null, freed, and non-top pointers", "[cycAlloc]")
{
  using Alloc = CycAlloc<ResizeRejectTag>;
  Alloc::wipe();

  CHECK_FALSE(Alloc::resizeInplace(nullptr, 16));

  void *x = Alloc::allocate(64, 4, 0);
  Alloc::deallocate(x, 64);
  CHECK_FALSE(Alloc::resizeInplace(x, 32)); // empty sizeStack after the free

  void *older = Alloc::allocate(64, 8, 0);      // fills its own block
  void *top = Alloc::allocate(64, 8, 0);        // spills into the tail block
  CHECK_FALSE(Alloc::resizeInplace(older, 32)); // older is not the tail owner
  Alloc::deallocate(top, 64);
  Alloc::deallocate(older, 64);
  Alloc::wipe();
}

// deallocate must ignore null and must key reclaim off the recorded size, not the
// caller size, so a mismatched caller size still fully unwinds the tail top.
TEST_CASE("CycAlloc: deallocate tolerates null and a mismatched caller size", "[cycAlloc]")
{
  using Alloc = CycAlloc<DeallocEdgeTag>;
  Alloc::wipe();

  Alloc::deallocate(nullptr, 0); // must be a no-op, not a crash

  void *a = Alloc::allocate(64, 8, 0);
  Alloc::deallocate(a, 8); // wrong caller size: recorded 64 still governs reclaim
  void *b = Alloc::allocate(64, 8, 0);
  CHECK(b == a); // slot fully reclaimed despite the mismatched size

  Alloc::deallocate(b, 64);
  Alloc::wipe();
}

// Freeing an allocation made before the last flip must decrement oldState, not
// state; a following flip then sees a leak-free oldState and does not assert.
TEST_CASE("CycAlloc: deallocate of a pre-flip allocation is tracked in oldState", "[cycAlloc]")
{
  using Alloc = CycAlloc<FlipFreeTag>;
  Alloc::wipe();

  void *a = Alloc::allocate(64, 8, 0);
  Alloc::flip();            // a now lives in oldState
  Alloc::deallocate(a, 64); // owner is not in state: must charge oldState
  Alloc::flip();            // asserts here if oldState.allocCount was not decremented

  void *b = Alloc::allocate(64, 8, 0); // allocator stays usable after the cycle
  CHECK(b != nullptr);
  Alloc::deallocate(b, 64);
  Alloc::wipe();
}

// A zero-size request returns null and must be a pure no-op: it may not bump used or
// fold padding, so the next real allocation lands exactly where it would have anyway.
TEST_CASE("CycAlloc: zero-size allocation returns null without perturbing the tail", "[cycAlloc]")
{
  using Alloc = CycAlloc<ZeroSizeTag>;
  Alloc::wipe();

  makeRoomyBlock<Alloc>();

  void *a = Alloc::allocate(32, 4, 0);
  REQUIRE(a != nullptr);

  CHECK(Alloc::allocate(0, 8, 0) == nullptr); // no block, no padding, no used bump

  void *b = Alloc::allocate(16, 4, 0);
  CHECK(b == static_cast<char *>(a) + 32); // still right after a, as if the zero call never happened

  Alloc::deallocate(b, 16);
  Alloc::deallocate(a, 32);
  Alloc::wipe();
}
