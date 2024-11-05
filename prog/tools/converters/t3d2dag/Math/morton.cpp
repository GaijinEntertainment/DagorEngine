// Copyright (C) Gaijin Games KFT.  All rights reserved.

/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "morton.h"

int even_dilate(const int val)
{
  int u = ((val & 0x0000ff00) << 8) | (val & 0x000000ff);
  int v = ((u & 0x00f000f0) << 4) | (u & 0x000f000f);
  int w = ((v & 0x0c0c0c0c) << 2) | (v & 0x03030303);
  int r = ((w & 0x22222222) << 1) | (w & 0x11111111);
  return r;
}
