
#ifndef __WHALE__UTILITIES_H

#define __WHALE__UTILITIES_H

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string.h>
#include <string>

// 1 -> i, 2 -> ii, ..., 9 -> ix, ...
std::string roman_itoa(int n, bool use_capital_letters);

std::string english_itoa(int n, int mode, bool use_capital_letters);

// 123 -> One hundred twenty three
inline std::string english1_itoa(int n, bool use_capital_letters)
{
	return english_itoa(n, 1, use_capital_letters);
}

// 111 -> One hundred twenty third
inline std::string english2_itoa(int n, bool use_capital_letters)
{
	return english_itoa(n, 2, use_capital_letters);
}

// 1 -> a, ... 26 -> z, 27 -> aa, 28 -> ab, ...
// Note: it is neither 26- nor 27-base.
std::string letter_itoa(int n, bool use_capital_letters);

std::string strip_dot(const std::string &s);

// style=1: "IDENTIFIER", style=2: "Identifier".
std::string convert_file_name_to_identifier(const std::string &s, int style);

const char *printable_increment(int x); // -1 -> "-1", 0 -> "", 1 -> "+1".

std::string prepare_for_printf(const std::string &s);
bool decode_escape_sequences(char *s, std::vector<int> &v);
inline bool is_octal_digit(char c) { return c>='0' && c<='7'; }

template<class InputIterator, class Printer>
std::ostream &custom_print(std::ostream &os, InputIterator first, InputIterator last,
	Printer printer, const char *word1=", ", const char *word2=" and ")
{
	if(first==last) return os;
	
	bool flag=false;
	for(InputIterator prev=first, next=++first; prev!=last; prev++, (next==last ? next : next++))
	{
		if(flag)
			os << (next==last ? word2 : word1);
		else
			flag=true;

		printer(os, *prev);
	}
	return os;
}

template <class Key, class Compare, class Allocator, class InputIterator>
	bool union_assign(std::set<Key, Compare, Allocator> &dest,
	InputIterator src_first, InputIterator src_last)
{
	bool flag=false;
	typename std::set<Key, Compare, Allocator>::iterator p1;
	InputIterator p2;
	for(p1=dest.begin(), p2=src_first; p2!=src_last; p2++)
	{
		while(p1!=dest.end() && *p1<*p2) p1++;
		if(p1==dest.end() || *p2<*p1)
		{
			dest.insert(*p2);
			flag=true;
		}
	}
	return flag;
}

template <class Key, class Compare, class Allocator>
	inline bool union_assign(std::set<Key, Compare, Allocator> &dest,
	const std::set<Key, Compare, Allocator> &src)
{
	return union_assign(dest, src.begin(), src.end());
}

template <class Key, class Compare, class Allocator>
	bool intersection_assign(std::set<Key, Compare, Allocator> &dest,
	const std::set<Key, Compare, Allocator> &src)
{
	std::set<Key, Compare, Allocator> result;
	std::set_intersection(dest.begin(), dest.end(), src.begin(), src.end(), std::inserter(result, result.begin()));
	bool flag=(dest==result);
	dest.swap(result);
	return flag;
}

template<class InputIterator>
std::ostream &print(std::ostream &os, InputIterator first, InputIterator last)
{
	bool flag=false;
	for(InputIterator p=first; p!=last; p++)
	{
		if(flag)
			os << ", ";
		else
			flag=true;
		
		os << *p;
	}
	return os;
}

template<class InputIterator>
std::ostream &print_with_and(std::ostream &os, InputIterator first, InputIterator last)
{
	if(first==last) return os;
	
	bool flag=false;
	for(InputIterator prev=first, next=++first; prev!=last; prev++, (next==last ? next : next++))
	{
		if(flag)
			os << (next==last ? " and " : ", ");
		else
			flag=true;
		
		os << *prev;
	}
	return os;
}

template<class T1, class T2> std::ostream &operator <<(std::ostream &os, const std::pair<T1, T2> &p)
{
	return os << "(" << p.first << ", " << p.second << ")";
}

template<class T1, class T2> inline std::ostream &operator <<(std::ostream &os, const std::vector<T1, T2> &v)
{
	os << "{";
	print(os, v.begin(), v.end());
	os << "}";
	return os;
}

template<class T1, class T2, class T3> inline std::ostream &operator <<(std::ostream &os, const std::set<T1, T2, T3> &s)
{
	os << "{";
	print(os, s.begin(), s.end());
	os << "}";
	return os;
}

template<class T1, class T2, class T3, class T4> inline std::ostream &operator <<(std::ostream &os, const std::map<T1, T2, T3, T4> &m)
{
	os << "{";
	print(os, m.begin(), m.end());
	os << "}";
	return os;
}

#include <algorithm>
#include <numeric>

template<class Container, class OutputIterator>
inline OutputIterator
copy(const Container& container, OutputIterator output)
{
	return std::copy(container.begin(), container.end(), output);
}

#endif
