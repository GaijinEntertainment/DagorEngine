/**
	Defines a number of functional objects

	@revision 0.16

	@file
*/

/* Probably the greatests advantage of Lisp is the LAMBDA statement */

#ifndef __FUNCTIONAL_EXTENSIONS_H__
#define __FUNCTIONAL_EXTENSIONS_H__


#include <functional>
#include <cstddef>
#include <map>

#include <ostream>
#include <string>

using std::pair;
using std::unary_function;
using std::binary_function;

#include "types.h"

/** @name Subscripting functors */
//@{

template<class Container, class ST = std::size_t,
		 class RT = typename Container::const_reference>
	struct const_subscript_t : std::unary_function<ST, RT>
	{
		const_subscript_t(const Container& c) : container(&c) {};
		RT operator()(const ST& i) const 
		{ return (*container)[i]; }

	protected:
		const Container* container;
	};

template<class Container, class ST = std::size_t, 
		 class RT = typename Container::reference>
	struct subscript_t : std::unary_function<ST, RT>
	{
		subscript_t(Container& c) : container(&c) {};
	    RT operator()(const ST& i) const
		{  	return (*container)[i]; }

	protected:
		Container *container;
	};

/**
	Creates subscript functor
	LALALLALA
*/
template<class Container>
	const_subscript_t<Container>
	subscript(const Container& c)
	{ return const_subscript_t<Container>(c); }

/** @overload
*/
template<class Container>
	subscript_t<Container>
	subscript(Container &c)
	{ return subscript_t<Container>(c); }

/** @overload
*/
template<class K, class T, class C, class A>
	const_subscript_t<std::map<K, T, C, A>, K, T>
	subscript(const std::map<K, T, C, A> &c)
	{ return const_subscript_t<std::map<K, T, C, A>, K, T>(c); }

/** @overload
*/
template<class K, class T, class C, class A>
	subscript_t<std::map<K, T, C, A>, K, T>
	subscript(std::map<K, T, C, A> &c)
	{ return subscript_t<std::map<K, T, C, A>, K, T>(c); }
//@}

/// @name Closures
//@{

template<class S, class T>
	struct closure0_t  
	{
		explicit closure0_t(T *it, S (T::*mp)()) : it(it), mp(mp) {}
		S operator()() const { return (it->*mp)(); }

	private:
		T *it;
		S (T::*mp)();
	};

template<class S, class T>
	struct closure0_t_const  
	{
		explicit closure0_t_const(T *it, S (T::*mp)() const) : it(it), mp(mp) {}
		S operator()() const { return (it->*mp)(); }

	private:
		T *it;
		S (T::*mp)() const;
	};


/** Creates closure that can be used to call nullary method */
template<class S, class T>
	closure0_t<S, T>
	member(T *it, S (T::*mp)())
	{ return closure0_t<S, T>(it, mp); }

/** @overload
*/
template<class S, class T>
	closure0_t<S, T>
	member(T& it, S (T::*mp)())
	{ return closure0_t<S, T>(&it, mp); }

/** @overload
*/
template<class S, class T>
	closure0_t_const<S, T>
	member(T *it, S (T::*mp)() const)
	{ return closure0_t_const<S, T>(it, mp); }

/** @overload
*/
template<class S, class T>
	closure0_t_const<S, T>
	member(T& it, S (T::*mp)() const)
	{ return closure0_t_const<S, T>(&it, mp); }

template<class S, class T, class A>
	struct closure1_t
	: std::unary_function<nonref<A>::type, S>
	{
		explicit closure1_t(T *xit, S (T::*xmp)(A))
		: it(xit) , mp(xmp) {}
	    S operator()(const_ref<A>::type a) const { return (it->*mp)(a); }

	private:
		T *it;
		S (T::* mp)(A);	
	};


template<class T, class A>
	struct closure1_t<void, T, A>
	: std::unary_function<nonref<A>::type, void>
	{
		explicit closure1_t(T *xit, void (T::*xmp)(A))
		: it(xit) , mp(xmp) {};
		void operator()(const_ref<A>::type a) const { (it->*mp)(a); }

	private:
		T *it;
		void (T::* mp)(A);	
	};

template<class S, class T, class A>
	struct closure1_t_const
	: std::unary_function<nonref<A>::type, S>
	{
		explicit closure1_t_const(T *xit, S (T::*xmp)(A) const)
		: it(xit) , mp(xmp) {}
	    S operator()(const_ref<A>::type a) const { return (it->*mp)(a); }

	private:
		T *it;
		S (T::* mp)(A) const;	
	};

