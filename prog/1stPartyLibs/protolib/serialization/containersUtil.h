#pragma once
#ifndef __GAIJIN_PROTO_CONTAINERSUTIL__
#define __GAIJIN_PROTO_CONTAINERSUTIL__

namespace proto
{
  namespace util
  {
    struct Yes
    {
      int v[666];
    };

    struct No
    {
    };

    template <typename T>
    class CanEraseReturnIterator
    {
    public:
      struct A {};
      struct B : A {};

      typedef typename T::const_iterator const_iterator;
      typedef typename T::iterator iterator;
      static No   test(void (T::*fn)(iterator), A);
      static No   test(void (T::*fn)(const_iterator), B);
      static Yes  test(const_iterator (T::*fn)(const_iterator), B);
      static Yes  test(iterator (T::*fn)(const_iterator), A);
      static Yes  test(iterator (T::*fn)(iterator), B);

      enum { value = sizeof(CanEraseReturnIterator<T>::test(&T::erase, B())) == sizeof(Yes) };
    };

    template <bool returnIt>
    struct EraseAndGetNext
    {
      template <typename T, typename I>
      static I erase(T & map, I it)
      {
        return map.erase(it);
      }
    };

    template <>
    struct EraseAndGetNext<false>
    {
      template <typename T, typename I>
      static I erase(T & map, I it)
      {
        I next = it;
        ++next;
        map.erase(it);
        return next;
      }
    };

    template <typename T, typename I>
    inline I map_erase_and_get_next_iterator(T & map, I it)
    {
      return EraseAndGetNext<CanEraseReturnIterator<T>::value>::erase(map, it);
    }

    template <typename T>
    struct PrintfFormatStr;

    template <> struct PrintfFormatStr<proto::int32>  { static const char * get() { return "%d"; }};
    template <> struct PrintfFormatStr<proto::uint32> { static const char * get() { return "%u"; }};
    template <> struct PrintfFormatStr<proto::int64>  { static const char * get() { return "%lld"; }};
    template <> struct PrintfFormatStr<proto::uint64> { static const char * get() { return "%llu"; }};
    template <> struct PrintfFormatStr<float> { static const char * get() { return "%f"; }};
    template <> struct PrintfFormatStr<double> { static const char * get() { return "%lf"; }};

    template <typename T>
    struct Value2StrWrapper
    {
      const char * valueToStr(T key)
      {
        snprintf(buffer, sizeof(buffer), PrintfFormatStr<T>::get(), key);
        buffer[sizeof(buffer) - 1] = 0;
        return buffer;
      }

      bool strToValue(const char * str, T & key) const
      {
        return sscanf(str, PrintfFormatStr<T>::get(), &key) == 1;
      }

      char buffer[64];
    };


    template <>
    struct Value2StrWrapper<proto::string>
    {
      const char * valueToStr(const proto::string & key) const
      {
        return str_cstr(key);
      }

      bool strToValue(const char * str, proto::string & key) const
      {
        key = str;
        return true;
      }
    };
  }
}

#endif // __GAIJIN_PROTO_CONTAINERSUTIL__
