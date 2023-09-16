#pragma once
#include <util/dag_stdint.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <daECS/core/event.h>
#include <daECS/core/internal/asserts.h>
#ifndef RESTRICT_FUN
#ifdef _MSC_VER
#define RESTRICT_FUN __declspec(restrict)
#else
#define RESTRICT_FUN
#endif
#endif

namespace ecs
{

template <int pow_of_two_capacity_shift = 16>
struct EventCircularBuffer
{
  // capacity has to be power-of-two, unless, we guarantee readAt/writeAt won't even overflow unsigned int.
  static constexpr unsigned capacity = 1 << pow_of_two_capacity_shift;
  static constexpr unsigned capacity_mask = capacity - 1;
  static constexpr unsigned header_size = sizeof(EntityId);

  uint32_t readAt = 0, writeAt = 0;
  eastl::unique_ptr<uint8_t[]> data = eastl::make_unique<uint8_t[]>(capacity + sizeof(Event) + sizeof(uint32_t)); // place for header
  uint32_t *alreadyRead = (uint32_t *)(data.get() + capacity + sizeof(Event));
  EventCircularBuffer() { *alreadyRead = 0; }

  uint32_t mask(uint32_t val) const { return val & capacity_mask; }
  uint32_t writeIndex() const { return mask(writeAt); }
  uint32_t readIndex() const { return mask(readAt); }
  bool empty() const { return readAt == writeAt; }
  bool full() const { return size() == capacity; };
  uint32_t size() const { return writeAt - *alreadyRead; }
  bool canFreeRead(uint32_t sz) const { return (*alreadyRead) + sz <= writeAt; };
  bool canRead(uint32_t sz) const { return readAt + sz <= writeAt; };
  bool canWrite(uint32_t sz) const { return size() + sz <= capacity; };
  template <unsigned alignment_>
  bool alignTo(uint32_t sz)
  {
    constexpr unsigned alignment = alignment_ == 0 ? unsigned(1) : alignment_; // this is to avoid potential mod by zero
    uint32_t alignDiff = (alignment_ != 0) ? ((alignment - (writeAt + header_size) % alignment) % alignment) : 0;
    sz += alignDiff + header_size;
    if (EASTL_LIKELY(eastl::max(writeIndex(), size()) + sz <= capacity))
      return true;
    return fillEndAndCanWrite(sz);
  }
  RESTRICT_FUN uint8_t *__restrict push(EntityId eid, uint32_t sz)
  {
    sz += sizeof(eid);
    G_FAST_ASSERT(canWrite(sz));
    uint8_t *ret = data.get() + writeIndex();
    writeAt += sz;
    new (ret, _NEW_INPLACE) EntityId(eid);
    return ret + sizeof(eid);
  }
  RESTRICT_FUN uint8_t *__restrict reading() { return data.get() + readIndex(); }
  uint32_t *justRead(uint32_t sz)
  {
    DAECS_EXT_FAST_ASSERT(canRead(sz));
    readAt += sz;
    return alreadyRead;
  }
  static void freeRead(uint32_t *already_read, uint32_t sz) { *already_read += sz; }
  void freeRead(uint32_t sz)
  {
    // DAECS_EXT_ASSERTF(canFreeRead(sz) && int(*alreadyRead) <= int(readAt)-int(sz),
    //   "%p, alreadyRead=%d writeAt=%d, readAt = %d, sz=%d", this, *alreadyRead, writeAt, readAt, sz);
    *alreadyRead += sz;
  }
  void normalize()
  {
    if (*alreadyRead == readAt && *alreadyRead == writeAt)
    {
      *alreadyRead = writeAt = readAt = 0;
    }
  }

