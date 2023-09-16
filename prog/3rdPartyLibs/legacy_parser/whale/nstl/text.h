
#ifndef __TEXT_H_C2229B00
#define __TEXT_H_C2229B00

/* 	TODO : atoi should check it's parameter and throw exception
	Behavior of basic_formatter copy constructor is obscure.

	Rev 0.3

*/



#include <string>
#include <sstream>
#include <list>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <iomanip>
#include <queue>
#include <cassert>


#ifndef NO_PCRE
#include <pcreposix.h>
#include <pcre.h>
#endif

#include "bases.h"
#ifndef _MSC_VER
  template <class _Arg>
  struct _Smanip { // store function pointer and argument value
      _Smanip(void(__cdecl* _Left)(std::ios_base&, _Arg), _Arg _Val) : _Pfun(_Left), _Manarg(_Val) {}

      void(__cdecl* _Pfun)(std::ios_base&, _Arg); // the function pointer
      _Arg _Manarg; // the argument value
  };
#endif


namespace text_internal
{
 	using namespace std;

    template<class charT, class traits>
    basic_string<charT, traits> &trim(basic_string<charT, traits>& s, charT c)
    { 
		typename basic_string<charT, traits>::size_type p1, p2;
        p1 = s.find_first_not_of(c); p2 = s.find_last_not_of(c);
        return 	s.erase(p2 + 1).erase(0, p1);
    }

    template<class charT, class traits>
    basic_string<charT, traits> &trim_ws(basic_string<charT, traits>& s)
    { 
		typename basic_string<charT, traits>::size_type p1, p2;
        p1 = s.find_first_not_of(" \t\n\r"); p2 = s.find_last_not_of(" \t\n\r");
        return 	s.erase(p2 + 1).erase(0, p1);
    }

    template<class charT, class traits>
    basic_string<charT, traits> trim_ws(const basic_string<charT, traits>& _s)
    { 
    	basic_string<charT, traits> s(_s);
		typename basic_string<charT, traits>::size_type p1, p2;
        p1 = s.find_first_not_of(" \t\n\r"); p2 = s.find_last_not_of(" \t\n\r");
        return 	s.erase(p2 + 1).erase(0, p1);
    }

    template<class charT, class traits>
    basic_string<charT, traits> chop(basic_string<charT, traits>& s)
    {
    	return s.erase(s.find_last_not_of("\r\n") + 1);
    }

	template<class charT, class traits>
    basic_string<charT, traits> chop(const basic_string<charT, traits>& s)
    {
    	basic_string<charT, traits> result(s);
    	return result.erase(result.find_last_not_of("\r\n") + 1);
    }


	string& quote(string& s);
    string& unquote(string& s);
	string& escape(string& s);
	string& unescape(string& s);

    template<class charT, class traits, class Container>
    void split(const basic_string<charT, traits>& str,
    		   const basic_string<charT, traits>& sep,
			   Container& words)
	{
   		int start, stop = 0;

		//while( start = str.find_first_not_of(sep, stop), (start >= 0 && stop >= 0))
		while (start = str.find_first_not_of(sep, stop), start != string::npos) 
    	{
       		stop = str.find_first_of(sep, start);
			words.push_back(str.substr(start, (stop == -1) ? -1 : stop - start)); 
		}
    }

    template<class charT, class traits>
    list< basic_string<charT, traits> >
    split(const basic_string<charT, traits>& str,
    	  const basic_string<charT, traits>& sep)
	{
		list< basic_string<charT, traits> > result;
		split(str, sep, result);
		return result;
	}

    class columnizer
    {
    	vector<unsigned> widthes;
        vector< list<string> > texts;
	public:
    	void add_column(unsigned width, const string& text);
        void flush(ostream& os);
    };

    inline
    string itos(int i) { stringstream s; s << i; return s.str(); }
    
    inline 
    int atoi(const string& s) { return atoi(s.c_str()); }

    string format(const char *fs, ... );

    template<class charT, class traits = char_traits<charT> >
    class basic_formatter 
    {
    protected:
    	typedef typename basic_string<charT, traits>::size_type size_type;
    	typedef basic_ios<charT, traits> ios_type;

    public:
    	basic_formatter(const string& format_str);
		basic_formatter(basic_ostream<charT, traits>& os,
        			    const string& format_str,
        			    const string& interleaver = string());
		~basic_formatter();

