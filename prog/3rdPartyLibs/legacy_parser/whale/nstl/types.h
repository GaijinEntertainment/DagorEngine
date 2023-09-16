/**	Gives certain type info 

	@file
*/


#ifndef _TYPES_H_
#define _TYPES_H_

struct Void {};
struct Default {};
struct True {};
struct False {};


template<class T>
struct nonref 			{ typedef T type; };

template<class T>
struct nonref<T &>		{ typedef T type; };

template<class T>
struct const_ref				{ typedef const T& type; };

template<class T>
struct const_ref<T &>			{ typedef const T& type; };

template<class T>
struct ref						{ typedef T& type; };

template<class T>
struct ref<T &>					{ typedef T& type; };




template<class T>
struct is_buildin {
	static const bool val = false;
};

#define mark_as_build_in_type(T)	template<>\
									struct is_buildin<T> {\
										static const bool val = true;\
									};

mark_as_build_in_type(signed char)
mark_as_build_in_type(short int)
mark_as_build_in_type(int)
mark_as_build_in_type(long int)
mark_as_build_in_type(unsigned char)
mark_as_build_in_type(unsigned short int)
mark_as_build_in_type(unsigned int)
mark_as_build_in_type(unsigned long int)
mark_as_build_in_type(float)
mark_as_build_in_type(double)
mark_as_build_in_type(long double)


#endif
