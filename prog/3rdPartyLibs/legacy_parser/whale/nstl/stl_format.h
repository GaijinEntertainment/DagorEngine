
/**	Output facilities for standard containers.

	Standard containers can be outputted in many different formats. Variation
	can be more or less classified as follows:
		- opening and closing braces
		- delimiter between elements
		- delimiter before the last element
		- symbol between key and value for map
	In addition to above parameters, which are unlikely to change in a program,
	we would like to sometimes print container elements one at line, e.g. for
	outputting vector of vectors.
		
	
	There are basically two ways to attach data to iostream: locale or parray. 
	Locale has these (dis)advantages:
		+	it is natural and standard way to implement such variations
		+	we don't have to bother with lifetime of our object
		-	changing locale is probably not very fast
	parray has these issues
		+	it is lightweight
		-	it is dumb - we should either rely on every data been static,
			or use reference counting
		-	there's no way equvivalent to locale::global()

	Taking above points into consideration, it is decided that 
		-	all data is stored inside some locale facet
		-	flag that tells about priting elements one at a line is stored
			in parray
		-	for non-stanard compliant libraries, data is stored in parray.
		-	it should be noted that while standard facets' interface 
			has mostly virtual methods, it is not possible in this case.


	TODO :
		braces manipulators
		easy facet creation
		better multiset/multimap formatting		
		width handling
		use char_traits everywhere 
		allow easy definition of output operator for user classes
		move inserter of pair somewhere

	@version 0.32
*/

#ifndef _STL_FORMAT_
#define _STL_FORMAT_

#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>

#include <bases.h>


/** Class that performs actual output of containers. Contains all data needed
	for formatting except for 'multiline' flag, which is passed from outside,
	if needed.
*/
template<class charT>
class stl_formatter {
public:	

	typedef std::basic_ostream<charT> os_type;

   	stl_formatter();

	template<class T1, class T2>
		void
		put(os_type& os, const std::pair<T1, T2>& p, bool m = false) const;
	template<class T, class A>
		void
		put(os_type& os, const std::vector<T, A>& v, bool m = false) const;
	template<class T, class A>
		void
		put(os_type& os, const std::deque<T, A>& d, bool m = false) const;
	template<class T, class A>
		void
		put(os_type& os, const std::list<T, A>& l, bool m = false) const;
	template<class T, class C, class A>
		void
		put(os_type& os, const std::set<T, C, A>& s, bool m = false) const;
	template<class T, class C, class A>
		void
		put(os_type& os, const std::multiset<T, C, A>& s, bool m = false) const;
	template<class K, class T, class C, class A>
		void
		put(os_type& os, const std::map<K, T, C, A>& c, bool m = false) const;
	template<class K, class T, class C, class A>
		void
		put(os_type& os, const std::multimap<K, T, C, A>& c, bool m = false) const;

protected:

	typedef std::pair< std::basic_string<charT>, std::basic_string<charT> > sp;

	sp delimiters;
	sp vector_braces, deque_braces, list_braces, set_braces, map_braces;
	std::basic_string<charT> map_arrow;

public:
};

extern const int multiline_flag_index;


#ifndef NO_CPP_LOCALE
/** Locale facet which merely provides storage for stl_formatter.
*/
template<class charT>
class stl_format : public std::locale::facet, public stl_formatter<charT> {
public:	
	static std::locale::id id;
	stl_format(size_t refs = 1) : std::locale::facet(refs) {}
};
#else
template<class charT>
class stl_format : public stl_formatter<charT>, public ref_counted
{
public:
	stl_format(size_t refs = 1) : ref_counted(refs)
	{}

	#ifdef STL_FORMAT_TEST
	~stl_format()
	{
		std::cout << "~stl_format()\n";
	}
	#endif
};
extern const int stl_format_index;

#endif



/**	Gets instance of stl_formatter for this stream. Creates one if not found */
template<class charT>
	const stl_formatter<charT>&
	get_formatter(std::basic_ostream<charT>& os);

/**	Gets value of multiline flag from stream */
template<class charT>
	bool
	get_multiline_flag(std::basic_ostream<charT>& os);

template<class charT>
	void 
	set_multiline_flag(std::basic_ostream<charT>& os, bool b);


/* 
 * Inserters for STL containers 
 */

template<class charT, class traits, class T1, class T2>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::pair<T1, T2>& p)
	{
		get_formatter(os).put(os, p, get_multiline_flag(os));		
		return os;
	}

template<class charT, class traits, class T, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::vector<T, A>& v)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, v, flag);		

		return os;
	}

template<class charT, class traits, class T, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::deque<T, A>& d)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, d, flag);		
		return os;
	}

template<class charT, class traits, class T, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::list<T, A>& l)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, l, flag);		
		return os;
	}

template<class charT, class traits, class T, class C, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::set<T, C, A>& s)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, s, flag);		
		return os;
	}

template<class charT, class traits, class T, class C, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::multiset<T, C, A>& s)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, s, flag);		
		return os;
	}

template<class charT, class traits, class K, class T, class C, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::map<K, T, C, A>& m)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, m, flag);		
		return os;
	}

template<class charT, class traits, class K, class T, class C, class A>
	std::basic_ostream<charT, traits>&
	operator<<
	(std::basic_ostream<charT, traits>& os, const std::multimap<K, T, C, A>& m)
	{
		bool flag = get_multiline_flag(os);
		set_multiline_flag(os, false);
		get_formatter(os).put(os, m,flag);		
		return os;
	}


/*	Manipulators */

template<class charT, class traits>
	std::basic_ostream<charT, traits>&
	multiline(std::basic_ostream<charT, traits>& os);

template<class charT>
	struct stream_inserter
	{
		template<class T>
		void operator()(std::basic_ostream<charT>& os, const T& val)
		{
			os << val;
		}
	};


/**	Routine which performs actual output. */
template<class charT, class char_traits, class InputIterator, class Inserter>
	void
	print_with_delimiters(std::basic_ostream<charT, char_traits> &os,
						  InputIterator first, InputIterator last,
						  const std::string& delim,
						  const std::string& last_delim,
						  Inserter inserter);

/** @overload */
template<class charT, class char_traits, class InputIterator>
	void
	print_with_delimiters(std::basic_ostream<charT, char_traits>& os,
						  InputIterator first, InputIterator last,
					  	  const std::string& delim,
					  	  const std::string& last_delim);

/** @overload */
template<class charT, class traits, class Container, class Inserter>
	void
	print_with_delimiters(std::basic_ostream<charT, traits> &os,
						  const Container &container,
						  const std::string& delim,
						  const std::string& last_delim,
						  Inserter inserter);

/** @overload */
template<class charT, class traits, class Container>
	void
	print_with_delimiters(std::basic_ostream<charT, traits>& os,
						  const Container &container,
						  const std::string& delim,
						  const std::string& last_delim);



template<class charT, class traits>
	std::basic_ostream<charT, traits>&
	nl(std::basic_ostream<charT, traits>& os)
	{
		os.put(os.widen('\n'));
		return os;
	}

#include "stl_format.cc"

#endif
