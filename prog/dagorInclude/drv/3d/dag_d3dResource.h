//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>

enum class D3DResourceType : uint8_t
{
  TEX,
  CUBETEX,
  VOLTEX,
  ARRTEX,
  CUBEARRTEX,
  SBUF
};

class D3dResource
{
public:
  virtual void destroy() = 0;

  virtual D3DResourceType getType() const = 0;
  virtual uint32_t getSize() const = 0;

  virtual const char *getName() const = 0;
  virtual void setName(const char *name) = 0;
  virtual void setName(const char *name, int size) = 0;

  // WARNING: This might allocate. Avoid calling it every frame.
  virtual void setApiName([[maybe_unused]] const char *name) const {}

protected:
  D3dResource() = default;
  D3dResource(D3dResource &&) = default;
  D3dResource &operator=(D3dResource &&) = default;
  ~D3dResource() = default;
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
