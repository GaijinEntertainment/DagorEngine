
/*
 *	Internal implementation file
*/

#include <utility_extensions.h>
#include <text.h>

#ifdef NO_CPP_LOCALE

inline void stl_format_callback(std::ios_base::event e, std::ios_base& io, int)
{
	void* p = io.pword(stl_format_index);
	if (p != 0) {
		
		ref_counted* rc = static_cast<ref_counted*>(p);

		if (e == std::ios_base::erase_event) {
			if (rc->ref_count() == 1) {
				io.pword(stl_format_index) = 0;
			}
			rc->release();
		} else
		if (e == std::ios_base::copyfmt_event) {
			rc->add_ref();
		}
	}
}

#endif


/**	Gets instance of stl_formatter for this stream. Creates one if not found */
template<class charT>
	const stl_formatter<charT>&
	get_formatter(std::basic_ostream<charT>& os)
	{
	#ifndef NO_CPP_LOCALE
		std::locale locale = os.getloc();
		if (!std::has_facet< stl_format<charT> >(locale)) {
			// FIXME: shouldn't we consider global locale first
			os.imbue(std::locale(locale, new stl_format<charT>()));
		}
		return std::use_facet< stl_format<charT> >(os.getloc());
	#else
		void* p = os.pword(stl_format_index);
		if (!p) {
			// We will keep pointer to subobject, so that callback know
			// type of object it deals with
			ref_counted* rc = new stl_format<charT>();
			p = os.pword(stl_format_index) = rc;
			os.register_callback(stl_format_callback, 0);
		}
		ref_counted* rc = static_cast<ref_counted*>(p);
		return dynamic_cast<stl_format<charT>&>(*rc);
	#endif
	}

template<class charT>
	bool
	get_multiline_flag(std::basic_ostream<charT>& os)
	{
		return os.iword(multiline_flag_index);
	}

template<class charT>
	void
	set_multiline_flag(std::basic_ostream<charT>& os, bool b)
	{
		os.iword(multiline_flag_index) = b;
	}

template<class charT, class traits = std::char_traits<charT> >
	struct braces_output_aux
	{
		typedef std::pair< std::basic_string<charT>, std::basic_string<charT> > sp;

		braces_output_aux(std::basic_ostream<charT, traits>& os,
						  const sp& braces, bool m)
		: os(os)
		{
			os << braces.first;
			if (m) os << "\n";
			if (m) {
				closing_brace = "\n" + text::trim_ws(braces.second);
			} else
				closing_brace = braces.second;			
		}
		~braces_output_aux()
		{
			os << closing_brace;
		}

		std::basic_string<charT, traits> closing_brace;
		std::basic_ostream<charT, traits>& os;		
	};

template<class charT>
	struct pair_output_aux
	{
		pair_output_aux(const std::basic_string<charT>& sep)
		: sep(sep)
		{}

		template<class T1, class T2>
		void
		operator()(std::basic_ostream<charT>& os, const std::pair<T1, T2>& p)
		{
			os << p.first << sep << p.second;
		}
		std::basic_string<charT> sep;
	};


template<class charT>
template<class T1, class T2>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::pair<T1, T2>& p, bool) const
	{
		os << "(" << p.first << ", " << p.second << ")";
	}

template<class charT>
template<class T, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::vector<T, A>& v, bool m) const
	{
		braces_output_aux<charT> aux(os, vector_braces, m);

		if (m) 
			print_with_delimiters(os, v, "\n", "\n");
		else
			print_with_delimiters(os, v, delimiters.first, delimiters.second);
		set_multiline_flag(os, false);
	}

template<class charT>
template<class T, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::deque<T, A>& d, bool m) const
	{
		braces_output_aux<charT> aux(os, deque_braces, m);

		if (m) 
			print_with_delimiters(os, d, "\n", "\n");
		else
			print_with_delimiters(os, d, delimiters.first, delimiters.second);

		set_multiline_flag(os, false);
	}

