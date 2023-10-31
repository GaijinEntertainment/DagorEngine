/*
 * Dagor Engine
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of use see prog/license.txt)
*/

#ifndef _DAGOR_DAG_VECTOR_H_
#define _DAGOR_DAG_VECTOR_H_
#pragma once


//dag::Vector is (almost) std vector API, but it's maximum container size is dependent on 4th template parameter,
//and it allows unititialized (initialized with new type;) values

//It main differences are:
// * it can call operator new; and not new() instead (it is template parameter and also type trait).
//   That means that it doesn't nesessarily fill scalars/pods with zeroes, like stl/eastl::vector
// if init_constructing = true (that is not default), just new T; will be called
// * it supports realloc for relocatable types (also type_trait, by default - all PODs are) and standard allocator.
//   A lot of memory allocators will do it much faster, than malloc + memcpy + free, and almost all not slower than that anyway
// * if sizeof Allocator is 0, than dag::Vector instance size is 16 in 64-bit platform (and not 24, like eastl/std::vector)
//   it is achieved by storing count and capacity as uint32_t (that is template parameter)
//   that can be beneficial for cache effeciency
// * resize capacity grow policy
//  it allocates at least 16bytes. Most of 64bit mem allocators are basically not capable of allocating less than that
// * push_back grow policy
//  it starts with at least 64 bytes allocation
//  it grows geometrically with a power of 3/2, not 2 like some other containers (including eastl).
//  that is benefical on most memory allocators (see folly::FBVector for explanation)
//Class is EASTL dependent (and can also be made std dependent, if needed, just not interested)
//When NOT to use:
// You need to allocate more than 4billion elements.
// You need exception safety (I hadn't bothered to make it exception-safe, as we don't have exceptions enabled)
//  if you need exceptions - please, contribute, with eastl style (optional exceptions) like EASTL_NOEXCEPT etc

//When to use:
// You need to fit instance in 16/24(empty-size-allocator/allocator-is-pointer) bytes (on x64) or to 12/16 bytes (on x32), or
// You address ([]) or compare for empty() more often then you do iterate, or
// You need to allocate unititialized data (for performance reasons)
// consider the following example:
//   vector<int> data(1000);
//   for (int i = 0; i < 1000; ++i) data[i] = i;//that can be IO read
// it is impossible to do that with std or eastl. Initial allocation will memset data.
// it may be imporrtant for performance reasons.
// by default, dag::Vector does itialized allocation (like vector), but it is template parameter.
//changes/additions in API:
//dag::Vector _is_ a drop-in replacement for eastl/std::vector, but it has some additional API and some changes
//changes:
// * template parameters bool init_constructing, typename Counter. They do have a reasonable defaults
//additions:
// * resize_noinit - resizes without memsetting PODs with zeroes.
// * push_back_noinit - last element will be constructed with placement new, but without () (i.e. for scalars - not initialized)
// * insert_default - inserts uninitialized POD elems (for non-PODS default constructor is called)
// * append_default - append uninitialized POD elems (for non-PODS default constructor is called)
// none of API changes will cause non-PODs (or PODs without using extended API) to be not-initialized!

#include <dag/dag_relocatable.h>
#include <dag/dag_config.h>
#include <EASTL/memory.h>
#include <EASTL/allocator.h>
#include <EASTL/type_traits.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#include <EASTL/initializer_list.h>
#include <EASTL/bonus/compressed_pair.h>

#ifndef DAG_VECTOR_DEFAULT_NAME
  #define DAG_VECTOR_DEFAULT_NAME "dag vector" // Unless the user overrides something, this is "dag vector".
#endif

#ifndef DAG_VECTOR_DEFAULT_ALLOCATOR
  #define DAG_VECTOR_DEFAULT_ALLOCATOR allocator_type(DAG_VECTOR_DEFAULT_NAME)
#endif

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
  #define IF_CONSTEXPR if constexpr
#else
  #define IF_CONSTEXPR if
#endif

namespace dag
{

template <class ForwardIterator, bool init_constructing>
typename eastl::disable_if<init_constructing, void>::type
small_vector_default_fill_n(ForwardIterator first, size_t n)
{
  typedef typename eastl::iterator_traits<ForwardIterator>::value_type value_type;
  ForwardIterator currentDest(first);
  for (; n > 0; --n, ++currentDest)
    ::new (eastl::addressof(*currentDest)) value_type;
}

template <class ForwardIterator, bool init_constructing>
typename eastl::enable_if<init_constructing, void>::type
small_vector_default_fill_n(ForwardIterator first, size_t n)
{
  eastl::uninitialized_default_fill_n(first, n);
}

template <typename T>
struct is_type_relocatable<T, typename eastl::enable_if_t<eastl::is_trivially_copyable_v<T>>> : public eastl::true_type {};

template <typename T, typename A, bool I, typename C> class Vector;
template <typename T, typename A, bool I, typename C>
struct is_type_relocatable<dag::Vector<T, A, I, C>, void> : public eastl::true_type {};

template <typename T,
          typename Allocator = EASTLAllocatorType,
          bool init_constructing = is_type_init_constructing<T>::value,
          typename Counter = uint32_t>
class Vector
{
  public:

  typedef Vector<T, Allocator, init_constructing, Counter> this_type;
  template <typename T2, typename A2, bool i2, typename C2> friend class Vector;

  typedef Allocator                                        allocator_type;
  typedef T                                                value_type;
  typedef T*                                               iterator;
  typedef const T*                                         const_iterator;
  typedef T*                                               pointer;
  typedef const T*                                         const_pointer;
  typedef T&                                               reference;
  typedef const T&                                         const_reference;
  typedef eastl::reverse_iterator<iterator>                reverse_iterator;
  typedef eastl::reverse_iterator<const_iterator>          const_reverse_iterator;
  typedef Counter                                          size_type;
  typedef ptrdiff_t                                        difference_type;

  ~Vector();

  Vector() = default;
  explicit Vector(const allocator_type& allocator) EA_NOEXCEPT : mBeginAndAllocator(NULL, allocator){}
  explicit Vector(size_type n, const allocator_type& allocator = DAG_VECTOR_DEFAULT_ALLOCATOR):
    mBeginAndAllocator(NULL, allocator) { reserve(n); small_vector_default_fill_n<T*, init_constructing>(mpBegin(), used() = n); }
  Vector(this_type&&a) EA_NOEXCEPT {DoSwap(a);}
  Vector(this_type&&x, const allocator_type& allocator) : mBeginAndAllocator(NULL, allocator)
  {
    if (internalAllocator() == x.internalAllocator()) // If allocators are equivalent...
      DoSwap(x);
    else IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      memcpy((void*)(mpBegin() = DoAllocate(used() = allocated() = x.size())), (const void*)x.begin(), (char*)x.end()-(char*)x.begin()); //-V780
      x.used() = 0; //x.set_capacity(0); //<- add this to release memory immediately
    }
    else
    {
      DoInit(eastl::move_iterator<iterator>(x.begin()), eastl::move_iterator<iterator>(x.end()), eastl::is_integral<iterator>());
      x.clear(); //x.set_capacity(0); //<- replace with this to release memory immediately
    }
  }
  Vector(size_type n, const value_type& value, const allocator_type& allocator = DAG_VECTOR_DEFAULT_ALLOCATOR) :
    mBeginAndAllocator(NULL, allocator)
  { resize(n, value); }