template<class T, class A>
	struct closure1_t_const<void, T, A>
	: std::unary_function<nonref<A>::type, void>
	{
		explicit closure1_t_const(T *xit, void (T::*xmp)(A) const)
		: it(xit) , mp(xmp) {};
		void operator()(const_ref<A>::type a) const { (it->*mp)(a); }

	private:
		T *it;
		void (T::* mp)(A) const;	
	};

/** Creates closure that can be used to call unary method */
template<class A, class S, class T>
	closure1_t<S, T, A> 
	member(T *it, S (T::*mp)(A))
	{ return closure1_t<S, T, A>(it, mp); }

/** @overload
*/
template<class A, class S, class T>
	closure1_t<S, T, A>
	member(T &it, S (T::*mp)(A))
	{ return closure1_t<S, T, A>(&it, mp); }

/** @overload
*/
template<class A, class S, class T>
	closure1_t_const<S, T, A> 
	member(T *it, S (T::*mp)(A) const)
	{ return closure1_t_const<S, T, A>(it, mp); }

/** @overload
*/
template<class A, class S, class T>
	closure1_t_const<S, T, A>
	member(T &it, S (T::*mp)(A) const)
	{ return closure1_t_const<S, T, A>(&it, mp); }


template<class S, class T, class A1, class A2>
	struct closure2_t
	: std::binary_function<nonref<A1>::type, nonref<A2>::type, S>
	{
		explicit closure2_t(T *xit, S (T::*xmp)(A1, A2))
		: it(xit), mp(xmp) {};
	    S operator()(const_ref<A1>::type a1, const_ref<A2>::type a2) const
    	{ return (it->*mp)(a1, a2); }

	private:
		T *it;
	    S (T::* mp)(A1, A2);
	};

template<class T, class A1, class  A2>
	struct closure2_t<void, T, A1, A2>
	: std::binary_function<nonref<A1>::type, nonref<A2>::type, void>
	{
		explicit closure2_t(T *xit, void (T::*xmp)(A1, A2))
		: it(xit), mp(xmp) {};
	    void operator()(const_ref<A1>::type a1, const_ref<A2>::type a2) const
    	{ (it->*mp)(a1, a2); }

	private:
		T *it;
		void (T::* mp)(A1, A2);
	};

template<class S, class T, class A1, class A2>
	struct closure2_t_const
	: std::binary_function<nonref<A1>::type, nonref<A2>::type, S>
	{
		explicit closure2_t_const(T *xit, S (T::*xmp)(A1, A2) const)
		: it(xit), mp(xmp) {};
	    S operator()(const_ref<A1>::type a1, const_ref<A2>::type a2) const
    	{ return (it->*mp)(a1, a2); }

	private:
		T *it;
	    S (T::* mp)(A1, A2) const;
	};

template<class T, class A1, class  A2>
	struct closure2_t_const<void, T, A1, A2>
	: std::binary_function<nonref<A1>::type, nonref<A2>::type, void>
	{
		explicit closure2_t_const(T *xit, void (T::*xmp)(A1, A2) const)
		: it(xit), mp(xmp) {};
	    void operator()(const_ref<A1>::type a1, const_ref<A2>::type a2) const
    	{ (it->*mp)(a1, a2); }

	private:
		T *it;
		void (T::* mp)(A1, A2) const;
	};


/** Creates closure that can be used to call binary method */
template<class A1, class A2, class S, class T>
	closure2_t<S, T, A1, A2>
	member(T *it, S (T::*mp)(A1, A2))
	{ return closure2_t<S, T, A1, A2>(it, mp); }

/** @overload
*/
template<class A1, class A2, class S, class T>
	closure2_t<S, T, A1, A2>
	member(T &it, S (T::*mp)(A1, A2))
	{ return closure2_t<S, T, A1, A2>(&it, mp); }

/** @overload
*/
template<class A1, class A2, class S, class T>
	closure2_t_const<S, T, A1, A2>
	member(T *it, S (T::*mp)(A1, A2) const)
	{ return closure2_t_const<S, T, A1, A2>(it, mp); }

/** @overload
*/
template<class A1, class A2, class S, class T>
	closure2_t_const<S, T, A1, A2>
	member(T &it, S (T::*mp)(A1, A2) const)
	{ return closure2_t_const<S, T, A1, A2>(&it, mp); }


//@}

/* Special ptr_fun replacement that handles reference parameters nicely */
/* TODO: void isn't handled as a special case and, in consequence,
	this code may fail under compilers that doesn't allow to return void
*/

