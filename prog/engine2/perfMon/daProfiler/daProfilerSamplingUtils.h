#pragma once
// call stack format
// 0-word: 8 bit new samples, 8 bit same samples (as in previous call stack)
// [1..4] words: 64 bit tid
// [5..5+new_samples*3) words, 48-bits-pointers (3*new_samples) callstacks
// [5+new_samples*3, 9+new_samples*3) - ticks
// total size 9 + 3*new_samples words
static constexpr size_t sampling_allocated_additional_words = 8;
static constexpr size_t sampling_allocated_address_words = 3;

inline uint64_t to_64_bit_pointer_low(const uint16_t *__restrict p48)
{
  uint64_t addr = 0;
  memcpy(&addr, p48, 6);
  return addr;
}

__forceinline uint64_t as_uint64(const uint16_t *__restrict p)
{
  uint64_t addr = 0;
  memcpy(&addr, p, 8);
  return addr;
}

__forceinline uint64_t sampling_end_ticks(const uint16_t *__restrict end) { return as_uint64(end - 4); }

inline uint64_t to_64_bit_pointer(const uint16_t *__restrict p48) // with sign bit replication
{
  uint64_t addr = to_64_bit_pointer_low(p48);
  return addr | ((-int64_t(addr >> 47)) << 48); // sign bit replication is required for correct pointer resolution
}

__forceinline void to_48_bit_pointer(uint16_t *__restrict p48, uint64_t addr) { memcpy(p48, &addr, 6); } // -V1086

__forceinline uint32_t sampling_saved_cs_count(const uint16_t *__restrict p) { return uint8_t(*p); }

__forceinline uint32_t sampling_same_cs_count(const uint16_t *__restrict p) { return uint8_t((*p) >> 8); }

__forceinline uint64_t sampling_saved_tid(const uint16_t *__restrict p) { return to_64_bit_pointer(p + 1); }

__forceinline const uint16_t *__restrict sampling_saved_cs(const uint16_t *__restrict p) { return p + 4; }

__forceinline uint64_t sampling_saved_ticks(const uint16_t *__restrict p)
{
  return as_uint64(p + sampling_saved_cs_count(p) * sampling_allocated_address_words + sampling_allocated_additional_words - 4);
}

__forceinline uint32_t sampling_allocated_words(const uint32_t cs_count)
{
  return cs_count * sampling_allocated_address_words + sampling_allocated_additional_words;
}

__forceinline uint32_t sampling_allocated_words(const uint16_t *__restrict p)
{
  return sampling_allocated_words(sampling_saved_cs_count(p));
}
