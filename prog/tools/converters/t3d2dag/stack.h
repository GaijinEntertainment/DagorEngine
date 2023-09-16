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

#ifndef _STACK_H_
#define _STACK_H_

#ifndef NULL
#define NULL 0
#endif

template <class ARG_TYPE>
class StackNode
{
public:
  ARG_TYPE object;
  StackNode *next;
};

template <class ARG_TYPE>
class Stack
{
protected:
  int size;
  StackNode<ARG_TYPE> *top;

public:
  Stack()
  {
    size = 0;
    top = NULL;
  }

  ~Stack()
  {
    StackNode<ARG_TYPE> *tmp, *node = top;

    while (node != NULL)
    {
      tmp = node;
      node = node->next;
      delete tmp;
    }
  }

  void push_back(const ARG_TYPE object)
  {
    StackNode<ARG_TYPE> *newNode = new StackNode<ARG_TYPE>;
    newNode->next = top;
    newNode->object = object;
    top = newNode;

    size++;
  }

  ARG_TYPE pop_back()
  {
    StackNode<ARG_TYPE> *oldNode = top;
    ARG_TYPE object = top->object;

    top = top->next;
    delete oldNode;

    size--;
    return object;
  }

  const int getSize() { return size; }
};

#endif // _STACK_H_
