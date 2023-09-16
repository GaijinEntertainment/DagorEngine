
#ifndef __DOLPHIN__STL_H

#define __DOLPHIN__STL_H

#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

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

#endif
