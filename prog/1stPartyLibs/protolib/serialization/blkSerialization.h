#pragma once
#ifndef __GAIJIN_PROTOIO_BLKSERIALIZATION__
#define __GAIJIN_PROTOIO_BLKSERIALIZATION__

#include <ioSys/dag_dataBlock.h>

#include <protoConfig.h>


namespace proto
{
  namespace io
  {
    template <typename T>
    struct BlkValueWrapper;

#define DECLARE_BLKWRAPPER(TYPE, FN)                                                          \
    template <>                                                                               \
    struct BlkValueWrapper<TYPE>                                                              \
    {                                                                                         \
      static inline void addValue(DataBlock & blk, const char * name, TYPE value)             \
      {                                                                                       \
        blk.add##FN(name, value);                                                             \
      }                                                                                       \
      static inline void setValue(DataBlock & blk, const char * name, TYPE value)             \
      {                                                                                       \
        blk.set##FN(name, value);                                                             \
      }                                                                                       \
      static inline void getValue(DataBlock & blk, const char * name, TYPE & value, int def)  \
      {                                                                                       \
        value = blk.get##FN(name, def);                                                       \
      }                                                                                       \
      static inline void getValue(DataBlock & blk, int idx, TYPE & value)                     \
      {                                                                                       \
        value = blk.get##FN(idx);                                                             \
      }                                                                                       \
    }

    DECLARE_BLKWRAPPER(proto::int32, Int);
    DECLARE_BLKWRAPPER(proto::uint32, Int);
    DECLARE_BLKWRAPPER(proto::int64, Int64);
    DECLARE_BLKWRAPPER(proto::uint64, Int64);
    DECLARE_BLKWRAPPER(bool, Bool);
    DECLARE_BLKWRAPPER(float, Real);
#undef DECLARE_BLKWRAPPER

    template<>
    struct BlkValueWrapper<proto::string>
    {
      static inline void addValue(DataBlock & blk, const char * name, const proto::string & value)
      {
        blk.addStr(name, str_cstr(value));
      }

      static inline void setValue(DataBlock & blk, const char * name, const proto::string & value)
      {
        blk.setStr(name, str_cstr(value));
      }

      static inline void getValue(
          DataBlock & blk,
          const char * name,
          proto::string & value,
          const char* def)
      {
        const char * str = blk.getStr(name, def);
        G_ASSERT(str != NULL);
        value = str;
      }

      static inline void getValue(DataBlock & blk, int idx, proto::string & value)
      {
        const char * str = blk.getStr(idx);
        G_ASSERT(str != NULL);
        value = str;
      }
    };
  }
}

#endif // __GAIJIN_PROTOIO_BLKSERIALIZATION__
