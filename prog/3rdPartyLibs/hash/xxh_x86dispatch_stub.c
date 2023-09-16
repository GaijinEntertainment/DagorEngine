#include <xxhash.h>
#if defined (__cplusplus)
extern "C" {
#endif

XXH64_hash_t  XXH3_64bits_dispatch(const void* input, size_t len)
{return XXH3_64bits(input, len);}
XXH64_hash_t  XXH3_64bits_withSeed_dispatch(const void* input, size_t len, XXH64_hash_t seed)
{return XXH3_64bits_withSeed(input, len, seed);}
XXH64_hash_t  XXH3_64bits_withSecret_dispatch(const void* input, size_t len, const void* secret, size_t secretLen)
{return XXH3_64bits_withSecret(input, len, secret, secretLen);}
XXH_errorcode XXH3_64bits_update_dispatch(XXH3_state_t* state, const void* input, size_t len)
{return XXH3_64bits_update(state, input, len);}

XXH128_hash_t XXH3_128bits_dispatch(const void* input, size_t len)
{return XXH3_128bits(input, len);}
XXH128_hash_t XXH3_128bits_withSeed_dispatch(const void* input, size_t len, XXH64_hash_t seed)
{return XXH3_128bits_withSeed(input, len, seed);}
XXH128_hash_t XXH3_128bits_withSecret_dispatch(const void* input, size_t len, const void* secret, size_t secretLen)
{return XXH3_128bits_withSecret(input, len, secret, secretLen);}
XXH_errorcode XXH3_128bits_update_dispatch(XXH3_state_t* state, const void* input, size_t len)
{return XXH3_128bits_update(state, input, len);}

#if defined (__cplusplus)
}
#endif
