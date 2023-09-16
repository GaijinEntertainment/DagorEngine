#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


// ############################################################################
// ##                                                                        ##
// ##  FLOAD.CPP                                                             ##
// ##                                                                        ##
// ##  Loads a file into a block of memory.                                  ##
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

#include "fload.h"


Fload::Fload(const StdString &fname)
{
  mName = SGET(fname);
  mData = 0;
  mLen = 0;

  FILE *fph = fopen(mName, "rb");
  if (fph)
  {
    fseek(fph, 0L, SEEK_END);
    mLen = ftell(fph);
    if (mLen)
    {
      fseek(fph, 0L, SEEK_SET);
      mData = (void *)new unsigned char[mLen];
      assert(mData);
      if (mData)
      {
        int r = fread(mData, mLen, 1, fph);
        assert(r);
        if (!r)
        {
          delete mData;
          mData = 0;
        }
      }
    }
    fclose(fph);
  }
  mReadLoc = (char *)mData;
  mReadLen = mLen;
}

Fload::~Fload(void) { delete mData; }


char *Fload::GetString(void)
{
  if (!mReadLoc)
  {
    return 0;
  }

  // now advance read pointer to end of string and stomp a zero
  // byte on top of it as a null string terminator.
  while (*mReadLoc == 0 || *mReadLoc == 10 || *mReadLoc == 13)
  {
    mReadLoc++;
    mReadLen--;
    if (!mReadLen)
    {
      mReadLoc = 0;
      return 0;
    }
  }

  char *ret = mReadLoc; // current read location is string location

  while (*mReadLoc && *mReadLoc != 10 && *mReadLoc != 13)
  {
    mReadLoc++;
    mReadLen--;
    if (!mReadLen)
    {
      mReadLoc = 0;
      break;
    }
  }

  if (mReadLen)
    *mReadLoc = 0; // replace line feeds with null terminated strings.

  if (mReadLoc)
    mReadLoc++;
  mReadLen--;

  if (!mReadLen)
  {
    mReadLoc = 0;
  }
  return ret;
}
