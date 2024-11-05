//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>

class D3dResource
{
public:
  virtual void destroy() = 0;
  virtual int restype() const = 0;
  virtual int ressize() const = 0;
  const char *getResName() const { return statName.c_str(); }
  void setResName(const char *name) { statName = name; }
  void setResName(const char *name, int size) { statName.setStr(name, size); }
  // WARNING: This might allocate. Avoid calling it every frame.
  virtual void setResApiName(const char * /*name*/) const {}

  D3dResource() = default;
  D3dResource(D3dResource &&) = default;
  D3dResource &operator=(D3dResource &&) = default;

protected:
  virtual ~D3dResource(){};

private:
  String statName;
};

enum
{
  RES3D_TEX,
  RES3D_CUBETEX,
  RES3D_VOLTEX,
  RES3D_ARRTEX,
  RES3D_CUBEARRTEX,
  RES3D_SBUF
};

inline void destroy_d3dres(D3dResource *res)
{
  if (res)
    res->destroy();
}

template <class T>
inline void del_d3dres(T *&p)
{
  destroy_d3dres(p);
  p = nullptr;
}
