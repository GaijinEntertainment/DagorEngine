#pragma once

#define USE_SHA1_HASH 0

#if USE_SHA1_HASH
#include <hash/sha1.h>
#define HASH_SIZE    20
#define HASH_CONTEXT sha1_context
#define HASH_UPDATE  sha1_update
#define HASH_INIT    sha1_starts
#define HASH_FINISH  sha1_finish

#else
#include <hash/BLAKE3/blake3.h>

#define HASH_SIZE    32
inline void blake3_finalize_32(const blake3_hasher *h, unsigned char *hash) { blake3_hasher_finalize(h, hash, HASH_SIZE); }
#define HASH_CONTEXT blake3_hasher
#define HASH_UPDATE  blake3_hasher_update
#define HASH_INIT    blake3_hasher_init
#define HASH_FINISH  blake3_finalize_32
#endif

#if HASH_SIZE == 20

#define HASH_TEMP_STRING "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
#define HASH_LIST_STRING "%02x/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
#define HASH_LIST(a)                                                                                                           \
  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], (a)[8], (a)[9], (a)[10], (a)[11], (a)[12], (a)[13], (a)[14], \
    (a)[15], (a)[16], (a)[17], (a)[18], (a)[19]

#else

#define HASH_TEMP_STRING \
  "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
#define HASH_LIST_STRING \
  "%02x/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"

#define HASH_LIST(a)                                                                                                              \
  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], (a)[8], (a)[9], (a)[10], (a)[11], (a)[12], (a)[13], (a)[14],    \
    (a)[15], (a)[16], (a)[17], (a)[18], (a)[19], (a)[20], (a)[21], (a)[22], (a)[23], (a)[24], (a)[25], (a)[26], (a)[27], (a)[28], \
    (a)[29], (a)[30], (a)[31]

#endif