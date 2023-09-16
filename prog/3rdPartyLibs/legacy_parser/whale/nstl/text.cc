
/*	advance method for class formatter: finds the next format specifier,
	set format flags as specified.
    format_specifier_pos point one symbol past the specifier found;
    if it wasn't found, the variable is unmodified

    This is internal file that gets included from text.h

    In version 0.2 destructor has no side effects (in used to flush pending
    characters). It now possible to copy formatters freely, but only
    one can be used for actual output.

    Rev 0.2
 */

#include <ctype.h>

template<class charT, class traits>
std::ios_base::fmtflags 
basic_formatter<charT, traits>::flag_for_symbol(charT symbol)
{
	switch(symbol) {
    case 's':
    	return std::ios_base::fmtflags(0U);
	case 'd':
	case 'i':
	case 'u':
		return std::ios_base::dec;
	case 'o':
		return std::ios_base::oct;
	case 'X':
		return std::ios_base::uppercase | std::ios_base::hex;
	case 'x':
		return std::ios_base::hex;
	case 'f':
		return std::ios_base::fixed;
	case 'E':
	case 'G':
		return std::ios_base::uppercase | std::ios_base::scientific;
	case 'e':
	case 'g':
		return std::ios_base::scientific;
	case 'p':
		return std::ios_base::uppercase | std::ios_base::hex;
	case '+':
		return std::ios_base::showpos;
	case '-':
		return std::ios_base::left;
	case '#':
		return std::ios_base::showbase | std::ios_base::showpoint;
	default:
		throw logic_error(text_internal::format("Invalid format symbol %c", symbol));
    }
}

/*	Returns format for specifier given by `b' - begin pointer and
	'e' - past-end pointer. Specifier includes '%'
*/	
template<class charT, class traits>
typename basic_formatter<charT, traits>::format
basic_formatter<charT, traits>::specifier_to_flags
(const charT *b, const charT *e)
{
    ++b;
    --e;
	assert(e >= b);

   	std::ios_base::fmtflags f(flag_for_symbol(*e));
	
	for (; b != e && !isdigit(*b); ++b) f |= flag_for_symbol(*b);

	bool fill_with_zero = (*b == '0');
    int width = 0, prec = 0;
    for (; b != e && isdigit(*b); ++b) width = width * 10 + (*b - '0');

    if (*b == '.')  {
		++b;
		for (; b != e && isdigit(*b); ++b) prec = prec *10 + (*b - '0');
	}

	assert(b == e);

	return format(f, width, prec, fill_with_zero);	
}

/* 	Finds format specifier starting from 'format_specifier_pos'
	Sets 'b' and 'e' to begin and past-end pointers.
    'b' will always point to '%'
    for nested specifiers, '}' is included
*/

template<class charT, class traits>
typename basic_formatter<charT, traits>::size_type
basic_formatter<charT, traits>::find_format_specifier
(const charT *&b, const charT *&e)
{
	typename basic_string<charT, traits>::size_type pos = format_specifier_pos, end;
    while( pos = format_str.find('%', pos), pos != -1) {
        if (format_str[pos + 1] != '%') break;
		else ++pos;		
    }
	if (pos == -1) return pos;

	b = format_str.data() + pos;
	if (*(b+1) == '{') {
    	end = pos;
		while( end = format_str.find('}', end), end != -1) {
			if (end == 0 || format_str[end - 1] != '\\') break;
			else ++end;
		}
		if (end == -1) throw logic_error("EOS in format specifier");
	} else {
		char stop_symbols[] = "diouxXfegEGcs";
		end = format_str.find_first_of(stop_symbols, pos);
		if (end == -1) throw logic_error("Invalid format specifier");
	}
	e = format_str.data() + end + 1;
	return pos;
}

template<class charT, class traits>
const char* 
basic_formatter<charT, traits>::find_interleaved(const charT *b, const charT *e)
{
	if (*b != '/') return b;
	if (*(b+1) != '{') throw logic_error("{ expected");
	for (const char* p = b + 2; p < e; ++p) {
		if (*p == '}') return p + 1;
	}
	throw logic_error("EOS found while looking for end of interleaving specifier");
}

