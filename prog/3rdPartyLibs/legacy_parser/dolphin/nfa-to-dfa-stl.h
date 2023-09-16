
// Some functions called from nfa-to-dfa.cpp.
// (C) Vladimir Prus <ghost@cs.msu.su>, 2000.

#ifndef _NFA_TO_DFA_STL_H_
#define _NFA_TO_DFA_STL_H_

#include <algorithm>
#include <numeric>

template<class Container, class ContainerType> inline ContainerType accumulate(const Container &container, ContainerType initial)
{
	return std::accumulate(container.begin(), container.end(), initial);
}

template<class Container> void sort(Container &container)
{
	std::sort(container.begin(), container.end());
}

template<class Container, class Compare> inline void sort(Container &container, Compare comp)
{
	std::sort(container.begin(), container.end(), comp);
}

template<class Container, class T> inline void erase(Container &container, const T &value)
{ 
	typename Container::iterator i=find(container, value);
	if(i!=container.end())
		container.erase(i);
}

template<class Container> inline void unique_and_erase(Container &container)
{
	typename Container::iterator i=unique(container.begin(), container.end());
	if(i!=container.end())
		container.erase(i, container.end());
}

template<class InputIterator, class Predicate> vector<int> find_all_indices_if(InputIterator b, InputIterator e, Predicate pred)
{
	vector<int> result;
	for(InputIterator i=b; i!=e; i++)
		if(pred(*i)) result.push_back(distance(b, i));
	return result;
}

template<class Container, class Predicate> inline vector<int> find_all_indices_if(Container &container, Predicate pred)
{
	return find_all_indices_if(container.begin(), container.end(), pred);
}

template<class InputIterator, class T> inline vector<int> find_all_indices(InputIterator b, InputIterator e, const T &t)
{
	return find_all_indices_if(b, e, std::bind(equal_to<T> (), std::placeholders::_1, t));
}

template<class Container, class T> inline vector<int> find_all_indices(Container &container, const T& t)
{
	return find_all_indices(container.begin(), container.end(), t);
}

template<class RandomAccessIterator> void bubble(RandomAccessIterator first, RandomAccessIterator last)
{
	bool switch_flag=true;
	while(switch_flag)
	{
		switch_flag=false;
		RandomAccessIterator aux=first;
		
		if(first!=last)
			for(--last; aux!=last; ++aux) 
				if(*aux > *(aux + 1)) 
				{
					swap(*aux, *(aux+1));
					switch_flag=true;
				}
	}
}

template<class Container> void bubble(Container &container)
{
	bubble(container.begin(), container.end());
}

template<class Container> inline void make_set(Container &container)
{
	if(container.size()>6)
		sort(container);
	else
		bubble(container);
	unique_and_erase(container);
}

#endif
