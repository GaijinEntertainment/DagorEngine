// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

template <class X>
class dag_auto_ptr
{
public:
  // 20.4.5.1 construct/copy/destroy:
  explicit dag_auto_ptr(X *p = 0) throw() { ptr = p; }
  dag_auto_ptr(const dag_auto_ptr &ap) throw()
  {
    ptr = ap.ptr;
    ap.ptr = NULL;
  }

  dag_auto_ptr &operator=(const dag_auto_ptr &ap) throw()
  {
    if (this != &ap)
    {
      ptr = ap.ptr;
      ap.ptr = NULL;
    }
    return *this;
  }

  ~dag_auto_ptr() throw()
  {
    if (ptr)
      delete ptr;
    ptr = NULL;
  }

  // 20.4.5.2 members:
  X &operator*() const throw() { return *ptr; }
  X *operator->() const throw() { return ptr; }
  X *get() const throw() { return ptr; }
  X *release() throw()
  {
    X *p = ptr;
    ptr = NULL;
    return p;
  }

private:
  mutable X *ptr;
};