template<class charT, class traits>
typename basic_formatter<charT, traits>::format
basic_formatter<charT, traits>::advance()
{
	format f;

	/* Do until nested formatter can advance or we can find simple specifier */
	
	while(true) { 
		if (nested) {
			f = nested->advance();
			if (f) return f;
			delete nested;
            nested = NULL;
		}

        size_type pos = format_specifier_pos;

		const charT *b, *e;
		while( remaining_repeats > 0 &&
        	   (pos = find_format_specifier(b, e), pos == -1))
		{
        	--remaining_repeats;
            os << format_str.substr(format_specifier_pos);
            if (remaining_repeats > 0) os << interleaver;
            format_specifier_pos = 0;
        }

	
        if (remaining_repeats <= 0) {
        	format_specifier_pos = format_str.size();
        	return format();
		}

       	os << format_str.substr(format_specifier_pos, pos - format_specifier_pos);
		format_specifier_pos = pos + (e - b);

		if (*(b+1) != '{') 	return specifier_to_flags(b, e);
		else {
			if (nested_counters.empty()) throw logic_error("No counter");

			const char* se = format_str.data() + format_str.length();
			const char* ie = find_interleaved(e, se);
            
            format_specifier_pos += (ie - e);
            string interleaver = (ie >= e + 3) ? string(e + 2, ie-1) : string("");

            nested = new basic_formatter(os, string(b+2, e-1), interleaver);
			nested->remaining_repeats = nested_counters.front();
			nested_counters.pop();
		}
	}
}

    	
template<class charT, class traits>
basic_formatter<charT, traits>::
basic_formatter(const string &format_str) :
	format_str(format_str), format_specifier_pos(0),
	os(ssp), nested(NULL), remaining_repeats(1), first_time(true)
	{
		//fmt = advance();
	}

template<class charT, class traits>
basic_formatter<charT, traits>::
basic_formatter(basic_ostream<charT, traits> &os, const string &format_str,
				const string& interleaver) :
	format_str(format_str), format_specifier_pos(0), interleaver(interleaver),
	os(os), nested(NULL), remaining_repeats(1), first_time(true)
	{
		//fmt = advance();
	}


template<class charT, class traits>
basic_formatter<charT, traits>::
~basic_formatter() { }


template<class charT, class traits>
basic_formatter<charT, traits>&
basic_formatter<charT, traits>::
operator<<(ios_base& (*pf)(ios_base&))
{	os << pf; return *this; }

template<class charT, class traits>
basic_formatter<charT, traits>&
basic_formatter<charT, traits>::
operator<<(ios_type& (*pf)(ios_type&))
{	os << pf; return *this; }

template<class charT, class traits>
template<class T> 
basic_formatter<charT, traits>&
basic_formatter<charT, traits>::
operator<<(const T &t)
{
	ios_base::fmtflags saved_flags = os.flags();
	charT saved_fill = os.fill();

	if (first_time) {
		fmt = advance();
		first_time = false;
	}	
	
	if (!fmt) throw logic_error("Too many values");
	
	os.flags(fmt.flags);
	os.width(fmt.width);
	os.precision(fmt.prec);
	os.fill(fmt.fill_with_zero ? '0' : ' ');
	os << t;
	
	os.fill(saved_fill);
	os.flags(saved_flags);

  	format fmt = advance();
	return *this;
}


template<class charT, class traits>       	
template<class T>
basic_formatter<charT, traits>& 
basic_formatter<charT, traits>::
operator<<(const _Smanip<T> &m)
{
	os << m; return *this;
}


template<class charT, class traits>       	
basic_formatter<charT, traits>& 
basic_formatter<charT, traits>::
counter(int n)
{ nested_counters.push(n); return *this; }

template<class charT, class traits>       	
basic_string<charT, traits> 
basic_formatter<charT, traits>::
str() const {
	return ssp.str();
}

template<class charT, class traits>
basic_formatter<charT, traits>::
operator basic_string<charT, traits>() const {
	return str();
}


// Uph, that was ugly. (Unused nowdays)
template<class charT, class traits>       	
void
basic_formatter<charT, traits>::
flush()	{
	assert(false);
	if (advance() && !uncaught_exception())	
		throw logic_error("Not all format specifiers were used");
}

template<class charT, class traits>
bool
basic_formatter<charT, traits>::test()
{
	basic_formatter<char>(cout, "%s %s %s\n") << "one" << "two" << "three";

    basic_formatter<char>(cout, "%s %{ %s %s} %s\n").counter(2)
    	<< "pre" << "11" << "12" << "21" << "22" << "post";
	basic_formatter<char>(cout, "%s %{d}\n").counter(10) << "10 dup 'd' : ";
    basic_formatter<char>(cout, "%s %{ %s }\n").counter(0) << "lonely";
    basic_formatter<char>(cout, "%{ xxx %s } %s").counter(0) << "zzzz";
};
