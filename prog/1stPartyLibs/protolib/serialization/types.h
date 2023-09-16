#pragma once
#ifndef __GAIJIN_PROTOIO_TYPES__
#define __GAIJIN_PROTOIO_TYPES__

class DataBlock;

namespace proto
{
  typedef int int32;
  typedef unsigned int uint32;

  typedef long long int int64;
  typedef unsigned long long int uint64;

  typedef int version;

  const version DEFAULT_VERSION = -1;
  const version SAVE_ALL_VERSION = DEFAULT_VERSION - 1;

  class BinarySaveOptions
  {
  public:
    explicit BinarySaveOptions(int number_, bool saveExtKey_ = true)
      : number(number_)
      , saveExtKey(saveExtKey_)
    {
    }


    int getNumber() const
    {
      return number;
    }

    bool needSaveExtKey() const
    {
      return saveExtKey;
    }
  private:
    int number;
    bool saveExtKey;
  };

  template <typename T>
  inline const char * str_cstr(const T & value)
  {
    return value.c_str();
  }

  template <typename T>
  inline size_t str_size(const T & value)
  {
    return value.size();
  }

  template <typename T>
  inline void str_assign(T & value, const char * src, size_t size)
  {
    value.assign(src, size);
  }

  class Serializable
  {
  };

  class SerializableVersioned : public Serializable
  {
  public:
    SerializableVersioned()
      : curVersion(DEFAULT_VERSION)
    {
    }

    virtual ~SerializableVersioned()
    {
    }

    version getVersion() const
    {
      return curVersion;
    }

    void touch(version _version)
    {
      curVersion = _version;
    }

    virtual void touchRecursive(version _version)
    {
      touch(_version);
    }
  private:
    version curVersion;
  };

  template <typename T>
  class ContainerVersioned :
    public T,
    public SerializableVersioned
  {
  public:
    ContainerVersioned()
      : structureVersion(DEFAULT_VERSION)
    {
    }

    version getStructureVersion() const
    {
      return structureVersion;
    }

    void touchStructure(version version)
    {
      structureVersion = version;
      touch(version);
    }

    virtual void touchRecursive(version _version)
    {
      touchStructure(_version);
      touchAllElements((const typename T::value_type*)NULL, _version);
    }

  private:
    void touchAllElements(const SerializableVersioned *, version version)
    {
      for (typename T::iterator it = T::begin(); it != T::end(); ++it)
      {
        (*it).touchRecursive(version);
      }
    }

    void touchAllElements(const void *, version version)
    {
      for (typename T::iterator it = T::begin(); it != T::end(); ++it)
      {
        it->second.touchRecursive(version);
      }
    }

    version structureVersion;
  };


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


  template <typename T>
  T get_enum_from_str(const char * value, T defaultValue)
  {
    T result(defaultValue);
    if (!str_to_enum(value, result))
    {
      result = defaultValue;
    }

    return result;
  }
}


#endif // __GAIJIN_PROTOIO_TYPES__
