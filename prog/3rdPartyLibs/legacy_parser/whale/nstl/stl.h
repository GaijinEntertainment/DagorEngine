

#ifndef _STL_H_
#define _STL_H_

#include "container_algorithms.h"

// Take the 'good' ones


using container_algorithms::fill;
using container_algorithms::copy;
using container_algorithms::copy_backward;
using container_algorithms::generate;
using container_algorithms::swap_ranges;
using container_algorithms::find;
using container_algorithms::find_if;
using container_algorithms::find_first_of;
using container_algorithms::find_end;
using container_algorithms::search;
using container_algorithms::max_element;
using container_algorithms::min_element;
using container_algorithms::mismatch;
using container_algorithms::reverse;
using container_algorithms::replace;
using container_algorithms::replace_if;
using container_algorithms::replace_copy;
using container_algorithms::replace_copy_if;
using container_algorithms::rotate;
using container_algorithms::partition;
using container_algorithms::stable_partition;
using container_algorithms::next_permutation;
using container_algorithms::prev_permutation;
using container_algorithms::inplace_merge;
using container_algorithms::remove;
using container_algorithms::remove_if;
using container_algorithms::remove_copy;
using container_algorithms::remove_copy_if;
using container_algorithms::unique;
using container_algorithms::unique_copy;
using container_algorithms::count;
using container_algorithms::count_if;
using container_algorithms::inner_product;
using container_algorithms::equal;
using container_algorithms::lexicographical_compare;
using container_algorithms::transform;
using container_algorithms::partial_sum;
using container_algorithms::for_each;
using container_algorithms::merge;
using container_algorithms::set_union;
using container_algorithms::set_intersection;
using container_algorithms::set_difference;
using container_algorithms::set_symmetric_difference;
using container_algorithms::includes;


// Take good versions of bad algorithms, if such exist

//fill_n 		-- no good versions

//generate_n 	-- no good versions

//adjacent_find	

	template<class Container>
		inline typename Container::iterator
		adjacent_find(Container &container)
		{
			return std::adjacent_find(container.begin(), container.end());
		}

//search_n 	 	-- no good versions


//accumulate

	template<class Container, class ContainerType>
		inline ContainerType
		accumulate(const Container& container, ContainerType initial)
		{
			return std::accumulate(container.begin(), container.end(), initial);
		}


//adjacent_difference

	template<class Container, class OutputIterator>
		inline OutputIterator
		adjacent_difference(const Container& container, OutputIterator out)
		{
			return std::adjacent_difference(container.begin(), container.end(),
											out);
		}

//sort
	
	template<class Container>
		inline void
		sort(Container& container)
		{
			std::sort(container.begin(), container.end());
		}

//stable_sort

	template<class Container>
		inline void
		stable_sort(Container &container)
		{
			std::stable_sort(container.begin(), container.end());
		}

//partial_sort

	template<class Container>
		inline void
		partial_sort(Container& container, typename Container::iterator middle)
		{
			std::partial_sort(container.begin(), middle, container.end());
		}

//partial_sort_copy

	template<class Container1, class Container2>
		inline void
		partial_sort_copy(Container1& container1, Container2& container2)
		{
			std::partial_sort_copy(container1.begin(), container1.end(),
								   container2.begin(), container2.end());
		}

//nth_element

	template<class Container>
		inline void
		nth_element(Container& container, typename Container::iterator nth)
		{
			std::nth_element(container.begin(), nth, container.end());
		}
	
//binary_search

	template<class Container, class T>
		inline bool
		binary_search(const Container& container, const T& value)
		{
			return std::binary_search(container.begin(), container.end(),
									  value);
		}

//lower_bound

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

	
//upper_bound

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
	

//equal_range

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
	

//make_heap
//push_heap
//pop_heap
//sort_heap



#endif