template<class A, class R>
	struct smart_pointer_to_unary_function
	: std::unary_function<nonref<A>::type, R>
	{
		explicit smart_pointer_to_unary_function(R (*f)(A)) : f(f) {}
		R operator()(const_ref<A>::type a) const { return f(a); }

	private:
		R (*f)(A);
	};

template<class A1, class A2, class R>
	struct smart_pointer_to_binary_function
	: std::binary_function<nonref<A1>::type, nonref<A2>::type, R>
	{
		explicit smart_pointer_to_binary_function(R (*f)(A1, A2)) : f(f) {}
		R operator()(ref<A1>::type a1, ref<A2>::type a2) const
		{ return f(a1, a2); }

	private:
		R (*f)(A1, A2);
	};

template<class A, class R>
	smart_pointer_to_unary_function<A, R>
	s_ptr_fun(R (*f)(A))
	{ return smart_pointer_to_unary_function<A, R>(f); }

template<class A1, class A2, class R>
	smart_pointer_to_binary_function<A1, A2, R>
	s_ptr_fun(R (*f)(A1, A2))
	{ return smart_pointer_to_binary_function<A1, A2, R>(f); }


template<class A1, class A2, class R, class T>
	std::binder1st< smart_pointer_to_binary_function<A1, A2, R> >
	bind1st(R (*f)(A1, A2), const T& t)
	{
		return binder1st< smart_pointer_to_binary_function<A1, A2, R> >
				(s_ptr_fun(f), t);
	}

template<class A1, class A2, class R, class T>
	std::binder2nd< smart_pointer_to_binary_function<A1, A2, R> >
	bind2nd(R (*f)(A1, A2), const T& t)
	{
		return binder2nd< smart_pointer_to_binary_function<A1, A2, R> >
				(s_ptr_fun(f), t);
	}


#ifndef HAVE_IDENTITY
template<class T>
	struct identity : public std::unary_function<T, T>
	{
		T operator()(const T &t) const { return t; }	
	};
#endif

/* Extremely usefull predicate */

template<class T>
	struct truth : public std::unary_function<T, bool>
	{
		bool operator()(const T &) const { return true; }
	};


/** @name Usefull pair addons */
//@{

template<class T1, class T2>
	struct pair_maker : std::binary_function< T1, T2, pair<T1, T2> >
	{
		pair<T1, T2> operator() ( const T1 &a, const T2 &b) const
	    { return make_pair(a, b); }
	};

template<class T1, class T2>
	struct take1st : std::unary_function<pair<T1, T2>, T1>
	{
		T1 operator()(const pair<T1, T2> &p) const { return p.first;   }
		T1 operator()(const pair<T1, T2> *p) const {  return p->first; }
	};

template<class T1, class T2>
	struct take2nd : std::unary_function<pair<T1, T2>, T2>
	{
		T2 operator()(const pair<T1, T2> &p) const { return p.second;  }
		T2 operator()(const pair<T1, T2> *p) const { return p->second; }
	};

template<class T1, class T2>
	struct first_equal : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator()(const pair<T1, T2> &a, const pair<T1, T2>& b) const
		{ return a.first == b.first; }
	};

template<class T1, class T2>
	struct first_less : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator () (const pair<T1, T2> &a, const pair<T1, T2> &b) const
	    { return a.first < b.first; }
	};

template<class T1, class T2>
	struct first_greater : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator () (const pair<T1, T2> &a, const pair<T1, T2> &b) const
    	{ return a.first > b.first; }
	};

template<class T1, class T2>
	struct second_equal : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator()(const pair<T1, T2> &a, const pair<T1, T2>& b) const
		{ return a.second == b.second; }
	};

template<class T1, class T2>
	struct second_less : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator () (const pair<T1, T2> &a, const pair<T1, T2> &b) const
	    { return a.second < b.second; }
	};

template<class T1, class T2>
	struct second_greater : binary_function< pair<T1, T2>, pair<T1, T2>, bool >
	{
		bool operator () (const pair<T1, T2> &a, const pair<T1, T2> &b) const
    	{ return a.second > b.second; }
	};
//@}


/** @name Bit-wise operations */

//@{
template<class T>
	struct binary_and : binary_function<T, T, T>
	{
		T operator () (const T & a, const T & b) const
		{ return T ( a & b ); }
	};

template<class T>
	struct binary_or : binary_function<T, T, T>
	{
		T operator()(const T& a, const T &b) const
    	{ return T (a | b ); }
	};