  Vector(const this_type& x)
  {
    reserve(x.size());
    eastl::uninitialized_copy_ptr(x.begin(), x.end(), mpBegin());
    used() = x.size();
  }
  Vector(const this_type& x, const allocator_type& allocator):mBeginAndAllocator(NULL, allocator)
  {
    reserve(x.size());
    eastl::uninitialized_copy_ptr(x.begin(), x.end(), mpBegin());
    used() = x.size();
  }

  Vector(std::initializer_list<value_type> ilist, const allocator_type& allocator = DAG_VECTOR_DEFAULT_ALLOCATOR) :
    mBeginAndAllocator(NULL, allocator)
  {
    DoInit(ilist.begin(), ilist.end(), eastl::false_type());
  }

  template <typename InputIterator>
  Vector(InputIterator first, InputIterator last, const allocator_type& allocator = DAG_VECTOR_DEFAULT_ALLOCATOR) :
    mBeginAndAllocator(NULL, allocator)
  {
    DoInit(first, last, eastl::is_integral<InputIterator>());
  }

  template <typename A2>
    explicit Vector(Vector<value_type, A2, init_constructing, size_type> &&x, const allocator_type &a) : mBeginAndAllocator(NULL, a)
  {
    IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      memcpy((void*)(mpBegin() = DoAllocate(used() = allocated() = x.size())), (const void*)x.begin(), (char*)x.end()-(char*)x.begin()); //-V780
      x.used() = 0; //x.set_capacity(0); //<- add this to release memory immediately
    }
    else
    {
      DoInit(eastl::move_iterator<iterator>(x.begin()), eastl::move_iterator<iterator>(x.end()), eastl::is_integral<iterator>());
      x.clear(); //x.set_capacity(0); //<- replace with this to release memory immediately
    }
  }

  template<typename V, typename U=T, typename CB=decltype(&V::cbegin), typename CE=decltype(&V::cend),
           typename B=typename eastl::enable_if<eastl::is_same<typename V::value_type const, U const>::value, bool>::type,
           typename D=eastl::disable_if_t<eastl::is_base_of_v<this_type, V>>>
    explicit Vector(const V &v, const allocator_type &a = DAG_VECTOR_DEFAULT_ALLOCATOR): Vector(v.cbegin(), v.cend(), a) {}

  template<typename V, typename U=T, typename CB=decltype(&V::cbegin), typename CE=decltype(&V::cend),
           typename B=typename eastl::enable_if<eastl::is_same<typename V::value_type const, U const>::value, bool>::type,
           typename D=eastl::disable_if_t<eastl::is_base_of_v<this_type, V>>>
    Vector& operator=(const V &v) { assign(v.cbegin(), v.cend()); return *this; }

  this_type& operator=(const this_type& x)
  {
    if (this != &x)
    {
      typedef typename eastl::iterator_traits<const_iterator>::iterator_category IC;
      DoAssignFromIterator<const_iterator>(x.begin(), x.end(), IC());
    }
    return *this;
  }
  void assign(size_type n, const value_type& value) { DoAssignValues(n, value); }
  this_type& operator=(this_type&& x) EA_NOEXCEPT
  {
    DoSwap(x);
    return *this;
  }

  void swap(this_type& x) { DoSwap(x);}

  template <typename InputIterator>
  void assign(InputIterator first, InputIterator last){DoAssign<InputIterator>(first, last, eastl::is_integral<InputIterator>());}
  inline void assign(std::initializer_list<value_type> ilist)
  {
    typedef typename std::initializer_list<value_type>::iterator InputIterator;
    typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
    DoAssignFromIterator<InputIterator>(ilist.begin(), ilist.end(), IC()); // initializer_list has const elements and so we can't move from them.
  }

  this_type& operator=(std::initializer_list<value_type> ilist)
  {
    typedef typename std::initializer_list<value_type>::iterator InputIterator;
    typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
    DoAssignFromIterator<InputIterator>(ilist.begin(), ilist.end(), IC()); // initializer_list has const elements and so we can't move from them.
    return *this;
  }

  iterator       begin() EA_NOEXCEPT{return mpBegin();}
  const_iterator begin() const EA_NOEXCEPT{return mpBegin();}
  const_iterator cbegin() const EA_NOEXCEPT{return mpBegin();}

  iterator       end() EA_NOEXCEPT{return mpBegin() + used();}
  const_iterator end() const EA_NOEXCEPT{return mpBegin() + used();}
  const_iterator cend() const EA_NOEXCEPT{return mpBegin() + used();}

  reverse_iterator       rbegin() EA_NOEXCEPT{return reverse_iterator(end());}
  const_reverse_iterator rbegin() const EA_NOEXCEPT{return const_reverse_iterator(end());}
  const_reverse_iterator crbegin() const EA_NOEXCEPT{return const_reverse_iterator(cend());}

  reverse_iterator       rend() EA_NOEXCEPT{return reverse_iterator(begin());}
  const_reverse_iterator rend() const EA_NOEXCEPT{return const_reverse_iterator(begin());}
  const_reverse_iterator crend() const EA_NOEXCEPT{return const_reverse_iterator(cbegin());}

  bool      empty() const EA_NOEXCEPT{return size() == 0;}
  size_type size() const EA_NOEXCEPT{return used();}
  size_type capacity() const EA_NOEXCEPT{return allocated();}

  void resize(size_type n, const value_type& value)
  {
    if (n > used())  // We expect that more often than not, resizes will be upsizes.
    {
      if (n > allocated())
        DoGrow(eastl::max(GetNewCapacity(used()), n));
      eastl::uninitialized_fill_n_ptr(mpBegin() + used(), n-used(), value);
    } else
      eastl::destruct(mpBegin() + n, end());
    used() = n;
  }

  void resize_noinit(size_type n)
  {
    if (n > used())  // We expect that more often than not, resizes will be upsizes.
    {
      if (n > allocated())
        DoGrow(eastl::max(GetNewCapacity(used()), n));
      small_vector_default_fill_n<T*, false>(mpBegin() + used(), n-used());
    } else
      eastl::destruct(mpBegin() + n, end());
    used() = n;
  }

  void resize(size_type n)
  {
    if (n > used())  // We expect that more often than not, resizes will be upsizes.
    {
      if (n > allocated())
        DoGrow(eastl::max(GetNewCapacity(used()), n));
      small_vector_default_fill_n<T*, init_constructing>(mpBegin() + used(), n-used());
    }
    else
      eastl::destruct(mpBegin() + n, end());
    used() = n;
  }

  void set_capacity(size_type n)
  {
    if (n <= used()) // If new capacity <= size...
    {
      if (n == 0)  // Very often n will be 0, and clear will be faster than resize and use less stack space.
        clear();
      else
        resize(n);
      shrink_to_fit();
    }
    else // Else new capacity > size.
    {
      DoGrow(n);
    }
  }
  void reserve(size_type n)
  {
    if (n > allocated()) // If n > capacity ...
      DoGrow(n);
  }

  void shrink_to_fit()
  {
    if (allocated() != used())
    {
      if (!used())
        return this_type(get_allocator()).swap(*this);

      size_type a = used();
      if (usingStdAllocator ? DoStdExpand<value_type, usingStdAllocator>(a)
                            : DoExpand<value_type, HasExpand<allocator_type>::value>(a))
      {
        allocated() = a;
        return;
      }

      IF_CONSTEXPR(is_type_relocatable<value_type>::value)
      {
        if (usingStdAllocator ? DoStdRealloc<value_type, usingStdAllocator>(a)
                              : DoRealloc<value_type, HasRealloc<allocator_type>::value>(a))
          allocated() = a;
      }
      else
      {
        this_type temp = this_type(eastl::move_iterator<iterator>(begin()), eastl::move_iterator<iterator>(end()), get_allocator());
        DoSwap(temp);
      }
    }
  }

  pointer       data() EA_NOEXCEPT{return mpBegin();}
  const_pointer data() const EA_NOEXCEPT{return mpBegin();}

  reference       operator[](size_type n);
  const_reference operator[](size_type n) const;

  reference       at(size_type n){return this->operator[](n);}//no exception handling
  const_reference at(size_type n) const{return this->operator[](n);}//no exception handling

  reference       front();
  const_reference front() const;

  reference       back();
  const_reference back() const;

  reference push_back(const value_type& value);
  reference push_back();
  reference push_back_noinit();
  void*     push_back_uninitialized();
  reference push_back(value_type&& value);
  void      pop_back();

  template<class... Args>
  iterator emplace(const_iterator position, Args&&... args);

  template<class... Args>
  reference emplace_back(Args&&... args);

  iterator insert(const_iterator position, const value_type& value);
  iterator insert(const_iterator position, size_type n, const value_type& value);
  iterator insert(const_iterator position, value_type&& value);
  iterator insert(const_iterator position, std::initializer_list<value_type> ilist);

  iterator insert_default(const_iterator position, size_type n);
  iterator append_default(size_type n);

  template <typename InputIterator>
  iterator insert(const_iterator position, InputIterator first, InputIterator last);

  iterator erase(const_iterator position);
  iterator erase(const_iterator first, const_iterator last);
  iterator erase_unsorted(const_iterator position);         // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.

  reverse_iterator erase(const_reverse_iterator position);
  reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);
  reverse_iterator erase_unsorted(const_reverse_iterator position);

  void clear() EA_NOEXCEPT;

  const allocator_type&  get_allocator() const EA_NOEXCEPT { return internalAllocator(); }
  allocator_type&  get_allocator() EA_NOEXCEPT { return internalAllocator(); }