  DAGOR_NOINLINE /* should be out of line, since it is rare */ bool fillEndAndCanWrite(uint32_t sz)
  {
    if (!canWrite(sz)) // not enough space at all!
      return false;
    const uint32_t excess = capacity - writeIndex();
    DAECS_EXT_FAST_ASSERT(sz >= sizeof(EntityId) + sizeof(Event) // otherwise we writing not EntityId+Event
                          && excess >= sizeof(EntityId)          // we require all Events to be aligned on 4 bytes
                          && excess < sz);                       // we can't be in this function unless excess >= sz
    uint8_t *fillAt = data.get() + writeIndex();
    memset(fillAt, 0xFFFFFFFF, 4);           // invalid entityId
    memset(fillAt + 4, 0, sizeof(uint32_t)); // type
    G_STATIC_ASSERT(sizeof(Event) == sizeof(uint32_t) + sizeof(uint32_t));
#if defined(__clang__)
#define OFFSETOF __builtin_offsetof
#else
#define OFFSETOF offsetof
#endif
    G_STATIC_ASSERT(OFFSETOF(Event, type) == 0);
    G_STATIC_ASSERT(OFFSETOF(Event, length) == sizeof(uint32_t));
    G_STATIC_ASSERT(OFFSETOF(Event, flags) == sizeof(uint32_t) + sizeof(uint16_t));
#undef OFFSETOF
    new (fillAt + 4 + 4, _NEW_INPLACE) uint32_t(excess - sizeof(EntityId)); // assume length are lowest bits
    writeAt += excess;
    return canWrite(sz);
  }
};

template <int pow_of_two_capacity_shift = 16>
struct DeferredEventsStorage
{
  typedef EventCircularBuffer<pow_of_two_capacity_shift> EventsBuffer;
  EventsBuffer active;
  G_STATIC_ASSERT(sizeof(EventsBuffer) == sizeof(void *) * 2 + sizeof(uint32_t) * 2);
  eastl::vector<EventsBuffer> buffers;
  bool canProcess() const { return buffers.empty(); }

  DAGOR_NOINLINE /* should be out of line, since it is rare */ void pushNewBuffer() { eastl::swap(buffers.emplace_back(), active); }

  template <class T> // todo: make emplace with variadic template?
  void emplaceEvent(EntityId eid, T &&event)
  {
    G_STATIC_ASSERT(Event::max_event_size + sizeof(EntityId) <= EventsBuffer::capacity);
    G_STATIC_ASSERT(sizeof(T) + sizeof(EntityId) <= EventsBuffer::capacity);
    constexpr uint32_t eventSize = sizeof(T);
    if (EASTL_UNLIKELY(!(active.template alignTo<(align_event_on_emplace ? alignof(T) : 0)>(eventSize)))) //-V562,547
      pushNewBuffer();
    G_STATIC_ASSERT(!align_event_on_emplace || alignof(T) == EVENT_ALIGNMENT);
    G_STATIC_ASSERT(eventSize % sizeof(EntityId) == 0); // all events should be at least 4 bytes aligned, regardless
    DAECS_EXT_ASSERT(eventSize == event.getLength());   //
    DAECS_EXT_ASSERT(!align_event_on_emplace || (active.writeIndex() % EVENT_ALIGNMENT == 0));
    auto at = active.push(eid, eventSize);
    if constexpr ((T::staticFlags() & EVFLG_DESTROY) == 0)
      memcpy(at, &event, eventSize);
    else
      new (at, _NEW_INPLACE) T(eastl::move(event));
  }

  RESTRICT_FUN void *__restrict allocateUntypedEvent(EntityId eid, uint32_t eventSize) // untyped event
  {
    // we can't send untyped events that require non-trivial move (not relocatable)
    // currently just assume it is same as non-trivial destruction.
    if (EASTL_UNLIKELY(!(active.template alignTo<(align_event_on_emplace ? EVENT_ALIGNMENT : 0)>(eventSize)))) //-V562,547
      pushNewBuffer();
    DAECS_EXT_FAST_ASSERT(eventSize <= EventsBuffer::capacity);
    DAECS_EXT_ASSERT(!align_event_on_emplace || (active.writeIndex() % EVENT_ALIGNMENT == 0));
    DAECS_EXT_ASSERT(!align_event_on_emplace || (eventSize % EVENT_ALIGNMENT == 0));
    DAECS_EXT_ASSERT(eventSize % sizeof(EntityId) == 0); // all events should be at least 4 bytes aligned, regardless
    return active.push(eid, eventSize);
  }
  static constexpr size_t maxEventSize() { return EventsBuffer::capacity - sizeof(EntityId); }
  size_t capacity() const { return active.capacity + active.capacity * buffers.size(); }
};

#undef RESTRICT_FUN

}; // namespace ecs
