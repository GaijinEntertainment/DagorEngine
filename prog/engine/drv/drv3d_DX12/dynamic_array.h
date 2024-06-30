#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>

/**
 * @file 
 * @brief Defines a dynamic array class template for managing arrays with dynamic sizes.
 */

/**
 * @brief A dynamic array class template for managing arrays with dynamic sizes.
 * @tparam T The type of elements stored in the array.
 */
template <typename T>
class DynamicArray
{
  /**
   * @brief Pointer to the dynamically allocated array.
   */
  eastl::unique_ptr<T[]> ptr;

  /**
   * @brief The number of elements in the array.
   */
  size_t count = 0;

public:

  /**
   * @brief Default constructor.
   */
  DynamicArray() = default;

  /**
   * @brief Copy constructor (deleted to prevent copying).
   */
  DynamicArray(const DynamicArray &) = delete;

  /**
   * @brief Move constructor
   * @param [in] other The other dynamic array to construct from.
   */
  DynamicArray(DynamicArray &&) = default;

  /**
   * @brief Constructs a dynamic array from a raw pointer and size.
   * @param [in] p  A pointer to the array.
   * @param [in] sz The size of the array.
   */
  DynamicArray(T *p, size_t sz) : ptr{p}, count{sz} {}

  /**
   * @brief Constructs a dynamic array from a unique pointer and size.
   * @param [in] p  A pointer to the array.
   * @param [in] sz The size of the array.
   */
  DynamicArray(eastl::unique_ptr<T[]> &&p, size_t sz) : ptr{eastl::forward<eastl::unique_ptr<T[]>>(p)}, count{sz} {}

  /**
   * @brief Constructs a empty dynamic array with a specified size.
   * @param [in] sz The size of the array.
   */
  explicit DynamicArray(size_t sz) : DynamicArray{eastl::make_unique<T[]>(sz), sz} {}

  /**
   * @brief Copy assignment operator (deleted to prevent copying).
   */
  DynamicArray &operator=(const DynamicArray &) = delete;

  /**
   * @brief Move assignment operator.
   * @param other The dynamic array to move from.
   * @return Reference to the moved dynamic array.
   */
  DynamicArray &operator=(DynamicArray &&other)
  {
    eastl::swap(ptr, other.ptr);
    eastl::swap(count, other.count);
    return *this;
  }

  /**
   * @brief Adopts a new pointer and size, releasing ownership of the previous pointer.
   * @param new_ptr Pointer to the new array.
   * @param new_sz Size of the new array.
   * 
   * The method also calls descructors for the elements stored before the adoption.
   */
  void adopt(T *new_ptr, size_t new_sz)
  {
    eastl::unique_ptr<T[]> newPtr{new_ptr};
    eastl::swap(ptr, newPtr);
    count = new_sz;
  }

  /**
   * @brief Releases ownership of the array.
   * @return Pointer to the array.
   * 
   * @note The method does not free memory occupied by the array.
   */
  T *release()
  {
    count = 0;
    return ptr.release();
  }

  /**
   * @brief Changes the size of the array to a specified value.
   * @param new_size The new size of the array.
   * @return True if resizing is successful, false otherwise.
   * 
   * @note If resizing is successful, all previously retrieved raw pointers to the stored elements become invalidated,
   * as in the process of resizing memory is reallocated.
   */
  bool resize(size_t new_size)
  {
    if (count == new_size)
    {
      return false;
    }

    if (0 == new_size)
    {
      ptr.reset();
      count = 0;
      return true;
    }

    auto newBlock = eastl::make_unique<T[]>(new_size);
    if (!newBlock)
    {
      return false;
    }

    auto moveCount = min(count, new_size);
    for (uint32_t i = 0; i < moveCount; ++i)
    {
      newBlock[i] = eastl::move(ptr[i]);
    }
    eastl::swap(ptr, newBlock);
    count = new_size;
    return true;
  }

  /**
   * @brief Accesses an element of the array.
   * @param [in] i The index of the element.
   * @return Reference to the element at the specified index.
   */
  T &operator[](size_t i) { return ptr[i]; }

  /**
   * @brief Access an element of the array (const version).
   * @param i The index of the element.
   * @return Reference to the element at the specified index.
   */
  const T &operator[](size_t i) const { return ptr[i]; }

  /**
   * @brief Returns the number of elements in the array.
   * @return The number of elements.
   */
  size_t size() const { return count; }

  /**
   * @brief Checks if the array is empty.
   * @return True if the array is empty, false otherwise.
   */
  bool empty() const { return !ptr || 0 == count; }

  /**
   * @brief Returns a raw pointer to the underlying array data.
   * @return A pointer to the array data.
   */
  T *data() { return ptr.get(); }

  /**
   * @brief Returns a raw pointer to the underlying array data (const version).
   * @return A const pointer to the array data.
   */
  const T *data() const { return ptr.get(); }

  /**
   * @brief Creates a span from the contents of the array.
   * @return A span representing the array.
   */
  eastl::span<T> asSpan() { return {data(), size()}; }

  /**
   * @brief Creates a span from the contents of the array (const version).
   * @return A span representing the array.
   */
  eastl::span<const T> asSpan() const { return {data(), size()}; }

  /**
   * @brief Creates a span from the contents of the array and then release ownership of it.
   * @return A span representing the array.
   * 
   * @note The method does not free memory occupied by the array.
   */
  eastl::span<T> releaseAsSpan()
  {
    auto retValue = asSpan();
    release();
    return retValue;
  }

  /**
   * @brief Creates a dynamic array from a given span.
   * @param span The span to create a dynamic array from.
   * @return A dynamic containing the elements of the span.
   * 
   * @note The method does not copy the contents of the span. 
   * As a result, the array and span manage the same data.
   */
  static DynamicArray fromSpan(eastl::span<T> span) { return {span.data(), span.size()}; }

  /**
   * @brief Returns a pointer to the first element of the array.
   * @return A pointer to the first element of the array.
   */
  T *begin() { return ptr.get(); }

  /**
   * @brief Returns a pointer to the first element of the array (const version).
   * @return A pointer to the first element of the array.
   */
  const T *begin() const { return ptr.get(); }

  /**
   * @brief Returns a const pointer to the first element of the array.
   * @return A const pointer to the first element of the array.
   */
  const T *cbegin() const { return begin(); }

  /**
   * @brief Returns a pointer to the virtual element following the last element of the vector.
   * @return A pointer to the virtual element following the last element of the vector.
   */
  T *end() { return begin() + size(); }

  /**
   * @brief Returns a raw pointer to the virtual element following the last element of the vector (const version).
   * @return A raw pointer to the virtual element following the last element of the vector.
   */
  const T *end() const { return begin() + size(); }

  /**
   * @brief Returns a const pointer to the virtual element following the last element of the vector.
   * @return A const pointer to the virtual element following the last element of the vector.
   */
  const T *cend() const { return end(); }
};
