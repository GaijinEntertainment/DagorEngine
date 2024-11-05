// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  LOADBMP.H                                                             ##
// ##                                                                        ##
// ##  Utility routines, reads and writes BMP files.  This is *windows*      ##
// ##  specific, the only piece of code that relies on windows and would     ##
// ##  need to be ported for other OS implementations.                       ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################


// Helper functions for turning a preloaded .BMP file into a bitmap in memory
// NOTE that the Bpp parameter is BYTES per pixel, not bits!
#include "stl.h"

class Bmp
{
public:
  unsigned char *LoadBMP(const StdString &fname, int &wid, int &hit, int &Bpp);
  unsigned char *LoadBMP(const unsigned char *data, int &wid, int &hit, int &Bpp);
  void SaveBMP(const char *fname, const unsigned char *data, int wid, int hit, int Bpp);
  void SaveTGA(const char *fname, const unsigned char *data, int wid, int hit, int Bpp);

  void SwapRGB(unsigned char *dest, const unsigned char *src, unsigned int size);
  void SwapVertically(unsigned char *dest, const unsigned char *src, int wid, int hit, int Bpp);
};
