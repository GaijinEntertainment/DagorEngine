/**	\file
	Some simple checks facilities.
	Here are two thing:
		- preconditions, which is check on function arguments
		- check, which is to assure internal function logic is OK
*/ 



#ifndef __CHECKS_H_
#define	__CHECKS_H_

#include <stdexcept>
#include <string>
#include <sstream>

using std::logic_error;
using std::string;
using std::ostringstream;

#include "text.h"
using text::format;

/// Exception that is thrown when check fails
class check_t : public std::logic_error
{
public:	
    check_t(const string &s, const char * file, int line) :
	    logic_error(string("check ") + s + " failed at " + file + ":" +
					((ostringstream&)(ostringstream() << line)).str())
		{} 
};

/// Exception that is thrown when precondition fails
class precondition_t : public std::logic_error
{
public:	
    precondition_t(const string &s, const char * file, int line) :
		logic_error(string("precondition ") + s + " failed at " + file + ":" +
					((ostringstream &)(ostringstream() << line)).str()) {} 
};



#if !defined( __DEBUG )
#define __DEBUG 2
#endif

#undef precondition
#undef preconditionx

#define PRECONDITION(p) preconditionx(p,#p)
#define precondition(p) preconditionx(p,#p)

#if __DEBUG < 1
#define preconditionx(p,s)   ((void)0)
#else
#define preconditionx(p,s)   \
    if(!(p)) {throw precondition_t(s,__FILE__,__LINE__);}
#endif

#undef check
#undef checkx

#define CHECK(p) checkx(p,#p)

#if __DEBUG < 2
#define checkx(p,s)    ((void)0)
#else
#define checkx(p,s)   \
    if(!(p)) {throw check_t(s,__FILE__,__LINE__);}
#endif

class traceback
{
	static string *frames;
public:
	class checkpoint {
    	string location;
	public:
    	checkpoint(const string &location) : location(location) {
        	traceback::reset();
		}	
        ~checkpoint() {
#if defined(__cpp_lib_uncaught_exceptions) && __cpp_lib_uncaught_exceptions >= 201411L
					if (std::uncaught_exceptions())
#else
					if (std::uncaught_exception())
#endif
						traceback::append(location);
			else traceback::reset();
        }
	};	

	static void reset() { init(); frames->clear(); }
    static void append(const string &s) 
		{ init(); frames->append(s); frames->append("\n"); }
    static void init() {
		if (!frames) frames = new string;
    }
    static const string &show() { init(); return *frames; }
};

#ifndef NO_TRACEBACK
#define routine(name) traceback::checkpoint cp_##name(#name);
#else
#define routine(name)
#endif

// This is obsolete
#if 0
// args must be enclosed in braces
#define THROW(condition, args)\
	if ( (condition) ) throw logic_error(format args);
#endif

// And this is current
#define THROW(condition, value)\
	if ( (condition) ) throw value;

#define WHERE_AUX2(f, l) f " : " #l
#define WHERE_AUX1(f,l)  WHERE_AUX2(f,l)
#define WHERE	WHERE_AUX1(__FILE__, __LINE__)


#endif
