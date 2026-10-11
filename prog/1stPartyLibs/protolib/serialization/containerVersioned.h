#pragma once
#ifndef __GAIJIN_PROTOIO_CONTAINERVERSIONED__
#define __GAIJIN_PROTOIO_CONTAINERVERSIONED__

#include <cstddef>
#include "serializable.h"

namespace proto
{
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
}

#endif // __GAIJIN_PROTOIO_CONTAINERVERSIONED__
