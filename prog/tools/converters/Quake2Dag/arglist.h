#ifndef ARGLIST_H
#define ARGLIST_H

#include "stl.h"

// ############################################################################
// ##                                                                        ##
// ##  ARGLIST.H                                                             ##
// ##                                                                        ##
// ##  Parses a string into a series of arguments.                           ##
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

class ArgList
{
public:
  ArgList(void);
  ArgList(const StdString &line);
  ~ArgList(void);
  ArgList(const ArgList &copy);
  ArgList &operator=(const ArgList &copy);
  ArgList &operator=(const StdString &line);

  void Set(const StdString &line);

  const StdString &operator[](int argc) const;
  StdString &operator[](int argc);

  operator StdString(void) const;

  // lower case to emulate an STL container
  bool empty(void) const;
  int size(void) const;
  void clear(void);

  void Push(const StdString &arg);

  const StringVector &Get(void) const { return mArgs; }

protected:
  void Copy(const ArgList &copy);
  StringVector mArgs;
};

#endif // ARGLIST_H