protected:
  static constexpr size_t min_allocate_size_bytes = 16; // None of allocators can allocate less than 16 bytes
  static constexpr size_t min_capacity_size_bytes = min_allocate_size_bytes; // To consider: use something like nallocx or malloc_good_size
  size_t GetNewCapacitySizeT(size_t currentCapacity) const
  {
    // This needs to return a value of at least currentCapacity and at least 1.
    if (currentCapacity == 0)
      return eastl::max(min_capacity_size_bytes/sizeof(value_type), (size_t)1);//allocate
    if (currentCapacity < DAG_STD_EXPAND_MIN_SIZE / sizeof(value_type))
      return currentCapacity * 2;
    return size_t(currentCapacity + (currentCapacity + 1) / 2); // x1.5 growth
  }
  size_type GetNewCapacity(size_type currentCapacity) const
  {
    size_t newCapacity = GetNewCapacitySizeT(size_t(currentCapacity));
    IF_CONSTEXPR(sizeof(size_type) < 4)
      return size_type(eastl::min((size_t)eastl::numeric_limits<size_type>::max(), newCapacity));
    return size_type(newCapacity);
  }

  template <typename Integer>
  void DoInit(Integer n, Integer value, eastl::true_type);

  template <typename InputIterator>
  void DoInit(InputIterator first, InputIterator last, eastl::false_type);

  template <typename InputIterator>
  void DoInitFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag);

  template <typename ForwardIterator>
  void DoInitFromIterator(ForwardIterator first, ForwardIterator last, EASTL_ITC_NS::forward_iterator_tag);

  template<typename... Args>
  void DoInsertValue(const_iterator position, Args&&... args)
  {
    #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < begin()) || (position > end())))
      DAG_ASSERTF(0, "Vector::insert/emplace -- invalid position");
    #endif

    // C++11 stipulates that position is const_iterator, but the return value is iterator.
    iterator destPosition = const_cast<value_type*>(position);

    if (used() == allocated())  // if we need to expand our capacity
    {
      size_type nNewCap = GetNewCapacity(used()), nDestPos = destPosition - mpBegin();
      if (DoReallocate(nNewCap))
      {
        destPosition = mpBegin() + nDestPos;
        allocated() = nNewCap;
      }
      else
      {
        pointer pNewData = DoAllocate(nNewCap);
        ::new((void*)(pNewData + nDestPos)) value_type(eastl::forward<Args>(args)...); // Because the old data is potentially being moved rather than copied, we need to move
        IF_CONSTEXPR(CanRealloc())
        {
          // If realloc code path exist the only valid way we can get here is insert to the end of empty container
          // (as otherwise realloc code path would have been taken)
          DAG_ASSERTF(empty() && mpBegin() == destPosition, "");
        }
        else IF_CONSTEXPR(is_type_relocatable<value_type>::value)
        {
          if (nDestPos)
            memcpy((void*)pNewData, (const void*)mpBegin(), sizeof(value_type) * nDestPos);
          if (size_t sz = (char*)end()-(char*)destPosition)
            memcpy((void*)(pNewData + nDestPos + 1), (const void*)destPosition, sz); //-V780
        }
        else
        {
          pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin(), destPosition, pNewData), mpEnd = end();
          eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, ++pNewEnd);
          eastl::destruct(mpBegin(), mpEnd);
        }

        DoFree(mpBegin(), allocated());
        mpBegin()    = pNewData;
        ++used();
        allocated() = nNewCap;

        return;
      }
    }

    value_type value(eastl::forward<Args>(args)...); // Need to do this before the move_backward below because maybe args refers to something within the moving range.
    pointer mpEnd = end();

    if (mpEnd != destPosition)
    {
      IF_CONSTEXPR(is_type_relocatable<value_type>::value)
      {
        memmove((void*)(destPosition+1), (void*)destPosition, (char*)mpEnd - (char*)destPosition);
      }
      else
      {
        ::new(static_cast<void*>(mpEnd)) value_type(eastl::move(*(mpEnd - 1))); // mpEnd is uninitialized memory, so we must construct into it instead of move into it like we do with the other elements below.
        eastl::move_backward(destPosition, mpEnd - 1, mpEnd);           // We need to go backward because of potential overlap issues.
        eastl::destruct(destPosition);
      }
    }

    ::new(static_cast<void*>(destPosition)) value_type(eastl::move(value)); // Move the value argument to the given position.

    ++used();
  }

  template<bool init_constr, typename VT>
  static inline typename eastl::disable_if<init_constr, VT>::type * Construct(void *p) { return ::new(p) VT; }

  template<bool init_constr, typename VT>
  static inline typename eastl::enable_if<init_constr, VT>::type * Construct(void *p) { return ::new(p) VT();}

  void DoInsertValues(const_iterator position, size_type n, const value_type& value)
  {
    #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < begin()) || (position > end())))
      DAG_ASSERTF(0, "Vector::insert -- invalid position");
    #endif

    // C++11 stipulates that position is const_iterator, but the return value is iterator.
    iterator destPosition = const_cast<value_type*>(position);

    if (n + used() > allocated())  // if we need to expand our capacity
    {
      size_type nNewCap = eastl::max(used() + n, GetNewCapacity(used())), nDestPos = destPosition - mpBegin();
      if (DoReallocate(nNewCap))
      {
        destPosition = mpBegin() + nDestPos;
        allocated() = nNewCap;
      }
      else
      {
        pointer pNewData = DoAllocate(nNewCap);
        IF_CONSTEXPR(CanRealloc())
        {
          // If realloc code path exist the only valid way we can get here is insert to the end of empty container
          // (as otherwise realloc code path would have been taken)
          DAG_ASSERTF(empty() && mpBegin() == destPosition, "");
          eastl::uninitialized_fill_n_ptr(pNewData, n, value);
        }
        else IF_CONSTEXPR(is_type_relocatable<value_type>::value)
        {
          if (nDestPos)
            memcpy((void*)pNewData, (const void*)mpBegin(), sizeof(value_type) * nDestPos);
          T *pDestPosition = pNewData + nDestPos;
          eastl::uninitialized_fill_n_ptr(pDestPosition, n, value);
          if (size_t sz = (char*)end()-(char*)destPosition)
            memcpy((void*)(pDestPosition + n), (const void*)destPosition, sz); //-V780
        }
        else
        {
          pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin(), destPosition, pNewData), mpEnd = end();
          eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
          pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd + n);
          eastl::destruct(mpBegin(), mpEnd);
        }

        DoFree(mpBegin(), allocated());
        mpBegin()    = pNewData;
        used()      += n;
        allocated() = nNewCap;

        return;
      }
    }

    const value_type temp  = value;
    pointer mpEnd = end();
    const size_type nExtra = static_cast<size_type>(mpEnd - destPosition);

    IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      if (nExtra)
        memmove((void*)(destPosition+n), (void*)destPosition, (char*)mpEnd-(char*)destPosition);
      eastl::uninitialized_fill_n_ptr(destPosition, n, temp);
    }
    else if (n < nExtra)
    {
      eastl::uninitialized_move_ptr(mpEnd - n, mpEnd, mpEnd);
      eastl::move_backward(destPosition, mpEnd - n, mpEnd); // We need move_backward because of potential overlap issues.
      eastl::fill(destPosition, destPosition + n, temp);
    }
    else
    {
      eastl::uninitialized_fill_n_ptr(mpEnd, n - nExtra, temp);
      eastl::uninitialized_move_ptr(destPosition, mpEnd, mpEnd + n - nExtra);
      eastl::fill(destPosition, mpEnd, temp);
    }

    used() += n;
  }

  void DoInsertValues(const_iterator position, size_type n)
  {
    #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < begin()) || (position > end())))
      DAG_ASSERTF(0, "Vector::insert -- invalid position");
    #endif

    // C++11 stipulates that position is const_iterator, but the return value is iterator.
    iterator destPosition = const_cast<value_type*>(position);

    if (n + used() > allocated())  // if we need to expand our capacity
    {
      size_type nNewCap = eastl::max(used() + n, GetNewCapacity(used())), nDestPos = destPosition - mpBegin();
      if (DoReallocate(nNewCap))
      {
        destPosition = mpBegin() + nDestPos;
        allocated() = nNewCap;
      }
      else
      {
        pointer pNewData = DoAllocate(nNewCap);
        IF_CONSTEXPR(CanRealloc())
        {
          // If realloc code path exist the only valid way we can get here is insert to the end of empty container
          // (as otherwise realloc code path would have been taken)
          DAG_ASSERTF(empty() && mpBegin() == destPosition, "");
          small_vector_default_fill_n<T*, init_constructing>(pNewData, n);
        }
        else IF_CONSTEXPR(is_type_relocatable<value_type>::value)
        {
          if (nDestPos)
            memcpy((void*)pNewData, (const void*)mpBegin(), sizeof(value_type) * nDestPos);
          T *pDestPosition = pNewData + nDestPos;
          small_vector_default_fill_n<T*, init_constructing>(pDestPosition, n);
          if (size_t sz = (char*)end()-(char*)destPosition)
            memcpy((void*)(pDestPosition + n), (const void*)destPosition, sz); //-V780
        }
        else
        {
          pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin(), destPosition, pNewData), mpEnd = end();
          small_vector_default_fill_n<T*, init_constructing>(pNewEnd, n);
          pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd + n);
          eastl::destruct(mpBegin(), mpEnd);
        }

        DoFree(mpBegin(), allocated());
        mpBegin()    = pNewData;
        used()      += n;
        allocated() = nNewCap;

        return;
      }
    }

    pointer mpEnd = end();
    const size_type nExtra = static_cast<size_type>(mpEnd - destPosition);

    IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      if (nExtra)
        memmove((void*)(destPosition+n), (void*)destPosition, (char*)mpEnd-(char*)destPosition);
      small_vector_default_fill_n<T*, init_constructing>(destPosition, n);
    }
    else if (n < nExtra)
    {
      eastl::uninitialized_move_ptr(mpEnd - n, mpEnd, mpEnd);
      eastl::move_backward(destPosition, mpEnd - n, mpEnd); // We need move_backward because of potential overlap issues.
      eastl::destruct(destPosition, destPosition + n);
      small_vector_default_fill_n<T*, init_constructing>(destPosition, n);
    }
    else
    {
      small_vector_default_fill_n<T*, init_constructing>(mpEnd, n - nExtra);
      eastl::uninitialized_move_ptr(destPosition, mpEnd, mpEnd + n - nExtra);
      eastl::destruct(destPosition, mpEnd);
      small_vector_default_fill_n<T*, init_constructing>(destPosition, mpEnd - destPosition);
    }

    used() += n;
  }

  void DoInsertValuesEnd(size_type n, const value_type& value)
  {
    pointer mpEnd = end();
    if (n + used() > allocated())
      DoGrow(eastl::max(GetNewCapacity(used()), used() + n));
    eastl::uninitialized_fill_n_ptr(mpEnd, n, value);
    used() += n;
  }

  template<bool initialize_construct>
  void DoInsertValuesEnd(size_type n)
  {
    if (n + used() > allocated())
      DoGrow(eastl::max(GetNewCapacity(used()), used() + n));
    small_vector_default_fill_n<T*, initialize_construct>(mpBegin() + used(), n);
    used()      += n;
  }

  template <typename InputIterator>
  void DoAssignFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
  {
    iterator position(mpBegin());
    auto mpEnd = end();

    while ((position != mpEnd) && (first != last))
    {
      *position = *first;
      ++first;
      ++position;
    }
    if (first == last)
      erase(position, mpEnd);
    else
      insert(mpEnd, first, last);
  }

  template <typename RandomAccessIterator>
  void DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag)
  {
    const size_type n = (size_type)eastl::distance(first, last);
    if (n > allocated()) // If n > capacity ...
    {
      size_type a = n;
      if (!mpBegin() || !(usingStdAllocator ? DoStdExpand<value_type, usingStdAllocator>(a) :
                                              DoExpand<value_type, HasExpand<allocator_type>::value>(a)))
      {
        eastl::destruct(mpBegin(), end());
        DoFree(mpBegin(), allocated());
        pointer const pNewData = DoAllocate(a);
        eastl::uninitialized_copy_ptr(first, last, pNewData);
        mpBegin() = pNewData;
        allocated() = a;
        used() = n;
        return;
      }
      else
        allocated() = a;
    }
    IF_CONSTEXPR(eastl::is_trivially_copyable_v<value_type>) // Trivially copyable class implies trivial destructor
      memcpy(mpBegin(), first, (char*)last - (char*)first); //-V780
    else if (n <= size())
    {
      eastl::copy(first, last, mpBegin());
      eastl::destruct(mpBegin() + n, end());
    }
    else // else size < n <= capacity
    {
      RandomAccessIterator pos = first + size();
      eastl::copy(first, pos, mpBegin());
      eastl::uninitialized_copy_ptr(pos, last, end());
    }
    used() = n;
  }

  template <typename Integer>
  inline void DoInsert(const_iterator position, Integer n, Integer value, eastl::true_type)
  {
    DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
  }


  template <typename InputIterator>
  inline void DoInsert(const_iterator position, InputIterator first, InputIterator last, eastl::false_type)
  {
    typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
    DoInsertFromIterator(position, first, last, IC());
  }


  template <typename InputIterator>
  inline void DoInsertFromIterator(const_iterator position, InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
  {
    for (; first != last; ++first, ++position)
      position = insert(position, *first);
  }

  template <typename BidirectionalIterator>
  void DoInsertFromIterator(const_iterator position, BidirectionalIterator first, BidirectionalIterator last,
                            EASTL_ITC_NS::bidirectional_iterator_tag)
  {
    #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < mpBegin()) || (position > end())))
      DAG_ASSERTF(0, "vector::insert -- invalid position");
    #endif

    // C++11 stipulates that position is const_iterator, but the return value is iterator.
    iterator destPosition = const_cast<value_type*>(position);

    const size_type n = (size_type)eastl::distance(first, last);
    if ((used() + n) > allocated()) // if we need to expand our capacity.
    {
      size_type nNewSize = eastl::max(used() + n, GetNewCapacity(used())),
                nDestPos = destPosition - mpBegin();
      if (DoReallocate(nNewSize))
      {
        destPosition = mpBegin() + nDestPos;
        allocated() = nNewSize;
      }
      else
      {
        pointer pNewData = DoAllocate(nNewSize), pNewEnd = pNewData;
        IF_CONSTEXPR(CanRealloc())
        {
          // If realloc code path exist the only valid way we can get here is insert to the end of empty container
          // (as otherwise realloc code path would have been taken)
          DAG_ASSERTF(empty() && mpBegin() == destPosition, "");
          eastl::uninitialized_copy_ptr(first, last, pNewEnd);
        }
        else
        {
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
          static_assert(!is_type_relocatable<value_type>::value, "should not reach here, expecting CanRealloc()==true");
#endif
#if EASTL_EXCEPTIONS_ENABLED
          try
          {
#endif
            pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin(), destPosition, pNewData);
            pNewEnd = eastl::uninitialized_copy_ptr(first, last, pNewEnd);
            pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(destPosition, end(), pNewEnd);
#if EASTL_EXCEPTIONS_ENABLED
          }
          catch(...)
          {
            eastl::destruct(pNewData, pNewEnd);
            DoFree(pNewData, nNewSize);
            throw;
          }
#endif
          eastl::destruct(mpBegin(), end());
        }

        DoFree(mpBegin(), allocated());
        mpBegin() = pNewData;
        allocated() = nNewSize;
        used() += n;

        return;
      }
    }
    // n fits within existing capacity
    {
      iterator mpEnd = end();
      const size_type nExtra = static_cast<size_type>(mpEnd - destPosition);
      IF_CONSTEXPR(is_type_relocatable<value_type>::value)
      {
        if (mpEnd != destPosition) // branch for append-like inserts
          memmove(destPosition + n, destPosition, (char*)mpEnd - (char*)destPosition); //-V780
        eastl::uninitialized_copy_ptr(first, last, destPosition);
      }
      else if (n < nExtra) // If the inserted values are entirely within initialized memory (i.e. are before mpEnd)...
      {
        eastl::uninitialized_move_ptr(mpEnd - n, mpEnd, mpEnd);
        eastl::move_backward(destPosition, mpEnd - n, mpEnd); // We need move_backward because of potential overlap issues.
        eastl::copy(first, last, destPosition);
      }
      else
      {
        BidirectionalIterator iTemp = first;
        eastl::advance(iTemp, nExtra);
        eastl::uninitialized_copy_ptr(iTemp, last, mpEnd);
        eastl::uninitialized_move_ptr(destPosition, mpEnd, mpEnd + n - nExtra);
        eastl::copy_backward(first, iTemp, destPosition + nExtra);
      }
      used() += n;
    }
  }

  void DoAssignValues(size_type n, const value_type& value)
  {
    if (n > allocated()) // If n > capacity ...
    {
      size_type a = n;
      if (!DoReallocate(a))
      {
        this_type temp(n, value, internalAllocator()); // We have little choice but to reallocate with new memory.
        swap(temp);
      } else
      {
        size_t cnt = used();
        eastl::fill_n(mpBegin(), cnt, value);
        eastl::uninitialized_fill_n_ptr(end(), n - cnt, value);
        used() = n;
        allocated() = a;
      }
    }
    else if (n > used()) // If n > size ...
    {
      auto mpEnd = end();
      eastl::fill(mpBegin(), mpEnd, value);
      eastl::uninitialized_fill_n_ptr(mpEnd, n - size_type(mpEnd - mpBegin()), value);
      used() = n;
    }
    else // else 0 <= n <= size
    {
      eastl::fill_n(mpBegin(), n, value);
      erase(mpBegin() + n, end());
    }
  }
  template <typename Integer>
  inline void DoAssign(Integer n, Integer value, eastl::true_type)
  {
    DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
  }


  template <typename InputIterator>
  inline void DoAssign(InputIterator first, InputIterator last, eastl::false_type)
  {
    typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
    DoAssignFromIterator<InputIterator>(first, last, IC());
  }

  inline void DoSwap(this_type& x)
  {
    eastl::swap(mBeginAndAllocator, x.mBeginAndAllocator);
    eastl::swap(used(),    x.used());
    eastl::swap(allocated(), x.allocated());
  }
  void DoGrow(size_type n)
  {
    if (!DoReallocate(n))
    {
      pointer const pNewData = DoAllocate(n);
      pointer mpEnd = end();
      IF_CONSTEXPR(is_type_relocatable<value_type>::value)
        memcpy((void*)pNewData, (const void*)mpBegin(), (char*)mpEnd-(char*)mpBegin());
      else
      {
        eastl::uninitialized_move_ptr_if_noexcept(mpBegin(), mpEnd, pNewData);
        eastl::destruct(mpBegin(), mpEnd);
      }

      DoFree(mpBegin(), allocated());
      mpBegin()    = pNewData;
    }

    allocated() = n;
  }

  eastl::compressed_pair<T *, allocator_type> mBeginAndAllocator{nullptr};
  size_type mCount=0, mAllocated=0;

  iterator & mpBegin() EA_NOEXCEPT { return mBeginAndAllocator.first(); }
  iterator mpBegin() const EA_NOEXCEPT { return mBeginAndAllocator.first(); }

  size_type & allocated() EA_NOEXCEPT { return mAllocated; }
  size_type allocated() const EA_NOEXCEPT { return mAllocated; }

  size_type & used() EA_NOEXCEPT { return mCount; } //-V1071 The return value is not always used.
  size_type used() const EA_NOEXCEPT { return mCount; }

  allocator_type&  internalAllocator() EA_NOEXCEPT { return mBeginAndAllocator.second(); }
  const allocator_type&  internalAllocator() const EA_NOEXCEPT { return mBeginAndAllocator.second(); }


  template <typename AT>
  class HasRealloc
  {
    // Note: intentionally without exact signature match in order catch incompatible declarations of 'requestResources' in compile-time
    template<typename U> static eastl::yes_type SFINAE(decltype(&U::realloc));
    template<typename U> static eastl::no_type SFINAE(...);
  public:
    static constexpr bool value = sizeof(SFINAE<AT>(nullptr)) == sizeof(eastl::yes_type);
  };
  template<typename AT, typename = const size_t> struct get_min_expand_size
   { static constexpr size_t min_expand_size = DAG_STD_EXPAND_MIN_SIZE; };

  template <typename AT> struct get_min_expand_size<AT, decltype(AT::min_expand_size)>
   { static constexpr size_t min_expand_size = AT::min_expand_size; };

  template <typename AT>
  class HasExpand
  {
    // Note: intentionally without exact signature match in order catch incompatible declarations of 'requestResources' in compile-time
    template<typename U> static eastl::yes_type SFINAE(decltype(&U::resizeInplace));
    template<typename U> static eastl::no_type SFINAE(...);
  public:
    static constexpr bool value = sizeof(SFINAE<AT>(nullptr)) == sizeof(eastl::yes_type);
    static constexpr size_t min_size = value ? get_min_expand_size<AT>::min_expand_size : size_t(~size_t(0));
  };

  static constexpr bool CanRealloc()
  {
    return (alignof(value_type) <= EA_PLATFORM_MIN_MALLOC_ALIGNMENT) &&
           is_type_relocatable<value_type>::value &&
           (usingStdAllocator || HasRealloc<allocator_type>::value);
  }

  value_type* DoReallocate(size_type &n)
  {
    if (!mpBegin())
      return nullptr;
    IF_CONSTEXPR(alignof(value_type) <= EA_PLATFORM_MIN_MALLOC_ALIGNMENT && is_type_relocatable<value_type>::value)
    {
      return usingStdAllocator ?
        DoStdRealloc<value_type, usingStdAllocator>(n) :
        DoRealloc<value_type, HasRealloc<allocator_type>::value>(n);
    }
    return usingStdAllocator ?
      DoStdExpand<value_type, usingStdAllocator>(n) :
      DoExpand<value_type, HasExpand<allocator_type>::value>(n);
  }
  template <typename VT, bool realloc_allowed>
  typename eastl::enable_if<realloc_allowed, VT*>::type
  DoRealloc(size_type &n)
  {
    n = eastl::max(size_type(min_allocate_size_bytes/sizeof(value_type)), n);
    VT * newBegin = (VT * )internalAllocator().realloc(mpBegin(), size_t(n)*sizeof(VT));
    return newBegin ? (mpBegin() = newBegin) : nullptr;
  }

  template <typename VT, bool allowed> typename eastl::disable_if<allowed, VT*>::type
  DoRealloc(size_type &){return nullptr;}

  template <typename VT, bool allowed>
  typename eastl::enable_if<allowed, VT*>::type
  DoExpand(size_type &n)
  {
    const size_t sz = size_t(n)*sizeof(VT);
    if (sz < HasExpand<allocator_type>::min_size)// cant expand small blocks
      return nullptr;
    VT * v = mpBegin();
    return internalAllocator().resizeInplace(v, sz) ? v : nullptr;
  }

  template <typename VT, bool allowed> typename eastl::disable_if<allowed, VT*>::type
  DoExpand(size_type &){return nullptr;}

  static constexpr bool usingStdAllocator = eastl::is_same_v<Allocator, EASTLAllocatorType>;

  template <typename VT, bool allowed>
  typename eastl::enable_if<allowed, VT*>::type
  DoStdExpand(size_type &n)
  {
    const size_t sz = size_t(n)*sizeof(VT);
    if (EASTL_LIKELY(sz < DAG_STD_EXPAND_MIN_SIZE))// cant expand small blocks
      return nullptr;
    VT * v = mpBegin();
    return DAG_RESIZE_IN_PLACE(v, sz) ? v : nullptr;
  }

  template <typename VT, bool allowed> typename eastl::disable_if<allowed, VT*>::type DoStdExpand(size_type &) { return nullptr; }


  template <typename VT, bool allowed>
  typename eastl::enable_if<allowed, VT*>::type
  DoStdRealloc(size_type &n)
  {
    n = eastl::max(size_type(min_allocate_size_bytes/sizeof(value_type)), n);
    VT* newBegin = (VT*)DAG_REALLOC(mpBegin(), size_t(n)*sizeof(T));
    return newBegin ? (mpBegin()=newBegin) : nullptr;
  }

  template <typename VT, bool allowed> typename eastl::disable_if<allowed, VT*>::type DoStdRealloc(size_type &) { return nullptr; }

  T* DoAllocate(size_type &n)
  {
    #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY(n >= eastl::numeric_limits<size_type>::max()))
      DAG_ASSERTF(0, "Vector::DoAllocate -- improbably large request.");
    #endif
    n = eastl::max(size_type(min_allocate_size_bytes/sizeof(value_type)), n);

    auto* p = (T*)eastl::allocate_memory(internalAllocator(), size_t(n) * sizeof(T), EASTL_ALIGN_OF(T), 0);
    EASTL_ASSERT_MSG(p != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");
    return p;
  }
  void DoFree(T* p, size_type n)
  {
    if (p)
       EASTLFree(internalAllocator(), p, n * sizeof(T));
  }
};

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline void Vector<T, Allocator, init_constructing, Counter>::clear() EA_NOEXCEPT
{
  eastl::destruct(begin(), end());
  used() = 0;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline Vector<T, Allocator, init_constructing, Counter>::~Vector()
{
  if (mpBegin())
  {
    eastl::destruct(begin(), end());
    DoFree(begin(), allocated());
  }
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reference
Vector<T, Allocator, init_constructing, Counter>::operator[](size_type n)
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(n >= size()))
    DAG_ASSERTF(0, "Vector::at n=%d num=%d, sizeof(T)=%d", n, size(), sizeof(T));
  #endif

  return *(mpBegin() + n);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::const_reference
Vector<T, Allocator, init_constructing, Counter>::operator[](size_type n) const
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(n >= size()))
    DAG_ASSERTF(0, "Vector::at n=%d num=%d, sizeof(T)=%d", n, size(), sizeof(T));
  #endif

  return *(mpBegin() + n);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reference
