//http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
// faster then TEA, and probably not worser
#ifndef WANG_HASH_INCLUDED
#define WANG_HASH_INCLUDED 1

uint wang_hash(uint seed)
{
  seed = (seed ^ 61) ^ (seed >> 16);
  seed *= 9;
  seed = seed ^ (seed >> 4);
  seed *= 0x27d4eb2d;
  seed = seed ^ (seed >> 15);
  return seed;
}

#endif