		basic_formatter& operator<<(ios_base& (*pf)(ios_base&));
		basic_formatter& operator<<(ios_type& (*pf)(ios_type&));
        template<class T> 
		basic_formatter& operator<<(const T& t);
        template<class T>
		basic_formatter& operator<<(const _Smanip<T>& m);

        basic_formatter& counter(int n);
        basic_string<charT, traits> str() const;
		operator basic_string<charT, traits>() const;
        void flush();

        static bool test();

   		// It is nessessary that only one formatter perform actual output	
		basic_formatter(const basic_formatter& a)
		: format_str(a.format_str), interleaver(a.interleaver),

		  os( &a.os == (basic_ostream<charT, traits>*)(&a.ssp) ? (basic_ostream<charT, traits>&)ssp : a.os),

		  format_specifier_pos(a.format_specifier_pos),
		  nested(a.nested),
		  remaining_repeats(a.remaining_repeats),
		  nested_counters(a.nested_counters),
		  fmt(a.fmt)
		{ ssp.str(a.ssp.str());	}


	protected:

    	basic_string<charT, traits> format_str;
        basic_string<charT, traits> interleaver;

   		basic_stringstream<charT, traits> ssp;   		    
        basic_ostream<charT, traits>& os;

		typename basic_string<charT, traits>::size_type format_specifier_pos;	
		basic_formatter* nested;
        int remaining_repeats;
	    queue<int> nested_counters;

		bool first_time;

        struct format {
        	ios_base::fmtflags flags;
            int width, prec;
            bool fill_with_zero;
            format() : flags(std::ios_base::fmtflags(~0U)), width(0), prec(0), fill_with_zero(false) {}
            format(ios_base::fmtflags flags, int width, int prec, bool fz)
            	: flags(flags), width(width), prec(prec), fill_with_zero(fz) {}
			bool operator!() { return flags == std::ios_base::fmtflags(~0U); }
            operator bool()  { return flags != std::ios_base::fmtflags(~0U); }
        };

		format fmt;       

        format advance();
		static format specifier_to_flags(const charT *b, const charT *e);
        size_type find_format_specifier(const charT *&b, const charT *&e);
		static ios_base::fmtflags  flag_for_symbol(charT symbol);
		static const char* find_interleaved(const charT *b, const charT *e);  	
    };

    typedef basic_formatter<char> formatter;

    #ifndef NO_PCRE
	class smart_regexp
	{
	protected:

		struct cr_ptr : public ref_counted
		{
			pcre *cr;
			cr_ptr(pcre *cr) : cr(cr) {}
			~cr_ptr() { if (cr) pcre_free(cr); }
			operator pcre*() { return cr; }
		};

		ptr_to_ref_counted<cr_ptr> compiled_regexp;	

		pair<int, int>* _matches;
        int matches_count;
        int current_match;

		string last_try;

        pcre* init(const char*, int flags);

	public:
		smart_regexp(const char* regexp, int flags = PCRE_EXTENDED);
        smart_regexp(const char* regexp, const string& s, 
					 int flags = PCRE_EXTENDED);

        bool operator()(const string &);
		string operator[](size_t i);
		const pair<int, int>* matches() { return _matches; }

		string replace(const string& replacement);

        void rewind();
        template<class T>
		smart_regexp& operator>>(T& t);
		~smart_regexp();

		friend class regexp_replacer;
	};

	template<>
	smart_regexp& smart_regexp::operator>>(string& s);

	class regexp_replacer
	{
	protected:
		smart_regexp regexp;
		string replacement;
	public:
		regexp_replacer(const char* regexp, const string& replacement);
		//	: smart_regexp(regexp), replacement(replacement) {}
	};
	
	#endif

	#include "text.cc"
}

namespace text
{
	using text_internal::trim;
	using text_internal::trim_ws;
	using text_internal::chop;
	using text_internal::quote;
	using text_internal::unquote;
	using text_internal::escape;
	using text_internal::unescape;
	using text_internal::split;
	using text_internal::columnizer;
	using text_internal::itos;
	using text_internal::atoi;
	using text_internal::format;
	using text_internal::basic_formatter;
	using text_internal::formatter;
	#ifndef NO_PCRE
	using text_internal::smart_regexp;
	using text_internal::regexp_replacer;
	#endif
}

inline
text::formatter operator+(std::ostream& os, const std::string& format)
{
	return text::formatter(os, format);
}


#endif
