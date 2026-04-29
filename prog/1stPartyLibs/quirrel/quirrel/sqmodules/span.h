#pragma once
#include <cstddef>
#include <type_traits>
#include <vector>
#include <cassert>
#include <sqrat.h>

namespace sqm
{

template <typename T>
class span {
public:
    typedef T           element_type;
    typedef typename SQRAT_STD::remove_cv<T>::type
                        value_type;
    typedef size_t      size_type;
    typedef ptrdiff_t   index_type;
    typedef T*          pointer;
    typedef T&          reference;
    typedef const T*    const_pointer;
    typedef const T&    const_reference;
    typedef T*          iterator;
    typedef const T*    const_iterator;

    span() noexcept : ptr_(nullptr), size_(0) {}

    span(pointer ptr, size_type count) : ptr_(ptr), size_(count) {}

    span(pointer first, pointer last)
        : ptr_(first), size_(static_cast<size_type>(last - first)) {
        assert(last >= first);
    }

    template <size_type N>
    span(T (&arr)[N]) noexcept : ptr_(arr), size_(N) {}

    template <typename U,
              typename = typename SQRAT_STD::enable_if<!SQRAT_STD::is_const<T>::value &&
                                                 SQRAT_STD::is_same<U, typename SQRAT_STD::remove_const<T>::type>::value>::type>
    span(SQRAT_STD::vector<U>& v) noexcept : ptr_(v.data()), size_(v.size()) {}

    template <typename U,
              typename = typename SQRAT_STD::enable_if<SQRAT_STD::is_same<U, typename SQRAT_STD::remove_const<T>::type>::value>::type>
    span(const SQRAT_STD::vector<U>& v) noexcept : ptr_(v.data()), size_(v.size()) {}

    template <typename U,
              typename = typename SQRAT_STD::enable_if<SQRAT_STD::is_same<const U, T>::value>::type>
    span(const span<U>& other) noexcept : ptr_(other.data()), size_(other.size()) {}

    size_type size() const noexcept { return size_; }
    size_type size_bytes() const noexcept { return size_ * sizeof(T); }
    bool empty() const noexcept { return size_ == 0; }
    pointer data() const noexcept { return ptr_; }

    reference operator[](size_type i) const {
        return ptr_[i];
    }

    reference at(size_type i) const {
        return ptr_[i];
    }

    reference front() const {
        return *ptr_;
    }

    reference back() const {
        return ptr_[size_ - 1];
    }

    iterator begin() const noexcept { return ptr_; }
    iterator end() const noexcept { return ptr_ + size_; }
    const_iterator cbegin() const noexcept { return ptr_; }
    const_iterator cend() const noexcept { return ptr_ + size_; }

private:
    pointer   ptr_;
    size_type size_;
};

template <typename T>
inline span<T> make_span(T* ptr, size_t count) { return span<T>(ptr, count); }

template <typename T>
inline span<T> make_span(T* first, T* last) { return span<T>(first, last); }

template <typename T, size_t N>
inline span<T> make_span(T (&arr)[N]) { return span<T>(arr); }

template <typename T>
inline span<T> make_span(SQRAT_STD::vector<T>& v) { return span<T>(v); }

template <typename T>
inline span<const T> make_span(const SQRAT_STD::vector<T>& v) { return span<const T>(v); }

}
