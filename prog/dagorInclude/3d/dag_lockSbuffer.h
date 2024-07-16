//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @file This file contains \ref LockedBuffer class and functions for managing locked memory./
 */

#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/type_traits.h>

/**
 * @brief A template class encapsulating locked shader buffer.
 * 
 * @tparam T Type of the shader buffer data.
 */
template <typename T>
class LockedBuffer
{
  /**
   * @brief A pointer to the buffer containing locked memory.
   */
  Sbuffer *mLockedObject = nullptr;

  /**
   * @brief A pointer to the locked data region.
   */
  T *mData = nullptr;

  /**
   * @brief Number of locked elements in the buffer.
   */
  size_t mCount = 0;

public:

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] locked_object  A pointer to the buffer to lock the data in, i.e. 'managed buffer'.
   * @param [in] ofs_bytes      Offset in bytes from the start of the managed buffer data to the position where locking should begin.
   * @param [in] count          Number of elements to lock or 0 if all elements after \b ofs_bytes have to be locked.
   * @param [in] flags          Flags specifying locking behavior. It has to be a value from \c VBLOCK_ enumeration. See \ref dag_3dConst_base.h for options.
   * 
   * The locked region must fit within buffer boundaries.
   */
  LockedBuffer(Sbuffer *locked_object, uint32_t ofs_bytes, uint32_t count, int flags) : mLockedObject(locked_object)
  {
    G_ASSERT(locked_object);
    // TODO: It is necessary to prohibit having count == 0
    G_ASSERT(!count || ofs_bytes + sizeof(T) * count <= locked_object->ressize());
    uint32_t size_bytes = count ? sizeof(T) * count : mLockedObject->ressize() - ofs_bytes;
    G_ASSERT(size_bytes % sizeof(T) == 0);
    mCount = size_bytes / sizeof(T);
    if (!mLockedObject->lock(ofs_bytes, size_bytes, (void **)&mData, flags) || !mData)
    {
      mLockedObject = nullptr;
      mData = nullptr;
    }
  }

  /**
   * @brief Copy constructor (explicitly deleted).
   */
  LockedBuffer(const LockedBuffer &rhs) = delete;

  /**
   * @brief Move constructor.
   * 
   * @param [in] rhs Buffer to move data from.
   */
  LockedBuffer(LockedBuffer &&rhs) noexcept
  {
    eastl::swap(mLockedObject, rhs.mLockedObject);
    eastl::swap(mData, rhs.mData);
    eastl::swap(mCount, rhs.mCount);
  }

  /**
   * @brief Copy-assignment operator (explicitly deleted).
   */
  LockedBuffer operator=(const LockedBuffer &rhs) = delete;

  /**
   * @brief Move-assignment operator.
   * 
   * @param [in] rhs    Buffer to move data from.
   * @return            Obtained buffer. 
   */
  LockedBuffer &operator=(LockedBuffer &&rhs) noexcept
  {
    eastl::swap(mLockedObject, rhs.mLockedObject);
    eastl::swap(mData, rhs.mData);
    eastl::swap(mCount, rhs.mCount);
    return *this;
  }
  
  /**
   * @brief Destructor.
   * 
   * Unlocks the memory region.
   */
  ~LockedBuffer()
  {
    if (mLockedObject)
      mLockedObject->unlock();
  }

  /**
   * @brief Deletes the managed buffer and the locked data.
   */
  void close() { LockedBuffer tmp(eastl::move(*this)); }

  /**
  * @brief Checks if the buffer is valid.
  * 
  * @return \c true if the object has been properly constructed and not closed.
  */
  explicit operator bool() const { return mLockedObject != nullptr; }

  /**
   * @brief Retrieves a pointer to the locked memory region.
   * 
   * @return    A pointer to the first locked element in the managed buffer.
   * 
   * @todo [[deprecated]]
   */
  T *get() { return mData; }

  /**
   * @brief Accesses a locked element.
   * 
   * @param [in] index  An index of the element to access.
   * @return            Accessed element.
   * 
   * @note  Elements are counted starting from the first locked one.
   *        If the locked region at the fifth element of the managed buffer and \b index is 2, 
   *        the operator accesses the 7-th element in the managed buffer.\n
   * @note  Accessed element must be located within the locked region.
   */
  T &operator[](size_t index)
  {
    G_ASSERT(index < mCount);
    return mData[index];
  }

  /**
   * @brief Copies data to the locked memory region.
   * 
   * @tparam InputIterator Type of the managed buffer iterator.
   * 
   * @param [in] index          An index of the position to begin copying to.
   * @param [in] data           Data to copy.
   * @param [in] count_elements Number of elements to copy.
   * 
   * @note  Elements are counted starting from the first locked one.
   *        If the locked region at the fifth element of the managed buffer and \b index is 2, 
   *        the copying begins at the 7-th position in the managed buffer.\n
   * @note  The data to copy must fit within the locked region boundaries.
   */
  template <typename InputIterator>
  void updateDataRange(size_t index, InputIterator data, size_t count_elements)
  {
    G_ASSERT(index + count_elements <= mCount);
    eastl::copy_n(data, count_elements, &mData[index]);
  }
};

