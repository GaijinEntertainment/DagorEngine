/**
	Utility extensions. We provide tuples with size up to 8 as well as 
	routines 'm2' - 'm8' with behavious idential to
	'make_pair' - 'make_octaple'

	@author Vladimir Prus <ghost@cs.msu.su>
	@version 0.1
	@todo Consider creating template operators similar to that in 'rel_ops'

	@file
*/


#ifndef __UTILITY_EXTENSIOINS__H__
#define __UTILITY_EXTENSIOINS__H__


#include <utility>

template<class T1, class T2>
	typename std::pair<T1, T2>
	m2(T1 f, T2 s)
	{
		return std::make_pair(f, s);
	}

#include "utility_extensions_generated.h"


#endif
