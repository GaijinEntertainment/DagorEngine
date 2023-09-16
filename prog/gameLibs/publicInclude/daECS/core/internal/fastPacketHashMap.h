#pragma once
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <EASTL/unique_ptr.h>

// simple implementation of ever growing packet hash map
// it is linear probing with fixed distance - 4.
// it is supposed to work with keys of size 4 (uint32_t, float), and can be easily updated to support other fixed size keys
//(as easy, as one enable_if, reference implementation is already there)
// unlike robin hood (ska) it is always O(1) (however, memory usage can grow faster!)
// just about same speed as ska though
// can be updated to support erase

template <typename Key, typename Val>
struct FastPacketHashMap
{
  enum
  {
    CNT = 4,
    EMPTY_KEY = 0,
    INVALID_VAL = eastl::numeric_limits<Val>::max()
  };
  typedef Key key_type;
#pragma pack(push, 1)
  typedef struct
  {
    Val second;
  } val_type;
#pragma pack(pop)
  static __forceinline val_type *end() { return nullptr; }

  __forceinline const val_type *find(key_type key) const { return (const val_type *)findVal(key); }
  __forceinline val_type *find(key_type key) { return (val_type *)findVal(key); }

  __forceinline const Val *findVal(key_type key) const
  {
    G_STATIC_ASSERT(sizeof(key_type) == 4); // if this ever happen, use enable_if to call find4_s for keys of other size
    return find4_v(key);
  }

  __forceinline Val *findVal(key_type key) { return const_cast<Val *>(((const FastPacketHashMap *)this)->findVal(key)); }
  Val &operator[](key_type key)
  {
    Val *v = try_insert(packets, mask, key);
    if (v)
      return *v;
    grow((mask + 1) * 2);
    return (*this)[key];
  }
  uint32_t bucket_count() const { return mask + 1; }
  void clear()
  {
    mask = 0;
    packets = eastl::make_unique<Packet[]>(1);
  }

protected:
#pragma pack(push, 1)
  struct Packet
  {
    key_type keys[CNT] = {0};
    Val vals[CNT] = {INVALID_VAL};
  };
#pragma pack(pop)
  eastl::unique_ptr<Packet[]> packets = eastl::make_unique<Packet[]>(1);
  uint32_t mask = 0; // bucket size is mask +1

  __forceinline const Val *find4_s(key_type key) const
  {
    const uint32_t index = key & mask;
    auto &packet = packets[index];
    int id = (int32_t(packet.keys[0] == key) + 2 * int32_t(packet.keys[1] == key) + 3 * int32_t(packet.keys[2] == key) +
              4 * int32_t(packet.keys[3] == key));
    return id ? (((val_type *)&packet.vals[0]) - 1) + id : nullptr;
  }

  __forceinline const Val *find4_v(key_type key) const
  {
    const uint32_t index = key & mask;
    auto &packet = packets[index];
    vec4i keys = v_ldui((const int *)packet.keys);
    vec4i m = v_cmp_eqi(keys, v_splatsi(key));
    int mm = v_signmask(v_cast_vec4f(m));
    return mm == 0 ? nullptr : &packet.vals[__bsf_unsafe(mm)];
  }

  static __forceinline Val *try_insert(eastl::unique_ptr<Packet[]> &packets2, uint32_t mask2, const key_type key)
  {
    const uint32_t index = key & mask2;
    auto &packet = packets2[index];
    for (int i = 0; i != CNT; ++i)
    {
      if (packet.keys[i] == key)
        return &packet.vals[i];
      if (packet.keys[i] == EMPTY_KEY)
      {
        packet.keys[i] = key;
        return &packet.vals[i];
      }
    }
    return nullptr;
  }

  void grow(uint32_t newSize)
  {
    const uint32_t size = (mask + 1);
    const uint32_t newMask = newSize - 1;
    eastl::unique_ptr<Packet[]> newPackets = eastl::make_unique<Packet[]>(newSize);
    for (uint32_t i = 0; i < size; ++i)
      for (uint32_t j = 0; j < CNT; ++j)
      {
        if (packets[i].keys[j] == EMPTY_KEY)
          break;
        Val *v = try_insert(newPackets, newMask, packets[i].keys[j]);
        if (!v)
        {
          newPackets.reset();
          grow(newSize * 2);
          return;
        }
        *v = packets[i].vals[j];
      }
    mask = newMask;
    eastl::swap(packets, newPackets);
  }
};
