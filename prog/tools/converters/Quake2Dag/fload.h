#ifndef FLOAD_H

#define FLOAD_H

// ############################################################################
// ##                                                                        ##
// ##  FLOAD.H                                                               ##
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


// Load a binary image block into memory.  Could come from a file on disk
// or a database.  Access method is not defined.  Will destruct allocated
// memory when Fload instance goes away!!!  Make your own local copy of
// it if you need it to stay around.

#include "stl.h"
#include "stringdict.h"

#include <stdio.h>

class Fload
{
public:
  Fload(const StdString &fname);

  ~Fload(void);

  void *GetData(void) const { return mData; };
  int GetLen(void) const { return mLen; };

  char *GetString(void);

private:
  StringRef mName;
  void *mData;
  int mLen;
  char *mReadLoc;
  int mReadLen;
};

#endif
