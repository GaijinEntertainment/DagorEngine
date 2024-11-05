//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>


#include <supp/dag_define_KRNLIMP.h>

KRNLIMP extern void *hierbit_allocmem(int sz);
KRNLIMP extern void hierbit_freemem(void *p, int sz);


template <class T>
T *hb_alloc(int cnt)
{
  return (T *)hierbit_allocmem(sizeof(T) * cnt);
}

template <class T>
T *hb_alloc_clr(int cnt)
{
  T *p = (T *)hierbit_allocmem(sizeof(T) * cnt);
  memset(p, 0, sizeof(T) * cnt);
  return p;
}

template <class T>
T *hb_new(int cnt)
{
  T *p = (T *)hierbit_allocmem(sizeof(T) * cnt);
  for (int i = 0; i < cnt; i++)
    p[i].ctor();
  return p;
}

template <class T>
T *hb_new_copy(const T &a)
{
  T *p = (T *)hierbit_allocmem(sizeof(T));
  p->ctor(a);
  return p;
}


template <class T>
void hb_free(T *p, int cnt)
{
  hierbit_freemem(p, sizeof(T) * cnt);
}

template <class T>
void hb_delete(T *_p, int cnt)
{
  T *p = (T *)_p;
  for (int i = 0; i < cnt; i++)
    p[i].dtor();
  hierbit_freemem(p, sizeof(T) * cnt);
}

#include <supp/dag_undef_KRNLIMP.h>
