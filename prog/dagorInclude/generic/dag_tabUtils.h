//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/************************************************************************
  some utility functions for Tab<T> & Tab<T*> templates
************************************************************************/

#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>
#include <util/dag_globDef.h>
#include <generic/dag_DObject.h>

namespace tabutils
{
/************************************************************************
  return true, if it's correct index for this tab

  usage:
  boolean = isCorrectIndex(tab, index);
************************************************************************/
template <class V, typename T = typename V::value_type>
inline bool isCorrectIndex(const V &tab, int index)
{
  return (index >= 0) && (index < tab.size());
}

/************************************************************************
  return value T* from Tab<T*> with check index range

  usage:
  pointer = getPtrVal(tab_ptr_array, index);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline T getPtrVal(const V &tab, int index)
{
  return isCorrectIndex(tab, index) ? tab[index] : NULL;
}

/************************************************************************
  return first value of the Tab<T*>

  usage:
  pointer = getFirst(tab_ptr_array);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline T getFirst(const V &tab)
{
  return getPtrVal(tab, 0);
}

/************************************************************************
  return last value of the Tab<T*>

  usage:
  pointer = getLast(tab_ptr_array);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline T getLast(const V &tab)
{
  return getPtrVal(tab, tab.size() - 1);
}

/************************************************************************
  correct delete all elements in the Tab<T*>

  usage:
  deleteAll(tab_ptr_array);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline void deleteAll(V &tab)
{
  clear_all_ptr_items_and_shrink(tab);
}

/************************************************************************
  erase and delete element in the Tab<T*>

  usage:
  deleteElement(tab_ptr_array, index);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline bool deleteElement(V &tab, int index)
{
  if (!isCorrectIndex(tab, index))
    return false;
  if (tab[index] != NULL)
    delete tab[index];
  erase_items(tab, index, 1);
  return true;
}


/************************************************************************
  erase and delete element in the Tab<T*>

  usage:
  deleteElement(tab_ptr_array, index);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T *, U>::is_true>
inline bool deleteElement(V &tab, T *element)
{
  int index = -1;
  for (int i = 0; i < tab.size(); i++)
  {
    if (tab[i] == element)
    {
      index = i;
      break;
    }
  }

  return deleteElement(tab, index);
}

/************************************************************************
  get element's index in the Tab<T*> using T.operator== ()

  usage:
  T element;
  int index = getIndex(tab_ptr_array, element);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T *const, U const>::is_true>
inline int getIndex(const V &tab, const T &element_to_find)
{
  for (int i = 0; i < tab.size(); i++)
    if (tab[i] && (*tab[i] == element_to_find))
      return i;
  return -1;
}


/************************************************************************
  get element's index in the Tab<T> using T.operator== ()

  usage:
  T element;
  int index = getIndex(tab_array, element);
************************************************************************/
template <typename V, typename U = typename V::value_type, typename T, bool s = dag::is_same<T const, U const>::is_true>
inline int getIndex(const V &tab, const T &element_to_find)
{
  for (int i = 0; i < tab.size(); i++)
    if (tab[i] == element_to_find)
      return i;

  return -1;
}

template <class T>
inline int getIndex(const PtrTab<T> &tab, const T *element_to_find)
{
  for (int i = 0; i < tab.size(); i++)
    if (tab[i] == element_to_find)
      return i;

  return -1;
}


/************************************************************************
  find element in the Tab<T*> using T.operator== ()

  usage:
  T element;
  T* found = find(tab_ptr_array, element);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T *const, U const>::is_true>
inline T *find(const V &tab, const T &element_to_find)
{
  int index = getIndex(tab, element_to_find);
  return isCorrectIndex(tab, index) ? tab[index] : NULL;
}

template <class T>
inline bool find(const PtrTab<T> &tab, const T *element_to_find)
{
  for (int i = 0; i < tab.size(); i++)
    if (tab[i] == element_to_find)
      return true;

  return false;
}

/************************************************************************
find element in the Tab<T> using T.operator== (). return true, if element present.

usage:
T element;
bool found = find(tab_array, element);
************************************************************************/
template <class V, typename U = typename V::value_type, typename T, bool s = dag::is_same<T const, U const>::is_true>
inline bool find(const V &tab, const T &element_to_find)
{
  int index = getIndex(tab, element_to_find);
  return isCorrectIndex(tab, index);
}


/************************************************************************
  duplicate Tab<T*> using assigment constructor of T /T(const T&)/

  usage:
  Tab<T*> source, dest;
  duplicate(source, dest);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline void duplicate(const V &src, V &dst)
{
  deleteAll(dst);
  dst.resize(src.size());
  for (int i = 0; i < src.size(); i++)
  {
    G_ASSERT(src[i] != NULL);
    dst[i] = new T(*src[i]);
  }
}


/************************************************************************
  compare two Tab<T*> using T.operator == ()

  usage:
  Tab<T*> source, dest;
  result = equals(source, dest);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline bool equalItems(const V &first, const V &second)
{
  if (first.size() != second.size())
    return false;

  for (int i = 0; i < first.size(); i++)
  {
    G_ASSERT(first[i] != NULL);
    G_ASSERT(second[i] != NULL);

    if (!(*first[i] == *second[i]))
      return false;
  }

  return true;
}


/************************************************************************
  compare two Tab<T> using memcmp

  usage:
  Tab<T> source, dest;
  result = equals(source, dest);
************************************************************************/
template <class V, typename T = typename V::value_type>
inline bool equals(const V &first, const V &second)
{
  if (first.size() != second.size())
    return false;
  return !first.size() || mem_eq(first, second.data());
}


/************************************************************************
  append value to a Tab<T*>

  usage:
  append(tab_ptr_array, new T(...));
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T *, U>::is_true>
inline int append(V &tab, T *value)
{
  return append_items(tab, 1, &value);
}


/************************************************************************
  erases value from Tab<T> - return false, if not found

  usage:
  erase(tab, value);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T, U>::is_true>
inline bool erase(V &tab, const T &value)
{
  int index = getIndex(tab, value);
  if (!isCorrectIndex(tab, index))
    return false;

  erase_items(tab, index, 1);
  return true;
}

/************************************************************************
  add element in the Tab<T>, if it not exists

  usage:
  addUnique(tab, elem);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T, U>::is_true>
inline bool addUnique(V &tab, const T &value)
{
  if (find(tab, value))
    return false;
  tab.push_back(value);
  return true;
}

template <class T>
inline bool addUnique(PtrTab<T> &tab, T *value)
{
  if (find(tab, value))
    return false;
  tab.push_back(value);
  return true;
}

template <class T>
inline bool addUnique(PtrTab<T> &tab, const Ptr<T> &value)
{
  if (find(tab, (T *)value))
    return false;
  tab.push_back((T *)value);
  return true;
}


/************************************************************************
  set all elements of array to value (using operator=)

  usage:
  setAll(tab, value);
************************************************************************/
template <class V, typename T, typename U = typename V::value_type, bool s = dag::is_same<T, U>::is_true>
inline void setAll(V &tab, const T &value)
{
  for (int i = 0; i < tab.size(); i++)
    tab[i] = value;
}

inline void setAll(Tab<char> &tab, const char &value) { memset(tab.data(), value, data_size(tab)); }

inline void setAll(Tab<unsigned char> &tab, const unsigned char &value) { memset(tab.data(), value, data_size(tab)); }

inline void setAll(Tab<bool> &tab, const bool &value) { memset(tab.data(), value, data_size(tab)); }

inline void setZero(Tab<int> &tab)
{
  if (tab.size())
    mem_set_0(tab);
}
inline void setZero(Tab<real> &tab)
{
  if (tab.size())
    mem_set_0(tab);
}
inline void setZero(Tab<bool> &tab)
{
  if (tab.size())
    mem_set_0(tab);
}
inline void setZero(Tab<char> &tab)
{
  if (tab.size())
    mem_set_0(tab);
}
inline void setZero(Tab<unsigned char> &tab)
{
  if (tab.size())
    mem_set_0(tab);
}

template <typename T>
inline void setZeroPtrs(Tab<T *> &tab)
{
  mem_set_0(tab);
}

/************************************************************************
  delete all, resize & set all pointers in Tab<T*> to NULL

  usage:
  safeResize(tab, value);
************************************************************************/
template <class V, typename T = typename V::value_type, size_t S = sizeof(*(T) nullptr)>
inline void safeResize(V &tab, int new_size)
{
  deleteAll(tab);
  tab.resize(new_size);
  setZeroPtrs(tab);
}
} // namespace tabutils
