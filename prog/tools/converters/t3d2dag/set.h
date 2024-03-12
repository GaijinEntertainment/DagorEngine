/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#ifndef _SET_H_
#define _SET_H_

#include <stdlib.h>
#include <string.h>

template <class ARG_TYPE>
class Set
{
protected:
  unsigned int capasity;
  unsigned int size;
  ARG_TYPE *list;

public:
  Set()
  {
    size = capasity = 0;
    list = NULL;
  }

  Set(const unsigned int iCapasity)
  {
    size = 0;
    capasity = iCapasity;
    list = (ARG_TYPE *)malloc(capasity * sizeof(ARG_TYPE));
  }

  ~Set() { free(list); }

  ARG_TYPE *getArray() const { return list; }

  ARG_TYPE &operator[](const unsigned int index) const { return list[index]; }

  void setSize(const unsigned int newSize)
  {
    capasity = size = newSize;
    list = (ARG_TYPE *)realloc(list, capasity * sizeof(ARG_TYPE));
  }

  unsigned int add(const ARG_TYPE object)
  {
    if (size >= capasity)
    {
      if (capasity)
        capasity += capasity;
      else
        capasity = 32;
      list = (ARG_TYPE *)realloc(list, capasity * sizeof(ARG_TYPE));
    }
    list[size] = object;
    return size++;
  }

  void remove(const unsigned int index)
  {
    if (index < size)
    {
      size--;
      list[index] = list[size];
    }
  }

  void orderedRemove(const unsigned int index)
  {
    if (index < size)
    {
      size--;
      memmove(list + index, list + index + 1, (size - index) * sizeof(ARG_TYPE));
    }
  }

  void removeObject(const ARG_TYPE object)
  {
    for (unsigned int i = 0; i < size; i++)
    {
      if (object == list[i])
      {
        remove(i);
        return;
      }
    }
  }

  void clear() { size = 0; }

  void reset()
  {
    free(list);
    list = NULL;
    size = capasity = 0;
  }

  unsigned int getSize() const { return size; }

private:
  int partition(int (*compare)(const ARG_TYPE &elem0, const ARG_TYPE &elem1), int p, int r)
  {
    ARG_TYPE tmp, pivot = list[p];
    int left = p;

    for (int i = p + 1; i <= r; i++)
    {
      if (compare(list[i], pivot) < 0)
      {
        left++;
        tmp = list[i];
        list[i] = list[left];
        list[left] = tmp;
      }
    }
    tmp = list[p];
    list[p] = list[left];
    list[left] = tmp;
    return left;
  }

  void quickSort(int (*compare)(const ARG_TYPE &elem0, const ARG_TYPE &elem1), int p, int r)
  {
    if (p < r)
    {
      int q = partition(compare, p, r);
      quickSort(compare, p, q - 1);
      quickSort(compare, q + 1, r);
    }
  }

public:
  void sort(int (*compare)(const ARG_TYPE &elem0, const ARG_TYPE &elem1)) { quickSort(compare, 0, size - 1); }
};

#endif // _SET_H_
