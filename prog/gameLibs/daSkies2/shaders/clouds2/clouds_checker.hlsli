#ifndef CLOUDS_CHECKER_HLSLI_INCLUDED
#define CLOUDS_CHECKER_HLSLI_INCLUDED 1

//quarter-rate checkerboard trace: the 2x2 sub-position traced at the given frame,
//quincunx walk (0,0)(0,1)(1,1)(1,0). The single source of the pattern for the
//trace dispatch remap and the TAA fresh-texel test
uint2 clouds_checker_sub(uint frame)
{
  return uint2((0xCu >> (frame & 3u)) & 1u, (0x6u >> (frame & 3u)) & 1u);
}

#endif
