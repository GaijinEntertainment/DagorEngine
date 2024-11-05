// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  STL.H                                                                 ##
// ##                                                                        ##
// ##  Common include header.  Includes most of STL and sets up some very    ##
// ##  common type definitions.                                              ##
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


#pragma warning(disable : 4786) // warning C4503: '  ' : decorated name length exceeded, name was truncated
#pragma warning(disable : 4503)
#pragma warning(disable : 4800) // int forced to bool


#include <string>
#include <map>
// #include <hash_map>
#include <vector>
#include <set>
#include <list>
#include <queue>
#include <deque>
#include <stack>
#include <iostream>
#include <algorithm>
#include <assert.h>

typedef std::string StdString;

typedef std::vector<StdString> StringVector;
typedef std::vector<StringVector> StringVectorVector;

typedef std::vector<int> IntVector;
typedef std::vector<char> CharVector;
typedef std::vector<short> ShortVector;
typedef std::vector<unsigned short> UShortVector;
typedef std::queue<int> IntQueue;
