#pragma once
#ifndef __GAIJIN_PROTOIO_HASEXTKEY__
#define __GAIJIN_PROTOIO_HASEXTKEY__

namespace proto
{
  template <typename T>
  struct HasExtKey
  {
    static const bool value = false;
  };

  template <bool HasExtKey>
  struct SetExtKeyHelper
  {
    template <typename T, typename K>
    static bool set(T &, const K &)
    {
      return false;
    }
  };

  template <>
  struct SetExtKeyHelper<true>
  {
    template <typename T, typename K>
    static bool set(T & obj, const K & key)
    {
      obj.setExtKey(key);
      return true;
    }
  };

  template <typename T, typename K>
  bool set_ext_key(T & obj, const K & key)
  {
    return SetExtKeyHelper<proto::HasExtKey<T>::value>::set(obj, key);
  }
}

#endif // __GAIJIN_PROTOIO_HASEXTKEY__
