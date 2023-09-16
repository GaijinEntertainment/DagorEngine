/*
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef __SGI_STL_HASH_FUN_H
#define __SGI_STL_HASH_FUN_H

#define __SGI_STL_HASH_FUNCTION__PROVIDE_COMPILER_WORKAROUND

#include <stddef.h>
#include <string>
#include <type_traits>

#pragma option push -w-inl

namespace std { 

//template <class Key> struct hash { };

inline size_t __stl_hash_string(const char* s)
{
  unsigned long h = 0; 
  for ( ; *s; ++s)
    h = 5*h + *s;
  
  return size_t(h);
}

template<> struct hash<char*>
{
  size_t operator()(const char* s) const { return __stl_hash_string(s); }
};

template<> struct hash<const char*>
{
  size_t operator()(const char* s) const { return __stl_hash_string(s); }
};

template<> struct hash<string>
{
	size_t operator()(const string &s) const 
    { return __stl_hash_string(s.c_str()); }
};

#ifndef __SGI_STL_HASH_FUNCTION__PROVIDE_COMPILER_WORKAROUND
template<> struct hash<const string>
{
	size_t operator()(const string &s) const
    { return __stl_hash_string(s.c_str()); }
};
#endif

/*template<> struct hash<char> {
  size_t operator()(char x) const { return x; }
};
template<> struct hash<unsigned char> {
  size_t operator()(unsigned char x) const { return x; }
};
template<> struct hash<signed char> {
  size_t operator()(unsigned char x) const { return x; }
};
template<> struct hash<short> {
  size_t operator()(short x) const { return x; }
};
template<> struct hash<unsigned short> {
  size_t operator()(unsigned short x) const { return x; }
};
template<> struct hash<int> {
  size_t operator()(int x) const { return x; }
};
template<> struct hash<unsigned int> {
  size_t operator()(unsigned int x) const { return x; }
};
template<> struct hash<long> {
  size_t operator()(long x) const { return x; }
};
template<> struct hash<unsigned long> {
  size_t operator()(unsigned long x) const { return x; }
};*/

}

#pragma option pop

#endif /* __SGI_STL_HASH_FUN_H */

// Local Variables:
// mode:C++
// End:
