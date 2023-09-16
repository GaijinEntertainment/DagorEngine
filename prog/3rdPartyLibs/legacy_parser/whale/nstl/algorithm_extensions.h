/*
    Extra algorithms based on STL algorithm and containers 
    Defined here are
    - for_each2 - applies a function to object in containers contained in
      another container
    - exists, all, exists_and_only
    - erase, unique_and_erase which handle end() properly
        ( vector will fail if end() is passes to erase)
    - make_set - sorts container, removes duplicates
    - buble sort, which is more efficient with small sequences
    - find_index, find_index_if
    - find_all_if, find_all_indices_if, find_all_indices -- return vector
        of indices or values 
    - multimap routines
    - get_all - returns all elements with the same key value
    - same_priorities - vector of vector of elements with the same key value
    - priority_multimap - creates multimap using supplied vectors of keys
        and values
    - reorder - reorders element in container accoring to vector of new
        indices

    Written by Vladimir Prus <ghost@cs.msu.su>

    Rev 0.37
*/

#ifndef _STL_EXTRA_H_5912BA20
#define _STL_EXTRA_H_5912BA20

#pragma option push -w-inl

#include <vector>
#include <set>
#include <map>

#include "checks.h"



    template<class Container, class Function>
        inline void
        for_each2(const Container&  container, Function f)
    {
        typename Container::const_iterator i;
            for (i = container.begin(); i != container.end(); ++i)
                    for_each(*i, f);
    }

    template<class Container, class V, class Container2, class Function>
        inline void
        for_each2(const Container &container, const Container2 V::* c2p,
                  Function f)
        {
            typename Container::const_iterator i;
            for (i = container.begin(); i != container.end(); ++i)
                for_each(&(*i)->*c2p, f);
    }


    // Quantative functions 

    template<class Container, class Predicate>
        inline bool
        exists(const Container& cont, Predicate pred)
        {
            return (find_if(cont, pred) != cont.end());
        }

    template<class Container, class Predicate>
        inline bool
        all(const Container &cont, Predicate pred)
        {
            typename Container::const_iterator i;
            for (i = cont.begin(); i != cont.end(); i++)
                if (!pred(*i)) return false;
            return true;
        }

    template<class Container, class Predicate>
        inline bool
        exists_and_only(const Container& cont, Predicate pred)
        {
            typename Container::iterator i = find_if(cont, pred);
            if (i == cont.end()) return false;
            if (find_if(++i, cont.end(), pred) != cont.end()) return false;
            return true;
        }

    template<class CT, class PT>
        inline bool
        contains(const CT& cont, const PT& probe)
        {
            return (find(cont, probe) != cont.end());
        }

    // Convenient modifiers     

    template<class Container1, class Container2>
        inline void
        append(Container1& container1, const Container2& container2)
        {
            container1.insert(container1.end(), container2.begin(), container2.end());
        }

    template<class Key, class Compare, class Allocator, class Container2>
        inline void
        append(std::set<Key, Compare, Allocator> &s, const Container2& container2)
        {
            s.insert(container2.begin(), container2.end());
        }

    template<class Container, class T>
        inline void
        erase(Container& container, const T& value)
        { 
            typename Container::iterator i = find(container, value);
            if (i != container.end()) container.erase(i);
        }

    template<class Container, class T>
        inline void
        remove_and_erase(Container& container, const T& value)
        {
            container.erase(remove(container, value), container.end());
        }

    template<class Container, class Predicate>
        inline  void
        remove_and_erase_if(Container& container, Predicate p)
        {
            container.erase(remove_if(container, p), container.end());
        }

    template<class Container, class Predicate>
        inline void
        remove_and_erase_indices(Container& container, Predicate p)
        {
            // Probably can do it in-place, but with auxillary indices store?

            Container tmp;

            for (typename Container::size_type i = 0; i < container.size(); ++i)
            {
                if (!p(i)) tmp.insert(tmp.end(), container[i]);
            }
            container.swap(tmp);
        }

    template<class Container>
        inline void
        grow(Container& container, std::size_t by_how_much = 1)
        {
            container.resize(container.size() + by_how_much);
        }


    template<class Container>
        inline void
        grow_to(Container& container, std::size_t to_what_size)
        {
            container.resize(max(container.size(), to_what_size));
        }

    // Sorted collections staff 
    
    template<class Container>
        inline void
        unique_and_erase(Container& container)
        {
            typename Container::iterator i = unique(container.begin(), container.end());
            if (i != container.end())
                container.erase(i, container.end());
        }

    template<class RandomAccessIterator>
        void
        bubble(RandomAccessIterator first, RandomAccessIterator last)
        {
            bool switch_flag = true;
            while(switch_flag && !(switch_flag = false)) {
        
                RandomAccessIterator aux = first;
                if (first != last)
                    for(--last; aux != last; ++aux) 
                        if (*(aux + 1) < *aux) 
                        { swap(*aux, *(aux + 1)); switch_flag = true; }
            }
        }


    template<class Container>
        inline void
        bubble(Container &container)
        {
            bubble(container.begin(), container.end());
        }

    template<class Container>
        inline void 
        make_set(Container& container)
        {
            if(container.size() > 6)
                sort(container.begin(), container.end());
            else bubble(container);
            unique_and_erase(container);
        }

    template<class C>
        class as_sorted_t
        {
            C& c;

        public:
            as_sorted_t(C& c) : c(c) {}

            operator C&() { return c; }
            C& contained() const { return c; }

            void operator*=(const C& another)
            {
                C tmp;
                set_intersection(c.begin(), c.end(), another.begin(), another.end(),
                                    inserter(tmp, tmp.begin()));
                c.swap(tmp);
            }   

            void operator+=(const C& another)
            {
                copy(another.begin(), another.end(), back_inserter(c));
                make_set(c);
            }

            void operator-=(const C& another)
            {
                C tmp;
                set_difference(c.begin(), c.end(), another.begin(), another.end(),
                                inserter(tmp, tmp.begin()));
                c.swap(tmp);
            }

            bool operator<=(const C& another)
            {
                return includes(another.begin(), another.end(), c.begin(), c.end());
            }
    };

    template<class C>
        as_sorted_t<C>
        as_sorted(C& c)
        {
            return as_sorted_t<C>(c);
        }

    template<class C>
        as_sorted_t<const C>
        as_sorted(const C& c)
        {
            return as_sorted_t<const C>(c);
        }

    // Both find_index below return size_t. It is temporary fix.
    // (is first returns Container::difference_type, compile error will
    // happen whenever as_sorted<T> is passed, because it doesn't have
    // such member)         
    template<class Container, class T>
        inline typename std::size_t
        find_index(const Container &container, const T& value)
        {
            const typename Container::const_iterator i = find(container, value);
            if (i != container.end()) return distance(container.begin(), i);
            return -1; 
        }

    template<class C, class T>
        inline typename std::size_t
        find_index(const as_sorted_t<C>& a, const T& value)
        {
            return find_index(a.contained(), value);
        }   

    template<class Container, class Predicate>
        inline typename Container::difference_type
        find_index_if(const Container &container, Predicate pred)
        {
            const typename Container::const_iterator i = find_if(container, pred);
            if (i != container.end()) return distance(container.begin(), i);
            return -1; 
        }

    // Vector of values generators

    template<class InputIterator, class Predicate>
        std::vector<InputIterator>
        find_all_if(InputIterator b, InputIterator e, Predicate pred)
        {
            std::vector<InputIterator> result;
            for (InputIterator i = b; i != e; i++)
            if (pred(*i)) result.push_back(i);
                return result;
        }

    template<class Container, class Predicate>
        inline std::vector<typename Container::iterator>
        find_all_if(Container &container, Predicate pred)
        {
            return find_all_if(container.begin(), container.end(), pred);   
        }

    template<class InputIterator, class Predicate>
        std::vector<int>
        find_all_indices_if(InputIterator b, InputIterator e, Predicate pred)
        {
            std::vector<int> result;
            for (InputIterator i = b; i != e; i++)
                if (pred(*i)) result.push_back(distance(b, i));
            return result;
        }

    template<class Container, class Predicate>
        inline std::vector<int>
        find_all_indices_if(Container &container, Predicate pred)
        {
            return find_all_indices_if(container.begin(), container.end(), pred);   
        }

    template<class InputIterator, class T>
        inline std::vector<int>
        find_all_indices(InputIterator b, InputIterator e, const T &t)      
        {
            return find_all_indices_if(b,e, bind2nd(equal_to<T> (), t));
        }

    template<class Container, class T>
        inline std::vector<int>
        find_all_indices(Container &container, const T& t)
        {
            return find_all_indices(container.begin(), container.end(), t); 
        }

    template<class Container>
        inline typename Container::size_type
        max_element_index(const Container& container)
        {
            typename Container::const_iterator i;
            i = max_element(container);
            return distance(container.begin(), i);
        }   

    template<class Container>
        inline typename Container::size_type
        min_element_index(const Container& container)
        {
            typename Container::const_iterator i, j;
            i = min_element(container);
            typename Container::size_type result = 0;
            j = container.begin();
            for (j = container.begin(); j != i; ++j, ++result);         
            return result;
        }   

    // Map / priority staff 

    template<class Container>
        std::set<typename Container::key_type>
        domain(const Container &container)
        {
            std::set<typename Container::key_type> result;
            for (typename Container::const_iterator i = container.begin();
                 i != container.end(); i++)
                result.insert(i->first);
            return result;
        }

    template<class Container>
        std::set<typename Container::mapped_type>
        range(const Container &container)
        {
            std::set<typename Container::mapped_type> result;
            for (typename Container::const_iterator i = container.begin();
                i != container.end(); i++)
                result.insert(i->second);
            return result;
        }

    /*template <class Key, class T, class Compare, class Allocator>
        std::vector<T>
        get_all(const std::multimap<Key, T, Compare, Allocator> &m, const Key &key)
        {
            std::vector<T> result;
            transform(m.lower_bound(key), m.upper_bound(key), back_inserter(result),
                      take2nd<const Key, T>());
            return result;          
        }

    template<class T>
        std::multimap<int, T>
        priority_multimap(const std::vector<T> &objs, const std::vector<int> &pris)
        {
            multimap<int, T> result;
            transform(pris.begin(), pris.end(), objs.begin(),
                  inserter(result, result.begin()),
                  pair_maker<int, T>());
            return result;
        }

    template<class T>
        std::vector<T>
        order_by_priority(const std::vector<T> &objs, const std::vector<int> &pris)
        {
            multimap<int, T> temp = priority_multimap(objs, pris);
            std::vector<T> result;
            transform(temp, back_inserter(result), take2nd<int, T>());
            return result;
        }


    template<class T>
        std::vector< std::vector<T> >
        same_priorities(const std::vector<T> &objs, const std::vector<int> &pris)
        {
            std::vector< std::vector<T> > result;
    
            multimap<int, T> pm = priority_multimap(objs, pris);
            int minp = pm.begin()->first, maxp = (--pm.end())->first;
            for (int p = minp; p <= maxp; p++) result.push_back(get_all(pm, p));
            return result;
        }
*/
    // order[i] is new index for element at position, i.e.
    // it's current index -> new index mapping
    // If order has duplicated elements, result is undefined
    template<class Container>
        void
        reorder(Container& container, const std::vector<size_t>& order)
        {
            PRECONDITION(container.size() == order.size());

            // Is element with original index i already placed to 
            // proper place as described in order
            std::vector<bool> already_moved(order.size());

            for (size_t i = 0; i < order.size(); ++i) {
            
                // Current is original index of element currently in container[i]
                // if order[current] differs from i we send current to
                // proper position and get new element in container[i]
                // if order[current] is equal to i, then element is already
                // in proper position, and no futher moves can be made

                for(int current = i;
                    !already_moved[current];
                    current = order[current])
                {
                    already_moved[current] = true;

                    // "if" can be simply omitted, with probably a small
                    // performance penalty
                    if (order[current] != i)
                        swap(container[i], container[order[current]]);
                    else break;
                }
            }
        }

    // Similar to reorder, but order is new index -> old index mapping
    template<class Container>
        void
        reorder_bwd(Container& container, const std::vector<size_t>& order)
        {
            std::vector<size_t> fwd_order(order.size());
            for (size_t i = 0; i < order.size(); ++i)
                fwd_order[order[i]] = i;

            reorder(container, fwd_order);
        }



    template<class C>
        struct priority_compare
        {
            priority_compare(const C& p) : p(p) {}

            bool operator()(size_t i1, size_t i2) {
                return p[i1] > p[i2];
            }   
        private:
            const C& p;
        };


    template<class C1, class C2>
        void
        priority_sort(C1& container, const C2& priorities)
        {
            PRECONDITION(container.size() == priorities.size());

            vector<size_t> indices;
            for (size_t i = 0; i < container.size(); ++i) indices.push_back(i);
            priority_compare< C2 > pc(priorities);
            sort(indices.begin(), indices.end(), pc);

            reorder_bwd(container, indices);
        }




    inline
    std::vector<int> natural_sequence(const int n)
    {
        std::vector<int> result;
        for (int i = 0; i < n; i++) result.push_back(i);        
        return result;
    }

#pragma option pop

#endif
