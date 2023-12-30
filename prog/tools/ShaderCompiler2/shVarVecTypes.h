#pragma once

namespace shc
{
struct Col4
{
  float r, g, b, a;

  template <typename T>
  void set(const T &c)
  {
    r = c.r, g = c.g, b = c.b, a = c.a;
  }
  void set(float _r, float _g, float _b, float _a) { r = _r, g = _g, b = _b, a = _a; }
};

struct Int4
{
  int x, y, z, w;

  template <typename T>
  void set(const T &p)
  {
    x = p.x, y = p.y, z = p.z, w = p.w;
  }
  void set(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; }
};
}; // namespace shc
