//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// this is to workaround DX11 inability to make multridrawindirect with passing _something_ to each subsequent draw
// basically, this is vertex buffer with serial ints [0 ... count), which you can set as a instanced stream (also requires changing
// vdecl) that is much slower on GPU than all normal APIs which _pass_ baseInstanceId in indirect args to drawcall yet that is THE ONLY
// way on DX11 right now, on DX12 there is also no reasonable implementation. But DX12 allows changing root constant between draws, so
// we can make it working better

namespace serial_buffer
{
extern void init_serial(uint32_t count); // should be paired with term
extern void term_serial();               //


extern Sbuffer *start_use_serial(uint32_t count);
extern void stop_use_serial(Sbuffer *);
struct SerialBufferUsage
{
  SerialBufferUsage(SerialBufferUsage &&s) : sb(s.sb) { s.sb = nullptr; }
  SerialBufferUsage(uint32_t count = 0) : sb(count ? start_use_serial(count) : nullptr) {}
  ~SerialBufferUsage()
  {
    if (sb)
      stop_use_serial(sb);
  }
  Sbuffer *get() const { return sb; }
  explicit operator Sbuffer *() const { return sb; }

private:
  Sbuffer *sb = nullptr;
  SerialBufferUsage(const SerialBufferUsage &) = delete;
  SerialBufferUsage &operator=(const SerialBufferUsage &) = delete;
};
struct SerialBufferCounter
{
  void init(uint32_t count_)
  {
    if (count)
      term_serial();
    count = count_;
    init_serial(count);
  }
  SerialBufferCounter(uint32_t count_) : count(count_) { init_serial(count); }
  SerialBufferCounter() = default;
  SerialBufferCounter(SerialBufferCounter &&a) : count(a.count) { a.count = 0; }
  SerialBufferCounter &operator=(SerialBufferCounter &&a)
  {
    uint32_t t = count;
    count = a.count;
    a.count = t;
    return *this;
  }
  SerialBufferUsage get() { return SerialBufferUsage(count); }
  ~SerialBufferCounter()
  {
    if (count)
      term_serial();
  }

protected:
  uint32_t count = 0;
};
} // namespace serial_buffer
