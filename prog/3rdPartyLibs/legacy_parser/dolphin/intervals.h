
// set-theoretic operations on intervals:
// {(1, 10)} & ~{(4, 5), (7, 7), (10, 15)} = {(1, 3), (6, 6), (8, 9)}

// supports any integral type with known numeric_limits.

#ifndef __DOLPHIN__INTERVALS_H

#define __DOLPHIN__INTERVALS_H

#include <iostream>
#include <set>
#include <limits>

//#define __DOLPHIN__INTERVALS_H__DEBUG

#ifdef __DOLPHIN__INTERVALS_H__DEBUG
template<class T1, class T2> std::ostream &operator <<(std::ostream &os, const std::pair<T1, T2> &p)
{
	return os << "(" << p.first << ", " << p.second << ")";
}
#endif

template<class T> struct UnionOfIntervals
{
	std::set<std::pair<T, T> > intervals;
	
	UnionOfIntervals()
	{
	}
	UnionOfIntervals(T i)
	{
		insert(i, i);
	}
	UnionOfIntervals(T i, T j)
	{
		insert(i, j);
	}
	operator std::set<T>()
	{
		std::set<T> result;
		
		for(typename std::set<std::pair<T, T> >::const_iterator p=intervals.begin(); p!=intervals.end(); p++)

			for(T i=p->first; i<=p->second; i++)
				result.insert(result.end(), i);
		
		return result;
	}
	void insert(T i, T j)
	{
		intervals.insert(std::make_pair(i, j));
	}
	void insert(std::pair<T, T> p)
	{
		intervals.insert(p);
	}
	void add_to_end(T i, T j)
	{
		intervals.insert(intervals.end(), std::make_pair(i, j));
	}
	void add_to_end(std::pair<T, T> p)
	{
		intervals.insert(intervals.end(), p);
	}
	bool contains(T x)
	{
		// optimize me!
		
		for(typename std::set<std::pair<T, T> >::const_iterator p=intervals.begin(); p!=intervals.end(); p++)
			if(p->first<=x && x<=p->second)
				return true;
		
		return false;
	}
	int count(T x)
	{
		return contains(x) ? 1 : 0;
	}
	bool empty() const
	{
		return intervals.size()==0;
	}
	bool check_consistency() const
	{
		// It there are no errors in this file, this function should
		// return true under any circumstances.
		
		if(intervals.size()==0) return true;
		
		typename std::set<std::pair<T, T> >::const_iterator p=intervals.begin();
		if(p->first > p->second) return false;
		
		T x=p->second;
		
		while(++p != intervals.end())
		{
			if(p->first <= x) return false;
			if(p->first > p->second) return false;
			x=p->second;
		}
		
		return true;
	}
};

template<class T> std::ostream &operator <<(std::ostream &os, const UnionOfIntervals<T> &ui)
{
	os << "{";
	
	bool flag=false;
	for(typename std::set<std::pair<T, T> >::const_iterator p=ui.intervals.begin(); p!=ui.intervals.end(); p++)
	{
		if(flag)
			os << ", ";
		else
			flag=true;
		
		if(p->first==p->second)
			os << p->first;
		else
			os << "(" << p->first << ", " << p->second << ")";
	}
	
	os << "}";
	
	return os;
}

// should optimize operators ~ and | by replacing result.insert(x) by
// result.insert(write_p, x).

template<class T> UnionOfIntervals<T> operator ~(const UnionOfIntervals<T> &ui)
{
	if(ui.intervals.size()==0)
		return UnionOfIntervals<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
	
	typename std::set<std::pair<T, T> >::const_iterator p=ui.intervals.begin();
	T x=std::numeric_limits<T>::min();
	UnionOfIntervals<T> result;

	T max_value;
	while(p!=ui.intervals.end())
	{
		if(x!=p->first)
			result.add_to_end(x, T(p->first-1));

		max_value=p->second;
		x=T(p->second+1);
		p++;
	}

	if(max_value!=std::numeric_limits<T>::max())
		result.add_to_end(x, std::numeric_limits<T>::max());

	return result;
}

