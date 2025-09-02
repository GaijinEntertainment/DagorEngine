// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define SPECIALIZE_MEMCPY_HELPER(TO, TYPE, OFS, FROM, SZ) \
  memcpy(((TYPE *__restrict)(TO)) + (OFS), (const TYPE *__restrict)(FROM), (SZ) * sizeof(TYPE))

#define SPECIALIZE_MEMCPY_TO_SOA(CSZ, TO, EID, FROM)                          \
  switch (CSZ)                                                                \
  {                                                                           \
    case 1: (TO)[EID] = *(const uint8_t *__restrict)(FROM); break;            \
    case 4: SPECIALIZE_MEMCPY_HELPER(TO, uint32_t, EID, FROM, 1); break;      \
    case 8: SPECIALIZE_MEMCPY_HELPER(TO, uint64_t, EID, FROM, 1); break;      \
    case 12: SPECIALIZE_MEMCPY_HELPER(TO, uint32_t, EID * 3, FROM, 3); break; \
    case 16: SPECIALIZE_MEMCPY_HELPER(TO, vec4i, EID, FROM, 1); break;        \
    default: memcpy(TO + (EID) * CSZ, FROM, CSZ);                             \
  }

#define SPECIALIZE_MEMCPY_HELPER_SOA2(DATA_TO, DATA_FROM, TYPE, OFS, SRC_OFS, CNT) \
  memcpy(((TYPE *__restrict)(DATA_TO)) + (OFS), ((TYPE *__restrict)(DATA_FROM)) + (SRC_OFS), sizeof(TYPE) * (CNT))

#define SPECIALIZE_MEMCPY_IN_SOA2(CSZ, DATA_TO, DATA_FROM, TO, FROM)                                  \
  switch (CSZ)                                                                                        \
  {                                                                                                   \
    case 1: (DATA_TO)[TO] = (DATA_FROM)[FROM]; break;                                                 \
    case 4: SPECIALIZE_MEMCPY_HELPER_SOA2(DATA_TO, DATA_FROM, uint32_t, TO, FROM, 1); break;          \
    case 8: SPECIALIZE_MEMCPY_HELPER_SOA2(DATA_TO, DATA_FROM, uint64_t, TO, FROM, 1); break;          \
    case 12: SPECIALIZE_MEMCPY_HELPER_SOA2(DATA_TO, DATA_FROM, uint32_t, TO * 3, FROM * 3, 3); break; \
    case 16: SPECIALIZE_MEMCPY_HELPER_SOA2(DATA_TO, DATA_FROM, vec4i, TO, FROM, 1); break;            \
    default: memcpy(DATA_TO + (TO) * CSZ, DATA_FROM + (FROM) * CSZ, CSZ);                             \
  }

#define SPECIALIZE_MEMCPY_IN_SOA(CSZ, DATA, TO, FROM) SPECIALIZE_MEMCPY_IN_SOA2(CSZ, DATA, DATA, TO, FROM)

#define SPECIALIZE_MEMCPY_HELPER2(TO, TYPE, FROM, SZ) \
  memcpy(((TYPE *__restrict)(TO)), (const TYPE *__restrict)(FROM), (SZ) * sizeof(TYPE))

#define SPECIALIZE_MEMCPY(CSZ, TO, FROM)                                            \
  switch (CSZ)                                                                      \
  {                                                                                 \
    case 1: *((uint8_t *__restrict)TO) = *(const uint8_t *__restrict)(FROM); break; \
    case 4: SPECIALIZE_MEMCPY_HELPER2(TO, uint32_t, FROM, 1); break;                \
    case 8: SPECIALIZE_MEMCPY_HELPER2(TO, uint64_t, FROM, 1); break;                \
    case 12: SPECIALIZE_MEMCPY_HELPER2(TO, uint32_t, FROM, 3); break;               \
    case 16: SPECIALIZE_MEMCPY_HELPER2(TO, vec4i, FROM, 1); break;                  \
    default: memcpy(TO, FROM, CSZ);                                                 \
  }

__forceinline bool specialized_mem_nequal(const void *__restrict a_, const void *__restrict b_, size_t sz)
{
  const uint8_t *__restrict a = (const uint8_t *)a_, *__restrict b = (const uint8_t *)b_;
  switch (sz)
  {
    case 1: return *a != *b; // bool
    case 4: return *(const uint32_t *__restrict)a != *(const uint32_t *__restrict)b;
    case 12: return memcmp((const uint32_t *)a, (const uint32_t *)b, 12) != 0;
    default: return memcmp(a, b, sz) != 0;
  }
}