template<class charT>
template<class T, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::list<T, A>& l, bool m) const
	{
		braces_output_aux<charT> aux(os, list_braces, m);

		if (m) 
			print_with_delimiters(os, l, "\n", "\n");
		else
			print_with_delimiters(os, l, delimiters.first, delimiters.second);
		set_multiline_flag(os, false);
	}

template<class charT>
template<class T, class C, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::set<T, C, A>& s, bool m) const
	{
		braces_output_aux<charT> aux(os, set_braces, m);

		if (m) 
			print_with_delimiters(os, s, "\n", "\n");
		else
			print_with_delimiters(os, s, delimiters.first, delimiters.second);

		set_multiline_flag(os, false);
	}

template<class charT>
template<class T, class C, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::multiset<T, C, A>& s, bool m) const
	{
		braces_output_aux<charT> aux(os, set_braces, m);

		if (m) 
			print_with_delimiters(os, s, "\n", "\n");
		else
			print_with_delimiters(os, s, delimiters.first, delimiters.second);
	
		set_multiline_flag(os, false);
	}

template<class charT>
template<class K, class T, class C, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::map<K, T, C, A>& c, bool m) const
	{
		braces_output_aux<charT> aux(os, map_braces, m);
		pair_output_aux<charT> aux2(map_arrow);

		if (m) {
			print_with_delimiters(os, c, "\n", "\n", aux2);
		}
		else
			print_with_delimiters(os, c, delimiters.first, delimiters.second,
								  aux2);
	}

template<class charT>
template<class K, class T, class C, class A>
	void
	stl_formatter<charT>::
	put(os_type& os, const std::multimap<K, T, C, A>& c, bool m) const
	{
		braces_output_aux<charT> aux(os, map_braces, m);
		pair_output_aux<charT> aux2(map_arrow);

		if (m) 
			print_with_delimiters(os, c, "\n", "\n", aux2);
		else
			print_with_delimiters(os, c, delimiters.first, delimiters.second,
								  aux2);

		set_multiline_flag(os, false);
	}




template<class charT, class traits>
	std::basic_ostream<charT, traits>&
	multiline(std::basic_ostream<charT, traits>& os)
	{
		set_multiline_flag(os, true);		
		return os;
	}



/** The function that performs real output */
template<class charT, class char_traits, class InputIterator, class Inserter>
	void
	print_with_delimiters(std::basic_ostream<charT, char_traits> &os,
						  InputIterator first, InputIterator last,
						  const std::string& delim,
						  const std::string& last_delim,
						  Inserter inserter)
	{	
		while(first != last) {
			inserter(os, *first++);
			if (first != last) 
				if (distance(first, last) == 1)		os << last_delim;
				else								os << delim;
		}
	}

template<class charT, class char_traits, class InputIterator>
	void
	print_with_delimiters(std::basic_ostream<charT, char_traits>& os,
						  InputIterator first, InputIterator last,
					  	  const std::string& delim,
					  	  const std::string& last_delim)
	{
		print_with_delimiters(os, first, last, delim, last_delim, 
						      stream_inserter<charT>());
	}



/** Wrapper for containers */
template<class charT, class traits, class Container, class Inserter>
	void
	print_with_delimiters(std::basic_ostream<charT, traits> &os,
						  const Container &container,
						  const std::string& delim,
						  const std::string& last_delim,
						  Inserter inserter)
	{
		print_with_delimiters(os, container.begin(), container.end(),
							  delim, last_delim, inserter);	
	}

template<class charT, class traits, class Container>
	void
	print_with_delimiters(std::basic_ostream<charT, traits>& os,
						  const Container &container,
						  const std::string& delim,
						  const std::string& last_delim)
	{
		print_with_delimiters(os, container.begin(), container.end(),
							 delim, last_delim);
	}



template<class charT>
	stl_formatter<charT>::
   	stl_formatter()
   	: delimiters(m2(", ", ", ")),
   		vector_braces(m2("[ ", " ]")),
		deque_braces(m2("[ ", " ]")),
   		list_braces(m2("[ ", " ]")),
   		set_braces(m2("{ ", " }")),
		map_braces(m2("{ ", " }")),
		map_arrow("==>")
	{}
	