// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClientInstance/sepClientInstanceTypes.h>

#include <dagCrypto/rand.h>


namespace sepclientinstance
{


// returns random value [0..999]
static int get_random_permille()
{
  uint32_t value = 0;
  crypto::rand_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(value));
  return value % 1000;
}


SepClientInstanceConfig::SepClientInstanceConfig() : sepUsageCalculatedPermilleValue(get_random_permille()) {}

} // namespace sepclientinstance
