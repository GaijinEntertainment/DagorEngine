
#ifndef __WHALE__SORTED_VECTOR_H

#define __WHALE__SORTED_VECTOR_H

#include <vector>

template<class T> class sorted_vector : public std::vector<T>
{
public:
  typedef typename std::vector<T> base_type;
  typedef typename std::vector<T>::iterator iterator;
private:
	iterator find_around(const T &x)
	{
		for(int i=0; i<base_type::size(); i++)
			if(!(((base_type&)(*this))[i]<x))
				return base_type::begin()+i;
		return base_type::end();
	}
	
public:
	iterator find(const T &x)
	{
		iterator p=find_around(x);
		if(p!=base_type::end() && *p==x)
			return p;
		else
			return base_type::end();
	}
	void insert(const T &x)
	{
		iterator p=find_around(x);
		if(p==base_type::end() || !(*p==x))
			std::vector<T>::insert(p, x);
	}
	void erase(const T &x)
	{
		iterator p=find_around(x);
		if(p!=base_type::end())
			std::vector<T>::erase(p);
	}
	int count(const T &x)
	{
		int result=0;
		
		for(int i=0; i<base_type::size(); i++)
			if(((base_type&)(*this))[i]==x)
				result++;
		
		return result;
	}
};

#endif
