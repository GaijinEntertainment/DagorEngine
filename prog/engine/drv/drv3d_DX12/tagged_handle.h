#pragma once


template <typename H, H NullValue, typename T>
class TaggedHandle
{
  H h; // -V730_NOINIT

public:
  bool interlockedIsNull() const { return interlocked_acquire_load(h) == NullValue; }
  void interlockedSet(H v) { interlocked_release_store(h, v); }
  H get() const { return h; }
  explicit TaggedHandle(H a) : h(a) {}
  TaggedHandle() {}
  bool operator!() const { return h == NullValue; }
  friend bool operator==(TaggedHandle l, TaggedHandle r) { return l.get() == r.get(); }
  friend bool operator!=(TaggedHandle l, TaggedHandle r) { return l.get() != r.get(); }
  friend bool operator<(TaggedHandle l, TaggedHandle r) { return l.get() < r.get(); }
  friend bool operator>(TaggedHandle l, TaggedHandle r) { return l.get() > r.get(); }
  friend H operator-(TaggedHandle l, TaggedHandle r) { return l.get() - r.get(); }
  static TaggedHandle Null() { return TaggedHandle(NullValue); }
  static TaggedHandle make(H value) { return TaggedHandle{value}; }
};

template <typename H, H NullValue, typename T>
inline TaggedHandle<H, NullValue, T> genereate_next_id(TaggedHandle<H, NullValue, T> last_id)
{
  auto value = last_id.get() + 1;
  if (value == NullValue)
    ++value;
  G_ASSERT(value != NullValue);
  return TaggedHandle<H, NullValue, T>{value};
}
