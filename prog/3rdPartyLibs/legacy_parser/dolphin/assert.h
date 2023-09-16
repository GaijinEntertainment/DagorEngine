
// a C++ exception-throwing <assert.h> replacement.

#ifndef __DOLPHIN__ASSERT_H

#define __DOLPHIN__ASSERT_H

#undef _ASSERT_H
#define _ASSERT_H		// to prevent standard assert.h inclusion

#define __DOLPHIN__ASSERT_H__ATTEMPT_TO_PLEASE_THE_COMPILER // see below

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
	// it's better to print it now rather than to rely upon some good man
	// somewhere upward the call stack: it can easily happen that we crash
	// while *** the stack.
	std::cout << "\nAssertion failed: " << condition << ", file " << fn
		<< ", line " << line << ".\n";
	
	throw *new FailedAssertion(line, fn, condition);
	
#ifdef __DOLPHIN__ASSERT_H__ATTEMPT_TO_PLEASE_THE_COMPILER
	return false;
#endif
}

#endif
