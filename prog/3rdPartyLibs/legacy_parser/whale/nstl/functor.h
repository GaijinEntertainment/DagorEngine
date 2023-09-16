
/* Lazy, functional containers are implemented this way:
	Actual processing is delegated to Context, which generetes sequence of
	values through its methods current(), advance() and eof(). 
	Class fn_iterator, parametrized with context type, provides interface to
	the container and behaves like input_iterator.
	fn_container has only two methods, begin() and end() returning 
	fn_iterator.
	
	
*/

#ifndef __FUNCTOR_H_
#define __FUNCTOR_H_

#include <cstdlib>
using std::size_t;

/*
 *  Forward declarations 
 */

template<class Context>
  class fn_context;

template<class Functor>
  class fn_infinite_context;

template<class State, class Functor>
  class fn_infinite_context_with_state;

template<class Functor>
  class fn_context_with_count;

template<class Functor, class State>
  class fn_context_with_count_and_state;

template<class Container, class Expression, class Predicate>
  class fn_comprehending_context;



	//template<class Container, class Functor> class fn_transforming_context;
	//template<class Container, class Functor> class fn_filtering_context;

template<class Context>					class fn_iterator;
template<class Context>					class fn_container;


/*
 *  Definitions of contexts
 */

template<class Value>
class fn_context
{
public:
	typedef Value value_type;

	const value_type& current() { return scratch; }
protected:
	void current(const value_type& v) { scratch = v; }	
	value_type scratch;
};

template<class Functor>
class fn_infinite_context : public fn_context<typename Functor::result_type>
{
	Functor f;

public:
	fn_infinite_context(Functor f = Functor()) : f(f)
		{ advance(); }

	void advance() { current(f()); }
	bool eof() const { return false; }
};

template<class Functor, class State>
class fn_infinite_context_with_state : public fn_context<typename Functor::result_type>
{
	Functor f;
    State s;
public:
	fn_infinite_context_with_state() {}
	fn_infinite_context_with_state(Functor f, const State &s)
		: s(s), f(f)
		{ advance(); }

	void advance() { current(f(s)); }
	bool eof() const { return false; }
};


template<class Functor>
class fn_context_with_count : public fn_context<typename Functor::result_type>
{
	int n;
	Functor f;
public:
	fn_context_with_count() : n(-1) {}
	fn_context_with_count(int n, Functor f = Functor()) : n(n), f(f)
		{ advance(); }

	void advance() { current(f()); --n; }
	bool eof()	const { return n < 0; }
};

template<class Functor, class State>
class fn_context_with_count_and_state :
	public fn_context<typename Functor::result_type>
{
	int n;
	Functor f;
	State s;
public:
	fn_context_with_count_and_state() : n(-1) {}

	fn_context_with_count_and_state(int n, Functor f = Functor(),
									const State& s = State())
		: n(n), f(f), s(s)
		{ advance(); }

	void advance() { current(f(s)); --n; }
	bool eof()	const { return n < 0; }
};


template<class Container, class Expression, class Predicate>
class fn_comprehending_context : public fn_context<typename Expression::result_type>
{	
	bool _force_eof;	// ????
	typename Container::const_iterator pos, end;
	Expression e;
	Predicate p;
public:
	typedef typename Expression::result_type value_type;

   	fn_comprehending_context() : _force_eof(true) {}

	fn_comprehending_context(const Container &c, Expression e, Predicate p)
		: pos(c.begin()), end(c.end()), e(e), p(p), _force_eof(false)
		{ advance(true); }
	void advance(bool first_time = false) {
    	if (!first_time) ++pos;
		while(pos != end && !p(*pos)) ++pos;
		if (pos != end) current(e(*pos));
	}			
	bool eof() const { return _force_eof || pos == end; }
};


/*
template<class Container, class Functor>
class fn_transforming_context
{
public:
	typedef typename Functor::result_type value_type;

	fn_transforming_context(const Container& c, Functor f = Functor())
		: pos(c.begin()), end(c.end()), f(f) {}

	void advance() { current(f(*pos++)); }
	bool eof()	const { return pos == end; }
};
*/




/* Method that any context must implement
	
class Context
{
	typedef something value_type;


	// If sequence if finite, eof must return true for default context
	Context();	

	// current should be valid atfer call
	Context( something );
	
	// The reference returned should be valid at least until next
	// advance call
	const value_type&    current();
	// Changes current item. After the call either current() should return
	// valid reference, or eof() should return true
	void advance();
	//	Tells if current can be called with any meaning	
	eof();
};
*/

#include <iterator>

template<class Context>
class fn_iterator : public
		std::iterator<std::input_iterator_tag, typename Context::value_type>
{
	Context context;

public:
	
	typedef typename Context::value_type value_type;
	typedef const typename Context::value_type * pointer;
	typedef const typename Context::value_type & reference;

    fn_iterator(const Context& context = Context()) : context(context) {}

	// All operations below correspond to section 24.1.1 of the standard 
	// expect for ==, which is not an equvivalence relation

	// copy ctor and assigment operators are implicitly defined

	bool operator==(const fn_iterator& another) const {
		return context.eof() && another.context.eof();	
	}
	bool operator!=(const fn_iterator& another) const {
		return !(*this == another);
	}
	reference operator*() const {
		return context.current();
	}
	pointer operator->() const {
		return &context.current();
	}
	fn_iterator& operator++() {
		context.advance();
		return *this;
	}	

	struct proxy {
		value_type value;
		proxy(const value_type &value) : value(value) {};
		value_type &operator*() { return value; }
	};
	proxy operator++(int) {
		proxy p(context.current());
		++(*this);
		return p;
	}
};

template<class Context>
class fn_container
{
public:
	typedef typename Context::value_type value_type;
	typedef fn_iterator<Context> iterator;
   	typedef fn_iterator<Context> const_iterator;

	fn_container(const Context& context = Context()) : _begin(context) {}
	iterator  begin()	const 	{	return _begin;	}
	iterator	end()	const	{	return iterator(); }

protected:
	iterator _begin;	
};

template<class Iterator>
class iterators_pair
{
public:
	typedef Iterator iterator;
	typedef Iterator const_iterator;


	iterators_pair(Iterator begin, Iterator end) : _begin(begin), _end(end) {}

	Iterator begin() const { return _begin; }
	Iterator end()   const { return _end; }

	bool empty() const { return _begin == _end; }

protected:
	Iterator _begin, _end;

};

#endif
