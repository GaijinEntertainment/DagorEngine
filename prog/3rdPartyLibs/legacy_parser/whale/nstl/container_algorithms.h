
/** Defines overloaded versions of all STL algorithms. Those versions
	take container rather than pair of iterator, making STL more
	convenient.
		However, some of those algorithm _may_ ambiguate standard
	ones. It is because C++ lack any notion of 'concept', and 
	partial ordering rules are very very obscure. 

	Which is more specialized? Why?
	template<class T> g(T);
	template<class T> g(T&);

    It was written by Vladimir Prus <ghost@cs.msu.su>

    \note This file was called stl.h in previous version

    @file

	Rev. 0.34
*/

//  TODO: more specialized version for list
//	HOW OVERLOADING REACTS ON CONST ?? 

// This routine should be considered
//	swap_ranges
//	mismatch
//	transform
//	


#ifndef __CONTAINER_ALGORITHMS_H__
#define __CONTAINER_ALGORITHMS_H__


#include <algorithm>
#include <numeric>

#include <set>
#include <map>
#include <list>

namespace container_algorithms {

	// Initialization algorithms 

	template<class Container, class T>
		inline void
		fill(Container& container, const T& value)
		{
			std::fill(container.begin(), container.end(), value);
		}

	template<class Container, class Size, class T>
		inline void
		fill_n(Container& container, Size size, const T &value)
		{	
			std::fill_n(container.begin(), size, value);
		}


	template<class Container, class OutputIterator>
		inline OutputIterator
		copy(const Container& container, OutputIterator output)
		{
			return std::copy(container.begin(), container.end(), output);
		}

	template<class Container, class BidirectionalIterator>
		inline BidirectionalIterator
		copy_backward(const Container& container, BidirectionalIterator out)
		{
			return std::copy_backward(container.begin(), container.end(), out);
		}

	template<class Container, class Generator>
		inline void
		generate(Container &container, Generator gen)
		{
			return std::generate(container.begin(), container.end(), gen);
		}

	template<class Container, class Size, class Generator>
		inline void
		generate_n(Container &container, Size size, Generator gen)
		{
			std::generate_n(container.begin(), size, value);
		}

	template<class Container, class ForwardIterator>
		inline typename Container::iterator 
		swap_ranges(Container &container, ForwardIterator out)
		{
			return std::swap_ranges(container.begin(), container.end(), out);
		}

	// Searching operations 

	template<class Container, class T>
		inline typename Container::iterator
		find(Container &container, const T &value)
		{
			return std::find(container.begin(), container.end(), value);
		}

	template<class Container, class T>
		inline typename Container::const_iterator
		find(const Container &container, const T &value)
		{
			return std::find(container.begin(), container.end(), value);
		}

	template<class Container, class Predicate>
		inline typename Container::iterator
		find_if(Container &container, Predicate pred)
		{
			return std::find_if(container.begin(), container.end(), pred);
		}

	template<class Container, class Predicate>
		inline typename Container::const_iterator
		find_if(const Container &container, Predicate pred)
		{
			return std::find_if(container.begin(), container.end(), pred);
		}

	// More efficient versions for set/map

	template<class T>
		inline typename std::set<T>::const_iterator 
		find(const std::set<T> &the_set, const typename std::set<T>::value_type &value)
		{
			return the_set.find(value);
		}

	template<class T1, class T2>
		inline typename std::map<T1, T2>::iterator 
		find(std::map<T1, T2> &the_map, const typename std::map<T1, T2>::key_type &value)
		{
			return the_map.find(value);
		}

	template<class T1, class T2>
		inline typename std::map<T1, T2>::const_iterator 
		find(const std::map<T1, T2> &the_map, const typename std::map<T1, T2>::key_type &value)
		{
			return the_map.find(value);
		}

	template<class Container>
		inline typename Container::iterator
		adjacent_find(Container &container)
		{
			return std::adjacent_find(container.begin(), container.end());
		}

	template<class Container>
		inline typename Container::const_iterator
		adjacent_find(const Container &container)
		{
			return std::adjacent_find(container.begin(), container.end());
		}

	template<class Container, class Predicate>
		inline typename Container::iterator
		adjacent_find(Container & container, Predicate p)
		{
			return std::adjacent_find(container.begin(), container.end(), p);
		}

