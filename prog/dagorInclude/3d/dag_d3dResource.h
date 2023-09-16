//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
  // WARNING: This might allocate. Avoid calling it every frame.
  virtual void setResApiName(const char * /*name*/) const {}

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

inline void destroy_d3dres(D3dResource *res) { return res ? res->destroy() : (void)0; }
template <class T>
inline void del_d3dres(T *&p)
{
  destroy_d3dres(p);
  p = nullptr;
}
