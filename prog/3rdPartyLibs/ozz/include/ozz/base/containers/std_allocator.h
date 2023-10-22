//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_
#define OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_

#include <new>

#include "ozz/base/memory/allocator.h"

namespace ozz {
// Define a STL allocator compliant allocator->
template <typename _Ty>
class StdAllocator {
 public:
  typedef _Ty value_type;                     // Element type.
  typedef value_type* pointer;                // Pointer to element.
  typedef value_type& reference;              // Reference to element.
  typedef const value_type* const_pointer;    // Constant pointer to element.
  typedef const value_type& const_reference;  // Constant reference to element.
  typedef size_t size_type;                   // Quantities of elements.
  typedef ptrdiff_t difference_type;  // Difference between two pointers.

  StdAllocator() noexcept {}
  StdAllocator(const StdAllocator&) noexcept {}

  template <class _Other>
  StdAllocator(const StdAllocator<_Other>&) noexcept {}

  template <class _Other>
  struct rebind {
    typedef StdAllocator<_Other> other;
  };

  pointer address(reference _ref) const noexcept { return &_ref; }
  const_pointer address(const_reference _ref) const noexcept { return &_ref; }

  template <class _Other, class... _Args>
  void construct(_Other* _ptr, _Args&&... _args) {
    ::new (static_cast<void*>(_ptr)) _Other(std::forward<_Args>(_args)...);
  }

  template <class _Other>
  void destroy(_Other* _ptr) {
    (void)_ptr;
    _ptr->~_Other();
  }

  // Allocates array of _Count elements.
  pointer allocate(size_t _count) noexcept {
    // Makes sure to a use c like allocator, to avoid duplicated constructor
    // calls.
    return reinterpret_cast<pointer>(memory::default_allocator()->Allocate(
        sizeof(value_type) * _count, alignof(value_type)));
  }

  // Deallocates object at _Ptr, ignores size.
  void deallocate(pointer _ptr, size_type) noexcept {
    memory::default_allocator()->Deallocate(_ptr);
  }

  size_type max_size() const noexcept {
    return (~size_type(0)) / sizeof(value_type);
  }
};

// Tests for allocator equality (always true).
template <class _Ty, class _Other>
inline bool operator==(const StdAllocator<_Ty>&,
                       const StdAllocator<_Other>&) noexcept {
  return true;
}

// Tests for allocator inequality (always false).
template <class _Ty, class _Other>
inline bool operator!=(const StdAllocator<_Ty>&,
                       const StdAllocator<_Other>&) noexcept {
  return false;
}
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_
