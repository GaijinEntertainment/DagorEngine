// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <crypto/ssl.h>
#include <crypto/rand.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <osApiWrappers/dag_critSec.h>
#include <EASTL/unique_ptr.h>
#include <debug/dag_assert.h>
#include <math/random/dag_random.h>
#if _TARGET_PC_LINUX | _TARGET_ANDROID
#include <pthread.h>
#endif


namespace crypto
{
static eastl::unique_ptr<WinCritSec[]> ssl_lockarray;
static void crypto_lock_callback(int mode, int type, const char * /*file*/, int /*line*/)
{
  if (mode & CRYPTO_LOCK)
    ssl_lockarray[type].lock();
  else
    ssl_lockarray[type].unlock();
}

void init_ssl(InitSslFlags init_flags)
{
  if ((init_flags & InitSslFlags::InitRand) != InitSslFlags::None)
    RAND_status();

  if ((init_flags & InitSslFlags::InitLocking) != InitSslFlags::None)
  {
    CRYPTO_set_locking_callback(&crypto_lock_callback);
    if (CRYPTO_get_locking_callback() == &crypto_lock_callback) // locking API was deprecated in newer versions of openssl
    {
#if _TARGET_PC_LINUX | _TARGET_ANDROID
      CRYPTO_set_id_callback([]() -> unsigned long { return (unsigned long)pthread_self(); });
#endif
      ssl_lockarray.reset(new WinCritSec[CRYPTO_num_locks()]);
    }
  }

  if ((init_flags & InitSslFlags::InitDagorRandSeed) != InitSslFlags::None)
    crypto::rand_bytes((uint8_t *)&g_rnd_seed, sizeof(g_rnd_seed));
}

void cleanup_ssl()
{
  if (CRYPTO_get_locking_callback() == &crypto_lock_callback)
  {
    CRYPTO_set_locking_callback(NULL);
#if _TARGET_PC_LINUX | _TARGET_ANDROID
    CRYPTO_set_id_callback(NULL);
#endif
    ssl_lockarray.reset();
  }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
  OPENSSL_cleanup();
#else
  CRYPTO_cleanup_all_ex_data(); // this cannot be called multiple times according to libcurl, but without this we will leak memory on
                                // shutdown
#endif
}

} // namespace crypto
