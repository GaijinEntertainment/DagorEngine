#include <crypto/rand.h>
#include <debug/dag_assert.h>
#if USE_BCRYPT
#include "bcryptUtils.h"
#else
#include <openssl/rand.h>
#endif

namespace crypto
{

void rand_bytes(uint8_t *out_buf, size_t buf_size)
{
#if USE_BCRYPT
  BCRYPT_DO(BCryptGenRandom(NULL, out_buf, buf_size, BCRYPT_USE_SYSTEM_PREFERRED_RNG));
#else
  G_VERIFY(RAND_bytes(out_buf, (int)buf_size));
#endif
}

} // namespace crypto