template<class T>
	struct binary_xor : binary_function<T, T, T>
	{
		T operator()(const T &a, const T &b) const
	    { return T (a ^ b); }
	};

template<class T>
	struct binary_not : unary_function<T, T>
	{
		T operator()(const T &a) const
	    { return T (~a); }
	};
//@}



template<class T>
	struct pointer_outputter
	{
		std::ostream* os;
	    std::string sep;	
    	pointer_outputter(std::ostream& os, const std::string& sep)
    	: os(&os), sep(sep) {}
		void operator()(const T* tp) { *os << *tp << sep; }		
	};




#if 0

#include <iostream>
using std::ostream;
#include <string>
using std::string;
#include <functional>
using std::unary_function;
using std::binary_function;
#include <utility>
using std::pair;
#include <map>
using std::map;

using std::cout;

/* Indirect functors */


/* Two 'closures' which combine member pointer with object pointer
   named after Borland keyword. Routine that creates closures
   is called 'member'.
*/


// Argument type goes first, so that selection of overloaded function
// is simplified




/* Workaround for compilers that do not allow returns with void */

template <class Operation> 
class vbinder1st : public unary_function<typename Operation::second_argument_type, void>
{
protected:
	Operation op;
    typename Operation::first_argument_type value;
  public:
    vbinder1st (const Operation& x, const typename Operation::first_argument_type& y)
		: op(x), value(y) {}
	void operator() (const typename Operation::second_argument_type& x) const
    { op(value, x); }
  };

template <class Operation, class T>
inline vbinder1st<Operation> vbind1st (const Operation& op, const T& x)
{
	typedef typename Operation::first_argument_type the_argument_type;
	return vbinder1st<Operation>(op, the_argument_type(x));
}

template <class Operation> 
class vbinder2nd : public unary_function<typename Operation::first_argument_type, void>
{
protected:
	Operation op;
	typename Operation::second_argument_type value;
  public:
    vbinder2nd (const Operation& x,
    	const typename Operation::second_argument_type& y) 
      : op(x), value(y) {}
    void operator() (const typename Operation::first_argument_type & x) const
    { op(x, value); }
};

template <class Operation, class T>
inline vbinder2nd<Operation> vbind2nd (const Operation& op, const T& x)
{
	typedef typename Operation::second_argument_type the_argument_type;
	return vbinder2nd<Operation>(op, the_argument_type(x));
}



template<class Arg, class Result, class Function>
struct immediate_function : public unary_function<Arg, typename Function::result_type>
{
	explicit immediate_function(Result (*ximf)(Arg), Function xf)
    	: imf(ximf), f(xf) {};
    typename Function::result_type operator()(const Arg &a) { return f(ximf(a)); }
private:
	Result (*imf)(Arg);
    Function f;
};

template<class Arg, class Result, class Function>
immediate_function<Arg, Result, Function>
immediate(Result (*fp)(Arg), Function &f)
{ return immediate_function<Arg, Result, Function>(fp, f); }

template<class ImFunction, class Function, class Result = typename Function::result_type>
struct immediate_function2 :
	public unary_function<typename ImFunction::argument_type, typename Function::result_type>
{
	explicit immediate_function2(ImFunction xim, Function xf)
	   	:  im(xim), f(xf) {}
	Result operator()(const typename ImFunction::argument_type &arg) { return f(im(arg)); }
private:
	ImFunction im;
    Function f;    	
};

template<class ImFunction, class Function>
struct immediate_function2<ImFunction, Function, void> :
	public unary_function<typename ImFunction::argument_type, typename Function::result_type>
{
	explicit immediate_function2(ImFunction xim, Function xf)
	   	:  im(xim), f(xf) {}
	void operator()(const typename ImFunction::argument_type &arg) { f(im(arg)); }
private:
	ImFunction im;
    Function f;    	
};


/*template<class ImFunction, class Function>
void immediate_function2<ImFunction, Function, void>::
operator()(const typename ImFunction::argument_type &arg) { f(im(arg)); }
  */

template<class ImFunction, class Function>
immediate_function2<ImFunction, Function>
immediate(ImFunction im, Function f)
{ return immediate_function2<ImFunction, Function>(im, f); }

template<class T>
struct pointer_outputter
{
	ostream *os;
    string sep;	
    pointer_outputter(ostream &xos, const string xsep)
    	: os(&xos), sep(xsep) {}
	void operator()(const T *tp) { *os << *tp << sep; }		

};

#endif

#endif
