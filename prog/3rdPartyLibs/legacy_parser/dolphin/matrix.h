
#ifndef __DOLPHIN__MATRIX_H

#define __DOLPHIN__MATRIX_H

//#define DEBUG_MATRIX

#include <algorithm>
#include <typeinfo>

template <class T> class matrix
{
protected:
	int m, n;
	T *body;
	
public:
	matrix(int supplied_m, int supplied_n)
	{
		body=new T[(m=supplied_m)*(n=supplied_n)];
	#ifdef DEBUG_MATRIX
		cout << "matrix<" << typeid(T).name() << ">::matrix(): raw size is " << m*n*sizeof(T) << "\n";
	#endif
	}
	matrix(matrix<T> &prototype)
	{
		body=new T[(m=prototype.m)*(n=prototype.n)];
	#ifdef DEBUG_MATRIX
		cout << "matrix<" << typeid(T).name() << ">::matrix(matrix<" << typeid(T).name() << "> &): raw size is " << m*n*sizeof(T) << "\n";
	#endif
	}
	~matrix()
	{
		delete[] body;
	#ifdef DEBUG_MATRIX
		cout << "matrix<" << typeid(T).name() << ">::~matrix()\n";
	#endif
	}
	matrix<T> &operator =(const matrix<T> &prototype)
	{
		delete[] body;
		body=new T[(m=prototype.m)*(n=prototype.n)];
		copy(prototype.body, prototype.body+m*n, body);
	#ifdef DEBUG_MATRIX
		cout << "matrix<" << typeid(T).name() << ">::operator =(matrix<" << typeid(T).name() << "> &): new raw size is " << m*n*sizeof(T) << "\n";
	#endif
		return *this;
	}
	void destructive_resize(int new_m, int new_n)
	{
		delete[] body;
		body=new T[(m=new_m)*(n=new_n)];
	#ifdef DEBUG_MATRIX
		cout << "matrix<" << typeid(T).name() << ">::destructive_resize(" << new_m << ", " << new_n << "): new raw size is " << m*n*sizeof(T) << "\n";
	#endif
	}
	
	typedef T * iterator;
	typedef const T * const_iterator;
	
	T *operator [](int i) { return body+i*n; }
	const T *operator [](int i) const { return body+i*n; }
	T &operator [](const std::pair<int, int> &c) { return body[c.first*n+c.second]; }
	const T &operator [](const std::pair<int, int> &c) const { return body[c.first*n+c.second]; }
	std::pair<int, int> locate(const iterator &p)
	{
		int offset=p-body;
		return std::make_pair(offset/n, offset%n);
	}
	iterator begin() { return body; }
	iterator end() { return body+m*n; }
	iterator at(int i, int j) { return body+i*n+j; }
};

#undef DEBUG_MATRIX

#endif
