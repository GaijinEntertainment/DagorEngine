#ifndef VOLMAP_GI_25D_BUFFER_ENCODE_INCLUDED
#define VOLMAP_GI_25D_BUFFER_ENCODE_INCLUDED 1

#include <dagi_volmap_consts_25d.hlsli>

#define GI25D_FLOOR_MAX_ADDR ((GI_25D_RESOLUTION_X*GI_25D_RESOLUTION_Z)*4)
#define GI25D_INTERSECTED_MAX_ADDR (((GI_25D_RESOLUTION_X*GI_25D_RESOLUTION_Y*GI_25D_RESOLUTION_Z)/32*4) + GI25D_FLOOR_MAX_ADDR)

uint get_volmap_floor_addr_unsafe(uint2 coord)
{
  uint addr;
#if 0
// simple ALU, but cache locality is good only in x coord
  addr = (coord.x + coord.y*GI_25D_RESOLUTION_X)<<2;
#else
  //4x4 block
  addr = (((coord.x>>2) + (coord.y>>2)*(GI_25D_RESOLUTION_X/4))<<6) + ((coord.x&3)<<2) + ((coord.y&3)<<4);
#endif
  return addr;
}

uint get_intersected_bit_address(uint3 coord, out uint bit)
{
  uint blockAddr;
#if 1
// simple ALU, but cache locality is good only in z and x coord
  uint at = (coord.x + coord.y*GI_25D_RESOLUTION_X)*GI_25D_RESOLUTION_Y + coord.z;
  bit = at&31;
  blockAddr = (at>>5)<<2;
#else
  //4x4x2 block, better cache localitym a bit more ALU
  blockAddr = ((coord.x>>2) + (coord.y>>2)*(GI_25D_RESOLUTION_X/4) + (coord.z>>1)*((GI_25D_RESOLUTION_X/4)*(GI_25D_RESOLUTION_Z/4)))<<2;
  bit = (coord.x&3) + ((coord.y&3)<<2) + ((coord.z&1)<<4);
#endif
  return GI25D_FLOOR_MAX_ADDR + blockAddr;
}
#endif