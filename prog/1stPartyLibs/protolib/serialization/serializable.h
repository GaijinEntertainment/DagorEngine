#pragma once
#ifndef __GAIJIN_PROTOIO_SERIALIZABLE__
#define __GAIJIN_PROTOIO_SERIALIZABLE__

namespace proto
{
  typedef int version;

  const version DEFAULT_VERSION = -1;
  const version SAVE_ALL_VERSION = DEFAULT_VERSION - 1;

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
}

#endif // __GAIJIN_PROTOIO_SERIALIZABLE__