Vector<T, Allocator, init_constructing, Counter>::front()
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(size()<=0))
    DAG_ASSERTF(0, "Vector::front on empty sizeof(T)=%d", sizeof(T));
  #endif

  return *mpBegin();
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::const_reference
Vector<T, Allocator, init_constructing, Counter>::front() const
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(size()<=0))
    DAG_ASSERTF(0, "Vector::front on empty sizeof(T)=%d", sizeof(T));
  #endif

  return *mpBegin();
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reference
Vector<T, Allocator, init_constructing, Counter>::back()
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(size()<=0))
    DAG_ASSERTF(0, "Vector::back on empty sizeof(T)=%d", sizeof(T));
  #endif

  return *(mpBegin() + used()-1);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::const_reference
Vector<T, Allocator, init_constructing, Counter>::back() const
{
  #if EASTL_ASSERT_ENABLED
  if (EASTL_UNLIKELY(size()<=0))
    DAG_ASSERTF(0, "Vector::back on empty sizeof(T)=%d", sizeof(T));
  #endif

  return *(mpBegin() + used()-1);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline T& Vector<T, Allocator, init_constructing, Counter>::push_back(const value_type& value)
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  return *::new((void*)(mpBegin() + (used()++))) value_type(value);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline T& Vector<T, Allocator, init_constructing, Counter>::push_back(value_type&& value)
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  return *::new((void*)(mpBegin() + (used()++))) value_type(eastl::move(value));
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline T&
Vector<T, Allocator, init_constructing, Counter>::push_back()
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  return *Construct<init_constructing, T>(mpBegin() + (used()++));
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline T& Vector<T, Allocator, init_constructing, Counter>::push_back_noinit()
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  return *Construct<false, T>(mpBegin() + (used()++));
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline void* Vector<T, Allocator, init_constructing, Counter>::push_back_uninitialized()
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  return (void*)(mpBegin() + (used()++));
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline void Vector<T, Allocator, init_constructing, Counter>::pop_back()
{
  #if EASTL_ASSERT_ENABLED
  DAG_ASSERTF(size()>0, "Vector::erase on empty");
  #endif
  --used();
  end()->~value_type();
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
template<class... Args>
inline typename Vector<T, Allocator, init_constructing, Counter>::reference
Vector<T, Allocator, init_constructing, Counter>::emplace_back(Args&&... args)
{
  if (EASTL_UNLIKELY(allocated() == used()))
    DoGrow(GetNewCapacity(used()));
  ::new((void*)(mpBegin() + used())) value_type(eastl::forward<Args>(args)...);
  ++used();
  return back();
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
template<class... Args>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::emplace(const_iterator position, Args&&... args)
{
  const intptr_t n = position - mpBegin(); // Save this because we might reallocate.

  if (n != used())
    DoInsertValue(position, eastl::forward<Args>(args)...);//not implemented
  else
  {
    if (used() == allocated())
      DoGrow(GetNewCapacity(used()));
    ::new((void*)(mpBegin()+used())) value_type(eastl::forward<Args>(args)...);
    used()++;
  }

  return mpBegin() + n;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::erase(const_iterator position)
{
  #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < begin()) || (position >= end())))
      DAG_ASSERTF(0, "Vector::erase on invalid position %d (T)=%d", position-cbegin(), sizeof(T));
  #endif
  iterator destPosition = const_cast<value_type*>(position);
  if (destPosition-mpBegin() < used()-1)
  {
    IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      destPosition->~value_type();
      memmove((void*)destPosition, (void*)(destPosition+1), (char*)end() - (char*)(destPosition+1));
      --used();
      return destPosition;
    }
    else
      eastl::move(destPosition + 1, end(), destPosition);
  }
  --used();
  end()->~value_type();
  return destPosition;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::erase(const_iterator first, const_iterator last)
{
  #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((first < mpBegin()) || (first > end()) || (last < mpBegin()) || (last > end()) || (last < first)))
      DAG_ASSERTF(0, "Vector::erase on invalid position %d..%d (T)=%d", first-cbegin(), last-cbegin(), sizeof(T));
  #endif
  if (first != last)
  {
    IF_CONSTEXPR(is_type_relocatable<value_type>::value)
    {
      eastl::destruct(first, last);
      memmove((void*)const_cast<value_type*>(first), (const void*)last, (char*)end() - (char*)last);
    }
    else
    {
      iterator const position = const_cast<value_type*>(eastl::move(const_cast<value_type*>(last),
                                                                    const_cast<value_type*>(end()),
                                                                    const_cast<value_type*>(first)));
      eastl::destruct(position, end());
    }
    used() -= (last - first);
  }
  return const_cast<value_type*>(first);
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::erase_unsorted(const_iterator position)
{
  #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < begin()) || (position >= end())))
      DAG_ASSERTF(0, "Vector::erase on invalid position %d (T)=%d", position-cbegin(), sizeof(T));
  #endif
  iterator destPosition = const_cast<value_type*>(position);
  IF_CONSTEXPR(is_type_relocatable<value_type>::value)
  {
    destPosition->~value_type();
    --used();
    memmove((void*)destPosition, (void*)end(), sizeof(T));
  }
  else
  {
    *destPosition = eastl::move(*(end() - 1));
    --used();
    end()->~value_type();
  }

  return destPosition;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reverse_iterator
Vector<T, Allocator, init_constructing, Counter>::erase(const_reverse_iterator position)
{
  return reverse_iterator(erase((++position).base()));
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reverse_iterator
Vector<T, Allocator, init_constructing, Counter>::erase(const_reverse_iterator first, const_reverse_iterator last)
{
  return reverse_iterator(erase(last.base(), first.base()));
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::reverse_iterator
Vector<T, Allocator, init_constructing, Counter>::erase_unsorted(const_reverse_iterator position)
{
  return reverse_iterator(erase_unsorted((++position).base()));
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert(const_iterator position, const value_type& value)
{
  #if EASTL_ASSERT_ENABLED
    if (EASTL_UNLIKELY((position < mpBegin()) || (position > end())))
      DAG_ASSERTF(0, "Vector::insert on invalid position %d (T)=%d", position-cbegin(), sizeof(T));
  #endif

  // We implment a quick pathway for the case that the insertion position is at the end and we have free capacity for it.
  const ptrdiff_t n = position - mpBegin(); // Save this because we might reallocate.

  if (n != (ptrdiff_t)used())
    DoInsertValue(position, value);
  else
  {
    if (used() == allocated())
      DoGrow(GetNewCapacity(used()));
    ::new((void*)end()) value_type(value);
    ++used(); // Increment this after the construction above in case the construction throws an exception.
  }

  return mpBegin() + n;
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert(const_iterator position, value_type&& value)
{
  return emplace(position, eastl::move(value));
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert(const_iterator position, size_type n, const value_type& value)
{
  const ptrdiff_t p = position - mpBegin(); // Save this because we might reallocate.
  DoInsertValues(position, n, value);
  return mpBegin() + p;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert_default(const_iterator position, size_type n)
{
  const ptrdiff_t p = position - mpBegin(); // Save this because we might reallocate.
  DoInsertValues(position, n);
  return mpBegin() + p;
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::append_default(size_type n)
{
  const ptrdiff_t p = size(); // Save this because we might reallocate.
  DoInsertValuesEnd<init_constructing>(n);
  return mpBegin() + p;
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
template <typename InputIterator>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert(const_iterator position, InputIterator first, InputIterator last)
{
  const ptrdiff_t n = position - mpBegin(); // Save this because we might reallocate.
  DoInsert(position, first, last, eastl::is_integral<InputIterator>());
  return mpBegin() + n;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
inline typename Vector<T, Allocator, init_constructing, Counter>::iterator
Vector<T, Allocator, init_constructing, Counter>::insert(const_iterator position, std::initializer_list<value_type> ilist)
{
  const ptrdiff_t n = position - mpBegin(); // Save this because we might reallocate.
  DoInsert(position, ilist.begin(), ilist.end(), eastl::false_type());
  return mpBegin() + n;
}

template <typename T, typename Allocator, bool init_constructing, typename Counter>
template <typename Integer>
inline void Vector<T, Allocator, init_constructing, Counter>::DoInit(Integer n, Integer value, eastl::true_type)
{
  size_type a = (size_type)n;
  mpBegin()    = DoAllocate(a);
  used() = (size_type)n;
  allocated() = a;

  typedef typename eastl::remove_const<T>::type non_const_value_type; // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
  eastl::uninitialized_fill_n_ptr<value_type, Integer>((non_const_value_type*)mpBegin(), (size_type)n, value);
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
template <typename InputIterator>
inline void Vector<T, Allocator, init_constructing, Counter>::DoInit(InputIterator first, InputIterator last, eastl::false_type)
{
  typedef typename eastl::iterator_traits<InputIterator>:: iterator_category IC;
  DoInitFromIterator(first, last, IC());
}


template <typename T, typename Allocator, bool init_constructing, typename Counter>
template <typename InputIterator>
inline void Vector<T, Allocator, init_constructing, Counter>::
  DoInitFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
{
  // To do: Use emplace_back instead of push_back(). Our emplace_back will work below without any ifdefs.
  for (; first != last; ++first)  // InputIterators by definition actually only allow you to iterate through them once.
      push_back(*first);        // Thus the standard *requires* that we do this (inefficient) implementation.
}                                 // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


template <typename T, typename Allocator, bool init_constructing, typename Counter>
template <typename ForwardIterator>
inline void Vector<T, Allocator, init_constructing, Counter>::
  DoInitFromIterator(ForwardIterator first, ForwardIterator last, EASTL_ITC_NS::forward_iterator_tag)
{
  const size_type cnt = (size_type)eastl::distance(first, last);
  if (!cnt)
    return;
  size_type n = cnt;
  mpBegin()    = DoAllocate(n);
  allocated() = n;
  used() = cnt;

  typedef typename eastl::remove_const<T>::type non_const_value_type; // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
  eastl::uninitialized_copy_ptr(first, last, (non_const_value_type*)mpBegin());
}

template <typename T, typename Allocator1, bool init1, typename Counter1,
                      typename Allocator2, bool init2, typename Counter2>
inline bool operator==(const Vector<T, Allocator1, init1, Counter1>& a, const Vector<T, Allocator2, init2, Counter2>& b)
{
  return ((a.size() == b.size()) && eastl::equal(a.begin(), a.end(), b.begin()));
}


template <typename T, typename Allocator1, bool init1, typename Counter1,
                      typename Allocator2, bool init2, typename Counter2>
inline bool operator!=(const Vector<T, Allocator1, init1, Counter1>& a, const Vector<T, Allocator2, init2, Counter2>& b)
{
  return ((a.size() != b.size()) || !eastl::equal(a.begin(), a.end(), b.begin()));
}

}
#undef IF_CONSTEXPR

#endif