template<class T> UnionOfIntervals<T> operator |(const UnionOfIntervals<T> &ui1, const UnionOfIntervals<T> &ui2)
{
#ifdef __DOLPHIN__INTERVALS_H__DEBUG
	std::cout << "UnionOfIntervals operator |:\n";
#endif
	
	if(ui1.intervals.size()==0) return ui2;
	if(ui2.intervals.size()==0) return ui1;
	
	typename std::set<std::pair<T, T> >::const_iterator p1=ui1.intervals.begin(),
		p2=ui2.intervals.begin();
	T x=std::min(p1->first, p2->first);
	UnionOfIntervals<T> result;
	
	while(p1!=ui1.intervals.end() && p2!=ui2.intervals.end())
	{
	#ifdef __DOLPHIN__INTERVALS_H__DEBUG
		std::cout << "\t" << *p1 << "; " << *p2 << "; x=" << x << "\n";
	#endif
		
		// if there is a gap between *p1 and *p2, then close the
		// current interval.
		
		if(p1->first<p2->first && p1->second<p2->first && p1->second+1<p2->first)
		{
		#ifdef __DOLPHIN__INTERVALS_H__DEBUG
			std::cout << "\tclose: making " << std::make_pair(x, p1->second) << ", advance p1, ";
		#endif
			result.add_to_end(x, p1->second);
			p1++;
			
			if(p1!=ui1.intervals.end())
				x=std::min(p1->first, p2->first);
			else
				x=p2->first;
			
		#ifdef __DOLPHIN__INTERVALS_H__DEBUG
			std::cout << "x=" << x << "\n";
		#endif
		}
		else if(p2->first<p1->first && p2->second<p1->first && p2->second+1<p1->first)
		{
		#ifdef __DOLPHIN__INTERVALS_H__DEBUG
			std::cout << "\tclose: making " << std::make_pair(x, p2->second) << ", advance p2, ";
		#endif
			result.add_to_end(x, p2->second);
			p2++;
			
			if(p2!=ui2.intervals.end())
				x=std::min(p1->first, p2->first);
			else
				x=p1->first;
			
		#ifdef __DOLPHIN__INTERVALS_H__DEBUG
			std::cout << "x=" << x << "\n";
		#endif
		}
		else
		{
			// if it isn't the case, then advance the interval
			// which has smaller higher (_not_ lower) bound.
			
			if(p1->second<p2->second)
			{
			#ifdef __DOLPHIN__INTERVALS_H__DEBUG
				std::cout << "\tadvance p1\n";
			#endif
				p1++;
			}
			else
			{
			#ifdef __DOLPHIN__INTERVALS_H__DEBUG
				std::cout << "\tadvance p2\n";
			#endif
				p2++;
			}
		}
	}
	
	// if there is an interval left, then close it.
	if(p1!=ui1.intervals.end())
	{
	#ifdef __DOLPHIN__INTERVALS_H__DEBUG
		std::cout << "\tfinalize: making " << std::make_pair(x, p1->second) << "\n";
	#endif
		result.add_to_end(x, p1->second);
		p1++;
		
		// if there are more intervals left, just copy them.
		while(p1!=ui1.intervals.end())
			result.add_to_end(*p1++);
	}
	else if(p2!=ui2.intervals.end())
	{
	#ifdef __DOLPHIN__INTERVALS_H__DEBUG
		std::cout << "\tfinalize: making " << std::make_pair(x, p2->second) << "\n";
	#endif
		result.add_to_end(x, p2->second);
		p2++;
		
		while(p2!=ui2.intervals.end())
			result.add_to_end(*p2++);
	}
	
	return result;
}

template<class T> UnionOfIntervals<T> operator &(const UnionOfIntervals<T> &ui1, const UnionOfIntervals<T> &ui2)
{
	// first check some simple sufficient conditions of empty intersection:
	if(ui1.empty() || ui2.empty())
		return UnionOfIntervals<T>();
//	if(ui1.intervals.back().second < ui2.intervals.front().first)
//		return UnionOfIntervals<T>();
//	if(ui2.intervals.back().second < ui1.intervals.front().first)
//		return UnionOfIntervals<T>();
	
	return ~((~ui1) | (~ui2));
}

template<class T> bool operator ==(const UnionOfIntervals<T> &ui1, const UnionOfIntervals<T> &ui2)
{
	return ui1.intervals == ui2.intervals;
}

template<class T> bool operator <(const UnionOfIntervals<T> &ui1, const UnionOfIntervals<T> &ui2)
{
	return ui1.intervals < ui2.intervals;
}

#endif
