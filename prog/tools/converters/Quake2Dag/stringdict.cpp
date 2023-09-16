#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ############################################################################
// ##                                                                        ##
// ##  STRINGDICT.CPP                                                        ##
// ##                                                                        ##
// ##  Global collection of all strings in application, no duplicates.       ##
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

#include "stringdict.h"

StringDict *StringDict::gSingleton = 0;
