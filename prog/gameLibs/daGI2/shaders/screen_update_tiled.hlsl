#ifndef SCREEN_UPDATE_TILED_INCLUDED
#define SCREEN_UPDATE_TILED_INCLUDED 1
#include <pcg_hash.hlsl>
#include <hammersley.hlsl>
//updates in even distributed 'tiles' + random pixel inside tile
uint2 pseudo_random_screen_coord(uint index, uint numSamples, uint2 screen_res, uint random)
{
  uint totalPixels = screen_res.x*screen_res.y;//this is const
  float aspectH = float(screen_res.y)/screen_res.x;
  uint2 tileRes;
  tileRes.x = max(1, floor(sqrt(numSamples/aspectH)));
  tileRes.y = max(1, floor(float(numSamples)/tileRes.x));
  uint2 tileWidthHeight = ceil(screen_res / float2(tileRes));
  uint2 tileCoord = uint2(index%tileRes.x, (index/tileRes.x)%tileRes.y);
  //tileCoord.y = 1;//tileRes.y-3;
  //return (tileCoord*tileSize + uint2(frame, frame/tileWidthHeight.x)%tileWidthHeight)%screen_res;
  //return ((tileCoord + 0.5)/float2(tileRes))*screen_res;
  //return random%screen_res;
  return ((tileCoord + float2(uint2(random, random>>16)%tileWidthHeight)/tileWidthHeight)/float2(tileRes))*screen_res;
  //return ((tileCoord + float2(uint2(frame, frame/tileWidthHeight.x)%tileWidthHeight)/tileWidthHeight)/float2(tileRes))*screen_res;
}
#endif