	template<class Container, class Predicate>
		inline typename Container::const_iterator 
		adjacent_find(const Container & container, Predicate p)
		{
			return std::adjacent_find(container.begin(), container.end(), p);
		}

	template<class Container1, class Container2>
		inline typename Container1::const_iterator
		find_first_of(const Container1 &container1, const Container2 &container2)
		{
			return std::find_first_of(container1.begin(), container1.end(),
					   				  container2.begin(), container2.end());
		}

	template<class Container1, class Container2>
		inline typename Container1::iterator
		find_first_of(Container1 &container1, const Container2 &container2)
		{
			return std::find_first_of(container1.begin(), container1.end(),
									  container2.begin(), container2.end());
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::iterator
		find_first_of(Container1 &container1, const Container2 &container2,
					  Predicate pred)
		{
			return std::find_first_of(container1.begin(), container1.end(),
					   				  container2.begin(), container2.end(),
					   				  pred);
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::const_iterator
		find_first_of(const Container1 &container1, const Container2 &container2,
					  Predicate pred)
		{
			return std::find_first_of(container1.begin(), container1.end(),
									  container2.begin(), container2.end(),
									  pred);
		}

	template<class Container1, class Container2>
		inline typename Container1::iterator
		find_end(Container1& container1, const Container2& container2)
		{
			return std::find_end(container1.begin(), container1.end(),
								 container2.begin(), container2.end());
		}

	template<class Container1, class Container2>
		inline typename Container1::const_iterator
		find_end(const Container1& container1, const Container2& container2)
		{
			return std::find_end(container1.begin(), container1.end(),
								 container2.begin(), container2.end());
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::iterator
		find_end(Container1& container1, const Container2& container2,
				 Predicate p)
		{
			return std::find_end(container1.begin(), container1.end(),
								 container2.begin(), container2.end(), p);
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::const_iterator
		find_end(const Container1& container1, const Container2& container2,
				 Predicate p)
		{
			return std::find_end(container1.begin(), container1.end(),
								 container2.begin(), container2.end(), p);
		}

	template<class Container1, class Container2>
		inline typename Container1::const_iterator
		search(const Container1 &container1, const Container2 &container2)
		{
			return std::search(container1.begin(), container1.end(),
							   container2.begin(), container2.end());
		}

	template<class Container1, class Container2>
		inline typename Container1::iterator
		search(Container1 &container1, const Container2 &container2)
		{
			return std::search(container1.begin(), container1.end(),
							   container2.begin(), container2.end());
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::iterator
		search(Container1 &container1, const Container2 &container2,
			   Predicate pred)
		{
			return std::search(container1.begin(), container1.end(),
							   container2.begin(), container2.end(), pred);
		}

	template<class Container1, class Container2, class Predicate>
		inline typename Container1::const_iterator
		search(const Container1 &container1, const Container2 &container2,
			   Predicate pred)
		{
			return std::search(container1.begin(), container1.end(),
							   container2.begin(), container2.end(), pred);
		}

	template<class Container, class Size, class T>
		inline typename Container::iterator
		search_n(Container& container, Size count, const T& value)
		{
			return std::search_n(container.begin(), container.end(), 
								 count, value);
		}

	template<class Container, class Size, class T>
		inline typename Container::const_iterator
		search_n(const Container& container, Size count, const T& value)
		{
			return std::search_n(container.begin(), container.end(), 
								 count, value);
		}

	template<class Container, class Size, class T, class Predicate>
		inline typename Container::iterator
		search_n(Container& container, Size count, const T& value,
				 Predicate pred)
		{
			return std::search_n(container.begin(), container.end(), 
								 count, value, pred);
		}

	template<class Container, class Size, class T, class Predicate>
		inline typename Container::const_iterator
		search_n(const Container& container, Size count, const T& value,
				 Predicate pred)
		{
			return std::search_n(container.begin(), container.end(), 
								 count, value, pred);
		}

	template<class Container>
		inline typename Container::const_iterator
		max_element(const Container &container)
		{
			return std::max_element(container.begin(), container.end());
		}

	template<class Container>
		inline typename Container::iterator
		max_element(Container &container)
		{
			return std::max_element(container.begin(), container.end());
		}

	template<class Container>
		inline typename Container::const_iterator
		min_element(const Container &container)
		{
			return std::min_element(container.begin(), container.end());
		}

	template<class Container>
		inline typename Container::iterator
		min_element(Container &container)
		{
			return std::min_element(container.begin(), container.end());
		}

	template<class Container, class InputIterator>
		inline typename Container::const_iterator
		mismatch(const Container &container, InputIterator i)
		{
			return std::mismatch(container.begin(), container.end(), i);
		}

	template<class Container, class InputIterator>
		inline typename Container::iterator
		mismatch(Container &container, InputIterator i)
		{
			return std::mismatch(container.begin(), container.end(), i);
		}

	// In-place transformations

	template<class Container>
		inline void
		reverse(Container &container)
		{
			std::reverse(container.begin(), container.end());
		}

	template<class Container, class T>
		inline void
		replace(Container& container, const T& what, const T& with_what)
		{
			std::replace(container.begin(), container.end(), what, with_what);
		}

	template<class Container, class Predicate, class T>
		inline void
		replace_if(Container& container, Predicate pred, const T& value)
		{
			std::replace_if(container.begin(), container.end(), pred, value);
		}

	template<class Container, class OutputIterator, class T>
		inline OutputIterator
		replace_copy(const Container& container, OutputIterator out,
					 const T& what, const T& value)
		{
			return std::replace_if(container.begin(), container.end(),
								   out, what, value);
		}

	template<class Container, class OutputIterator, class Predicate, class T>
		inline OutputIterator
		replace_copy_if(const Container& container, OutputIterator out,
						Predicate pred, const T& value)
		{
			return std::replace_copy_if(container.begin(), container.end(),
										out, pred, value);
		}

	template<class Container, class ForwardIterator>
		inline void
		rotate(Container& container, ForwardIterator middle)
		{
			return rotate(container.begin(), container.end(), middle);
		}

	template<class Container, class Predicate>
		inline typename Container::iterator
		partition(Container& container, Predicate pred)
		{
			return std::partition(container.begin(), container.end(), pred);
		}

	template<class Container, class Predicate>
		inline typename Container::iterator
		stable_partition(Container& container, Predicate pred)
		{
			return std::stable_partition(container.begin(), container.end(), pred);
		}

	template<class Container>
		inline bool
		next_permutation(Container& container) 
		{
			return std::next_permutation(container.begin(), container.end());
		}

	template<class Container>
		inline bool
		prev_permutation(Container& container)
		{
			return std::prev_permutation(container.begin(), container.end());
		}

	template<class Container>
		inline void
		inplace_merge(Container& container, typename Container::iterator middle)
		{
			return std::inplace_merge(container.begin(), middle, container.end());
		}

	template<class Container>
		inline void
		random_shuffle(Container& container)
		{
			std::random_shuffle(container.begin(), container.end());
		}

	template<class Container, class T>
		inline typename Container::iterator
		remove(Container& container, const T& what) 
		{
			return std::remove(container.begin(), container.end(), what);
		}

	template<class Container, class Predicate>
		inline typename Container::iterator
		remove_if(Container& container, Predicate pred) 
		{
			return std::remove_if(container.begin(), container.end(), pred);
		}

	template<class Container, class OutputIterator, class T>
		inline OutputIterator 
		remove_copy(const Container& container, OutputIterator out,
					const T& value)
		{
			return std::remove_copy(container.begin(), container.end(),
									out, value);
		}

	template<class Container, class OutputIterator, class Predicate>
		inline OutputIterator 
		remove_copy_if(const Container& container, OutputIterator out,
					   Predicate pred)
		{
			return remove_copy_if(container.begin(), container.end(), out, pred);
		}

	template<class Container>
		inline typename Container::iterator
		unique(Container& container) 
		{
			return std::unique(container.begin(), container.end());
		}

	template<class Container, class OutputIterator>
		inline OutputIterator
		unique_copy(const Container& container, OutputIterator out)
		{
			return std::unique_copy(container.begin(), container.end(), out);
		}

	// Scalar-producing algorithms

	template<class Container, class T>
		inline std::iterator_traits<typename Container::iterator>::difference_type
		count(const Container& container, const T& value)
		{
			return std::count(container.begin(), container.end(), value);
		}

	template<class Container, class Predicate>
		inline std::iterator_traits<typename Container::iterator>::difference_type
		count_if(const Container& container, Predicate pred)
		{
			return std::count_if(container.begin(), container.end(), pred);
		}

	template<class Container, class ContainerType>
		inline ContainerType
		accumulate(const Container& container, ContainerType initial)
		{
			return std::accumulate(container.begin(), container.end(), initial);
		}

	template<class Container, class ContainerType, class BinaryOperation>
		inline ContainerType
		accumulate(const Container& container, const ContainerType& initial,
				   BinaryOperation op)
		{
			return std::accumulate(container.begin(), container.end(),
								   initial, op);
		}

	template<class Container1, class Container2, class ContainerType>
		inline ContainerType                             
		inner_product(const Container1& c1, const Container2& c2,
					  const ContainerType initial)
		{
			return std::inner_product(c1.begin(), c1.end(), c2.begin(), initial);
		}

	template<class Container1, class Container2>
		inline bool
		equal(const Container1& container1, const Container2& container2)
		{ 
			if (container1.size() != container2.size()) return false;
			return std::equal(container1.begin(), container1.end(),
							  container2.begin()); 
		}

	template<class Container1, class Container2>
		inline bool
		lexicographical_compare
		(const Container1& c1, const Container2& c2)
		{
			return std::lexicographical_compare(c1.begin(), c1.end(),
												c2.begin(), c2.end());
		} 

	// Sequence-generating algorithms

	template<class Container, class OutputIterator, class UnaryFunction>
		inline OutputIterator
		transform(const Container& container, OutputIterator result,
				  UnaryFunction f)
		{
			return std::transform(container.begin(), container.end(), result, f);
		}

	template<class Container, class UnaryFunction>
		inline typename Container::iterator
		transform (Container &container, UnaryFunction f)
		{
			return std::transform(container.begin(), container.end(),
								  container.begin(), f);
		}

	template<class Container, class OutputIterator>
		inline OutputIterator
		partial_sum(const Container& container, OutputIterator out)
		{
			return std::partial_sum(container.begin(), container.end(), out);
		}

	template<class Container, class OutputIterator>
		inline OutputIterator
		adjacent_difference(const Container& container, OutputIterator out)
		{
			return std::adjacent_difference(container.begin(), container.end(),
											out);
		}

	template<class Container, class OutputIterator, class Predicate>
		inline OutputIterator
		adjacent_difference(const Container& container, OutputIterator out,
						    Predicate pred)
		{
			return std::adjacent_difference(container.begin(), container.end(),
											out, pred);
		}

	template<class Container, class Function>
		inline void
		for_each(const Container &container, Function f)
		{
			std::for_each(container.begin(), container.end(), f);
		}

	// Ordered collection algorithms 

	template<class Container>
		inline void
		sort(Container& container)
		{
			std::sort(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		sort(Container &container, Compare comp)
		{
			std::sort(container.begin(), container.end(), comp);
		}

	template<class T, class A>
		inline void
		sort(std::list<T, A>& l)
		{
			l.sort();
		}

	template<class T, class A, class Compare>
		inline void
		sort(std::list<T, A>& l, Compare comp)
		{
			l.sort(comp);
		}

	template<class Container>
		inline void
		stable_sort(Container &container)
		{
			std::stable_sort(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		stable_sort(Container &container, Compare comp)
		{
			std::stable_sort(container.begin(), container.end(), comp);
		}

	template<class Container>
		inline void
		partial_sort(Container& container, typename Container::iterator middle)
		{
			std::partial_sort(container.begin(), middle, container.end());
		}

	template<class Container, class Compare>
		inline void
		partial_sort(Container& container, typename Container::iterator middle,
					 Compare comp)
		{
			std::partial_sort(container.begin(), middle, container.end(), comp);
		}

	template<class Container1, class Container2>
		inline void
		partial_sort_copy(Container1& container1, Container2& container2)
		{
			std::partial_sort_copy(container1.begin(), container1.end(),
								   container2.begin(), container2.end());
		}

	template<class Container1, class Container2, class Compare>
		inline void
		partial_sort_copy(Container1& container1, Container2& container2,
						  Compare comp)
		{
			std::partial_sort_copy(container1.begin(), container1.end(),
								   container2.begin(), container2.end(),
								   comp);
		}

	template<class Container>
		inline void
		nth_element(Container& container, typename Container::iterator nth)
		{
			std::nth_element(container.begin(), nth, container.end());
		}

	template<class Container, class Compare>
		inline void
		nth_element(Container& container, typename Container::iterator nth,
					Compare comp)
		{
			std::nth_element(container.begin(), nth, container.end(), comp);
		}

	template<class Container, class T>
		inline bool
		binary_search(const Container& container, const T& value)
		{
			return std::binary_search(container.begin(), container.end(), 
									  value);
		}

	template<class Container, class T, class Compare>
		inline bool
		binary_search(const Container& container, const T& value, Compare comp)
		{
			return std::binary_search(container.begin(), container.end(), 
									  value, comp);
		}

	template<class Container, class T>
		inline typename Container::iterator
		lower_bound(Container& container, const T& value)
		{
			return std::lower_bound(container.begin(), container.end(), value);
		}

	template<class Container, class T>
		inline typename Container::const_iterator
		lower_bound(const Container& container, const T& value)
		{
			return std::lower_bound(container.begin(), container.end(), value);
		}

	template<class Container, class T, class Compare>
		inline typename Container::iterator
		lower_bound(Container& container, const T& value, Compare comp)
		{
			return std::lower_bound(container.begin(), container.end(), value,
									comp);
		}

	template<class Container, class T, class Compare>
		inline typename Container::const_iterator
		lower_bound(const Container& container, const T& value, Compare comp)
		{
			return std::lower_bound(container.begin(), container.end(), value,
									comp);
		}

	template<class Container, class T>
		inline typename Container::iterator
		upper_bound(Container& container, const T& value)
		{
			return std::upper_bound(container.begin(), container.end(), value);
		}

	template<class Container, class T>
		inline typename Container::const_iterator
		upper_bound(const Container& container, const T& value)
		{
			return std::upper_bound(container.begin(), container.end(), value);
		}

	template<class Container, class T, class Compare>
		inline typename Container::iterator
		upper_bound(Container& container, const T& value, Compare comp)
		{
			return std::upper_bound(container.begin(), container.end(), value,
									comp);
		}

	template<class Container, class T, class Compare>
		inline typename Container::const_iterator
		upper_bound(const Container& container, const T& value, Compare comp)
		{
			return std::upper_bound(container.begin(), container.end(), value,
									comp);
		}

	template<class Container, class T>
		inline std::pair<typename Container::iterator, typename Container::iterator>
		equal_range(Container& container, const T& value)
		{
			return std::equal_range(container.begin(), container.end(), value);
		}

	template<class Container, class T>
		inline std::pair<typename Container::const_iterator, typename Container::const_iterator>
		equal_range(const Container& container, const T& value)
		{
			return std::equal_range(container.begin(), container.end(), value);
		}

	template<class Container, class T, class Compare>
		inline std::pair<typename Container::iterator, typename Container::iterator>
		equal_range(Container& container, const T& value, Compare comp)
		{
			return std::equal_range(container.begin(), container.end(), value,
									comp);
		}

	template<class Container, class T, class Compare>
		inline std::pair<typename Container::const_iterator, typename Container::const_iterator>
		equal_range(const Container& container, const T& value, Compare comp)
		{
			return std::equal_range(container.begin(), container.end(), value,
									comp);
		}

	template<class Container1, class Container2, class OutputIterator>
		inline OutputIterator
		merge(const Container1& container1, const Container2& container2,
			  OutputIterator i)
		{
			return std::merge(container1.begin(), container1.end(),
							  container2.begin(), container2.end(), i);
		}	

	template<class Container1, class Container2, class OutputIterator,
			 class Compare>
		inline OutputIterator
		merge(const Container1& container1, const Container2& container2,
			  OutputIterator i, Compare comp)
		{
			return std::merge(container1.begin(), container1.end(),
							  container2.begin(), container2.end(), i, comp);
		}	

	
	template<class Container1, class Container2, class OutputIterator>
		inline OutputIterator
		set_union(const Container1& container1, const Container2& container2,
			  	  OutputIterator i)
		{
			return std::set_union(container1.begin(), container1.end(),
							  	  container2.begin(), container2.end(), i);
		}	

	template<class Container1, class Container2, class OutputIterator,
			 class Compare>
		inline OutputIterator
		set_union(const Container1& container1, const Container2& container2,
			  	  OutputIterator i, Compare comp)
		{
			return std::set_union(container1.begin(), container1.end(),
							  	  container2.begin(), container2.end(), i, comp);
		}
		
	template<class Container1, class Container2, class OutputIterator>
		inline OutputIterator
		set_intersection(const Container1& container1,
						 const Container2& container2, OutputIterator i)
		{
			return std::set_intersection(container1.begin(), container1.end(),
							  	  		container2.begin(), container2.end(), i);
		}	

	template<class Container1, class Container2, class OutputIterator,
			 class Compare>
		inline OutputIterator
		set_intersection(const Container1& container1,
						 const Container2& container2, OutputIterator i,
						 Compare comp)
		{
			return std::set_inersection(container1.begin(), container1.end(),
							  	  		container2.begin(), container2.end(),
							  	  		i, comp);
		}
	
   	template<class Container1, class Container2, class OutputIterator>
		inline OutputIterator
		set_difference(const Container1& container1,
					   const Container2& container2,  OutputIterator i)
		{
			return std::set_difference(container1.begin(), container1.end(),
							  	  	   container2.begin(), container2.end(), i);
		}	

	template<class Container1, class Container2, class OutputIterator,
			 class Compare>
		inline OutputIterator
		set_difference(const Container1& container1,
					   const Container2& container2, OutputIterator i, Compare comp)
		{
			return std::set_difference(container1.begin(), container1.end(),
							  	  	   container2.begin(), container2.end(),
							  	  	   i, comp);
		}
	
	template<class Container1, class Container2, class OutputIterator>
		inline OutputIterator
		set_symmetric_difference(const Container1& container1,
								 const Container2& container2,
			  	  				 OutputIterator i)
		{
			return std::set_symmetric_difference(container1.begin(),
												 container1.end(),
							  	  				 container2.begin(),
							  	  				 container2.end(), i);
		}	

	template<class Container1, class Container2, class OutputIterator,
			 class Compare>
		inline OutputIterator
		set_symmetric_difference(const Container1& container1,
								 const Container2& container2,
								 OutputIterator i, Compare comp)
		{
			return std::set_symmetric_difference(container1.begin(),
												 container1.end(),
							  	  				 container2.begin(),
							  	  				 container2.end(), i, comp);
		}

	template<class Container1, class Container2>
		inline bool 
		includes(const Container1& container1, const Container2& container2)
		{
			return std::includes(container1.begin(), container1.end(),
								 container2.begin(), container2.end());
		}

	template<class Container1, class Container2, class Compare>
		inline bool 
		includes(const Container1& container1, const Container2& container2,
				 Compare comp)
		{
			return std::includes(container1.begin(), container1.end(),
								 container2.begin(), container2.end(), comp);
		}

	template<class Container>
		inline void 
		make_heap(Container& container)
		{
			std::make_heap(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		make_heap(Container& container, Compare comp)
		{
			std::make_heap(container.begin(), container.end(), comp);
		}	

	template<class Container>
		inline void 
		push_heap(Container& container)
		{
			std::push_heap(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		push_heap(Container& container, Compare comp)
		{
			std::push_heap(container.begin(), container.end(), comp);
		}	

	template<class Container>
		inline void 
		pop_heap(Container& container)
		{
			std::pop_heap(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		pop_heap(Container& container, Compare comp)
		{
			std::pop_heap(container.begin(), container.end(), comp);
		}

	template<class Container>
		inline void 
		sort_heap(Container& container)
		{
			std::sort_heap(container.begin(), container.end());
		}

	template<class Container, class Compare>
		inline void
		sort_heap(Container& container, Compare comp)
		{
			std::sort_heap(container.begin(), container.end(), comp);
		}
}	


#endif
