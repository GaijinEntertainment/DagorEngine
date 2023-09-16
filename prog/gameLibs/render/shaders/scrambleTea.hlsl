//http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
uint2 scramble_TEA(uint2 v)
{
  uint k[4] ={ 0xA341316Cu , 0xC8013EA4u , 0xAD90777Du , 0x7E95761Eu };

  uint y = v[0];
  uint z = v[1];
  uint sum = 0;

  #ifndef TEA_ITERATIONS
    #define TEA_ITERATIONS 3
  #endif
  UNROLL for(uint i = 0; i < TEA_ITERATIONS; ++i)
  {
    sum += 0x9e3779b9;
    y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
    z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
  }

  return uint2(y, z);
}