/**
 * @brief A wrapper class for \ref LockedBuffer allowing to access the elements with a specified offset.
 * 
 * @tparam T Type of the data stored in the managed buffer.
 */
template <typename T>
class LockedBufferWithOffset
{
  /**
   * @brief Buffer that implements locking behavior.
   */
  LockedBuffer<T> &mLockedBuffer;

  /**
   * @brief Offset to use when accessing locked elements.
   */
  size_t mOffsetInElems;

public:

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] locked_buffer      Buffer that implements locking behavior.
   * @param [in] offset_in_elements Offset to use when accessing locked elements.
   */
  LockedBufferWithOffset(LockedBuffer<T> &locked_buffer, size_t offset_in_elements = 0) :
    mLockedBuffer(locked_buffer), mOffsetInElems(offset_in_elements)
  {}

  /**
   * @brief Accesses a locked element.
   *
   * @param [in] index  An index of the element to access.
   * @return            Accessed element.
   *
   * @note  This operator is similar to \ref LockedBuffer::operator[], but 
   *        \c mOffsetInElems is added to \b index when accessing the element.\n
   * @note <c> mOffsetInElems + index </c> must be within the locked region.
   */
  T &operator[](size_t index) { return mLockedBuffer[mOffsetInElems + index]; }

  /**
   * @brief Increases the offset by a specified value.
   * 
   * @param [in] count  Value to increase the offset by.
   * @return            Modified buffer.
   */
  LockedBufferWithOffset &operator+=(size_t count)
  {
    mOffsetInElems += count;
    return *this;
  }
};

/**
 * @brief Locks data in the shader buffer.
 *
 * @tparam T Type of the data stored in the shader buffer.
 * 
 * @param [in] buf          A pointer to the buffer to lock the data in.
 * @param [in] ofs_bytes    Offset in bytes from the start of the buffer data to the position where locking should begin.
 * @param [in] count        Number of elements to lock or 0 if all elements after \b ofs_bytes have to be locked.
 * @param [in] flags        Flags specifying locking behavior. It has to be a value from \c VBLOCK_ enumeration. See \ref dag_3dConst_base.h for options.
 * @return                  Constructed \ref LockedBuffer object.
 * 
 * The locked region must fit within buffer boundaries.
 * 
 * @warning \c VBLOCK_READONLY flag should be used if and only if \b T is a const type.
 *          In case \b T is a const type, this flag will be forced.
 */
template <typename T>
LockedBuffer<T> lock_sbuffer(Sbuffer *buf, uint32_t ofs_bytes, uint32_t count, int flags)
{
  G_ASSERT(buf);
  if (eastl::is_const_v<T>)
    flags |= VBLOCK_READONLY;
  G_ASSERT(eastl::is_const_v<T> == !!(flags & VBLOCK_READONLY));
  return LockedBuffer<T>(buf, ofs_bytes, count, flags);
}

/**
 * @brief Issues an event querry that fires upon buffer read-back readiness. (?)
 * 
 * @param [in] query    A pointer to the query to issue.
 * @param [in] buf      A pointer to the buffer to query readiness of.
 */
inline bool issue_readback_query(D3dEventQuery *query, Sbuffer *buf)
{
  if (buf->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
    buf->unlock();
  return d3d::issue_event_query(query);
}

