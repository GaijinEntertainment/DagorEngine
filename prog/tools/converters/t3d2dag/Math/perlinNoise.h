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

#ifndef _PERLINNOISE_H_
#define _PERLINNOISE_H_

float noise1(const float x);
float noise2(const float x, const float y);
float noise3(const float x, const float y, const float z);

float turbulence2(const float x, const float y, float freq);
float turbulence3(const float x, const float y, const float z, float freq);

float tileableNoise1(const float x, const float w);
float tileableNoise2(const float x, const float y, const float w, const float h);
float tileableNoise3(const float x, const float y, const float z, const float w, const float h, const float d);

float tileableTurbulence2(const float x, const float y, const float w, const float h, float freq);
float tileableTurbulence3(const float x, const float y, const float z, const float w, const float h, const float d, float freq);

void initPerlin();


#endif // _PERLINNOISE_H_
