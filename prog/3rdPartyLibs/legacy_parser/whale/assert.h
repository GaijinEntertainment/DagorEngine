
// a C++ exception-throwing <assert.h> replacement.

#ifndef __WHALE__ASSERT_H

#define __WHALE__ASSERT_H

#undef _ASSERT_H
#define _ASSERT_H		// to prevent standard assert.h inclusion

#include <iostream>

class FailedAssertion
{
public:
	int line;
	const char *fn;
	const char *condition;
	
	FailedAssertion(int supplied_line, const char *supplied_fn, const char *supplied_condition)
	{
		line=supplied_line;
		fn=supplied_fn;
		condition=supplied_condition;
	}
};

#undef assert
#ifdef NDEBUG
	#define assert(condition) (void)((condition) ? true : false)
#else
	#define assert(condition) ((condition) ? true : assert_proc(__LINE__, __FILE__, #condition))
#endif

inline bool assert_proc(int line, const char *fn, const char *condition)
{
	std::cout << "\nAssertion failed: " << condition << ", file " << fn
		<< ", line " << line << ".\n";
	throw *new FailedAssertion(line, fn, condition);
	return false; // to please the compiler
}

#endif
