//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <dag/dag_relocatable.h>
#include <memory/dag_mem.h>

class DagorAsset;


// asset reference provider interface
class IDagorAssetRefProvider
{
public:
  enum
  {
    RFLG_EXTERNAL = 0x0001, //< ref is external and must be exported as refslot to be bound on gameres loading
    RFLG_OPTIONAL = 0x0002, //< ref slot is optional (refAsset may be NULL)
    RFLG_BROKEN = 0x0004,   //< ref is broken - refAsset cannot be resolved by asset name
  };

  struct Ref
  {
    DagorAsset *refAsset = nullptr;
    int flags = 0;

    Ref() = default;
    Ref(Ref &&a) : refAsset(a.refAsset), flags(a.flags)
    {
      a.refAsset = nullptr;
      a.flags = 0;
    }
    Ref(const Ref &a) : Ref() { operator=(a); }
    ~Ref() { free(); }
    Ref &operator=(const Ref &a)
    {
      free();
      refAsset = a.refAsset;
      flags = a.flags & ~RFLG_BROKEN;
      if (const char *bn = a.getBrokenRef())
        setBrokenRef(bn);
      return *this;
    }
    Ref &operator=(Ref &&a)
    {
      auto r = refAsset;
      auto f = flags;
      refAsset = a.refAsset;
      flags = a.flags;
      a.refAsset = r;
      a.flags = f;
      return *this;
    }

    DagorAsset *getAsset() const { return (flags & RFLG_BROKEN) ? NULL : refAsset; }
    const char *getBrokenRef() const { return (flags & RFLG_BROKEN) ? (char *)refAsset : NULL; }

    void setBrokenRef(const char *name)
    {
      free();
      flags |= RFLG_BROKEN;
      refAsset = (DagorAsset *)str_dup(name, tmpmem);
    }
    void free()
    {
      if ((flags & RFLG_BROKEN) && refAsset)
      {
        memfree(refAsset, tmpmem);
        refAsset = NULL;
        flags &= ~RFLG_BROKEN;
      }
    }
  };

public:
  virtual const char *__stdcall getRefProviderIdStr() const = 0;

  virtual const char *__stdcall getAssetType() const = 0;

  virtual void __stdcall onRegister() = 0;
  virtual void __stdcall onUnregister() = 0;

  //! resolves references and returns referenced assets and refslot attributes
  virtual void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &out_refs) = 0;

  //! resolves references and returns referenced assets and refslot attributes (for specified target and profile)
  virtual void __stdcall getAssetRefsEx(DagorAsset &a, Tab<Ref> &out_refs, unsigned /*target*/, const char * /*profile*/)
  {
    getAssetRefs(a, out_refs);
  }
};
DAG_DECLARE_RELOCATABLE(IDagorAssetRefProvider::Ref);
