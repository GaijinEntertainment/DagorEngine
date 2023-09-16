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

#ifndef _MORTON_H_
#define _MORTON_H_

int even_dilate(const int val);
#define odd_dilate(val) ((even_dilate(val) << 1))

#define mIndex2D(row, col) (even_dilate(row) | odd_dilate(col))
#define mIndex2DPadded(row, col, size) \
  ((row < size) ? ((col < size) ? mIndex2D(row, col) : (size) * (size) + row) : (size) * (size + 1) + col)

#endif // _MORTON_H_
