#ifndef UINT_NOISE1D
#define UINT_NOISE1D 1
  uint uint_noise1D(uint position, uint seed)
  {
    uint BIT_NOISE1 = 0x68E31DA4;
    uint BIT_NOISE2 = 0xB5297A4D;
    uint BIT_NOISE3 = 0x1B56C4E9;
    uint mangled = position;
    mangled *= BIT_NOISE1;
    mangled += seed;
    mangled ^= (mangled>>8);
    mangled += BIT_NOISE2;
    mangled ^= (mangled<<8);
    mangled *= BIT_NOISE3;
    mangled ^= (mangled>>8);
    return mangled;
  }

  uint uint_noise2D(uint posX, uint posY, uint seed)
  {
    uint PRIME_NUMBER = 198491317;
    return uint_noise1D(posX + (PRIME_NUMBER*posY), seed);
  }

  uint uint_noise3D(uint posX, uint posY, uint posZ, uint seed)
  {
    uint PRIME_NUMBER1 = 198491317;
    uint PRIME_NUMBER2 = 6542989;
    return uint_noise1D(posX + (PRIME_NUMBER1*posY) + (PRIME_NUMBER2*posZ), seed);
  }
#endif