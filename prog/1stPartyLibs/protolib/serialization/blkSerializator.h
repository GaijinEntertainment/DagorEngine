#pragma once
#ifndef __GAIJIN_PROTOIO_BLKSERIALIZATOR__
#define __GAIJIN_PROTOIO_BLKSERIALIZATOR__

#include "serializable.h"

class DataBlock;

namespace proto
{
  class BlkSerializator
  {
  public:
    BlkSerializator(DataBlock & blk_,
                    version version_ = SAVE_ALL_VERSION,
                    bool skipKeyField_ = false)
      : blk(blk_)
      , checkVersion(version_)
      , skipKeyField(skipKeyField_)
    {

    }

    BlkSerializator(const BlkSerializator & ser,
                    DataBlock & blk_,
                    bool skipKeyField_ = false)
      : blk(blk_)
      , checkVersion(ser.checkVersion)
      , skipKeyField(skipKeyField_)
    {
    }

    DataBlock & blk;

    bool checkSerialize(int version) const
    {
      return version > checkVersion;
    }

    template <typename T>
    bool checkSerialize(const T & obj) const
    {
      return checkSerializeInternal(&obj);
    }

    bool isSkipKeyField() const
    {
      return skipKeyField;
    }

  private:

    bool checkSerializeInternal(const SerializableVersioned * obj) const
    {
      return checkSerialize(obj->getVersion());
    }

    bool checkSerializeInternal(const void * /*obj*/) const
    {
      // serialize all nonversioned objects
      return true;
    }

    const version checkVersion;
    const bool    skipKeyField;
  };
}

#endif // __GAIJIN_PROTOIO_BLKSERIALIZATOR__
