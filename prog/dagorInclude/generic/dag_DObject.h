//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/// @addtogroup common
/// @{

/// @addtogroup baseclasses
/// @{

/** @file
  Dynamic object with reference count garbage collector and RTTI
*/

// #define DEBUG_DOBJECTS
#ifndef DEBUG_DOBJECTS
#include <debug/dag_fatal.h>
#endif
#include <osApiWrappers/dag_atomic.h>
#include <dag/dag_relocatable.h>

#include <supp/dag_define_COREIMP.h>

/**
  Class id for objects derived from DObject.

  ids<0x1000 are reserved for Dagor core.

  Use random number generator for your own ids and declare them
  using #decl_dclassid() macro, to allow duplicate id check in the future.
*/
class DClassID
{
public:
  unsigned id;
  constexpr DClassID(unsigned a) : id(a) {}

  bool operator==(DClassID a) const { return id == a.id; }
  bool operator!=(DClassID a) const { return id != a.id; }
};

/**
  Dagor base dynamic object with reference count garbage collector and run-time class identification.
*/
class DObject
{
public:
  /** @fn destroy()
    Destroy all linked objects and set pointers to NULL.
    This method should be used to avoid lost DObjects.
  */

  /** @fn addRef()
    Increments reference count
  */

  /** @fn delRef()
    Decrements reference count
  */

#ifdef DEBUG_DOBJECTS
  KRNLIMP DObject();
  KRNLIMP DObject(const DObject &src) : ref_count(0), dobject_flags(src.dobject_flags) {}

  KRNLIMP virtual ~DObject();
  KRNLIMP void addRef();
  KRNLIMP void delRef();
  KRNLIMP virtual void destroy();

  /// Start debugging this DObject
  KRNLIMP virtual void startdebug();

  /// Stop debugging this DObject
  KRNLIMP virtual void stopdebug();
#else
  DObject() { ref_count = 0; }
  DObject(const DObject & /*src*/) : ref_count(0) {}

  virtual ~DObject() {}

  void addRef() { interlocked_increment(ref_count); }
  void delRef()
  {
    if (interlocked_decrement(ref_count) == 0)
      delete this;
  }
  void delRef(IMemAlloc *m)
  {
    if (interlocked_decrement(ref_count) == 0)
      memdelete(this, m);
  }

  virtual void destroy() {}
#endif

  /// returns current reference count
  int getRefCount() { return ref_count; }

  /// returns non-zero if this object is a sub-class of the specified class
  KRNLIMP virtual bool isSubOf(DClassID id);

  /// class name used for debugging
  KRNLIMP virtual const char *class_name() const;

  /// returns specified interface (ids and interfaces are user-defined)
  KRNLIMP virtual void *get_interface(int id);

protected:
  /// reference count
  volatile int ref_count;

#ifdef DEBUG_DOBJECTS
  /// flags used for debugging
  int dobject_flags;
#elif _TARGET_64BIT
  int _resv;
#endif
};

/**
  "smart" pointer to object derived from DObject.
  It automatically handles reference counter increments/decrements.
  You can use it as a normal pointer, but it also has some additional methods.
*/
template <class T>
class Ptr
{
public:
  Ptr(const Ptr &p) { init(p.ptr); }
  Ptr(Ptr &&p) : ptr(p.ptr) { p.ptr = nullptr; }
  Ptr(T *p = NULL) { init(p); }
  ~Ptr()
  {
    T *t = ptr;
    ptr = NULL;
    (t) ? t->delRef() : (void)0;
  }

  /// cast operators
  operator T *() const { return ptr; }
  T &operator*() const { return *ptr; }
  T *operator->() const { return ptr; }

  Ptr &operator=(const Ptr &p)
  {
    T *t = ptr;
    ptr = p.ptr;
    addRef();
    if (t)
      t->delRef();
    return *this;
  }

  Ptr &operator=(T &&other)
  {
    T *tmp = ptr;
    ptr = other.ptr;
    other.ptr = tmp;
    return *this;
  }

  Ptr &operator=(T *p)
  {
    T *t = ptr;
    ptr = p;
    addRef();
    if (t)
      t->delRef();
    return *this;
  }

  void reset(IMemAlloc *m)
  {
    T *t = ptr;
    ptr = NULL;
    if (t)
      t->delRef(m);
  }

  /// explicit cast to base type
  T *get() const { return ptr; }

  /// increment reference count
  __forceinline void addRef()
  {
    if (ptr)
      ptr->addRef();
  }

  /// decrement reference count
  __forceinline void delRef()
  {
    if (ptr)
      ptr->delRef();
  }

  /// initialize pointer. Use if constructor was not called for some reason.
  __forceinline void init(T *p = NULL)
  {
    ptr = p;
    addRef();
  }

  /// call destroy() on object and set pointer to NULL
  void destroy()
  {
    if (ptr)
    {
      T *t = ptr;
      ptr = NULL;
      t->destroy();
      t->delRef();
    }
  }

  inline bool operator==(T *other_ptr) const { return ptr == other_ptr; }

protected:
  T *ptr;
};

template <typename T>
struct dag::is_type_relocatable<Ptr<T>, void> : public eastl::true_type
{};

/// "smart" pointer to #DObject
typedef Ptr<DObject> DObjectPtr;

/// Macro that should be used for declaration of #DObject class ids.
//-V:decl_dclassid:1043
#define decl_dclassid(id, classname) static constexpr DClassID classname##CID(id)

/// 'auto' declare class and some other stuff
#define decl_ptr(cn) typedef Ptr<class cn> cn##Ptr;

#define decl_dclass_hdr(classname, baseclass)            \
  decl_ptr(classname) class classname : public baseclass \
  {                                                      \
  public:

#define decl_class_name(classname)        \
  const char *class_name() const override \
  {                                       \
    return #classname;                    \
  }

#define decl_issubof(classname, baseclass)                 \
  bool isSubOf(DClassID id) override                       \
  {                                                        \
    return id == classname##CID || baseclass::isSubOf(id); \
  }

#define decl_dclass(cn, bc) decl_dclass_hdr(cn, bc) decl_class_name(cn) decl_issubof(cn, bc)

#define decl_dclass_and_id(cn, bc, id) \
  decl_dclassid(id, cn);               \
  decl_dclass(cn, bc)

#define end_dclass_decl() }

/// Null #DClassID
decl_dclassid(0, Null);

/// #DClassID for base #DObject
decl_dclassid(1, DObject);

KRNLIMP void check_lost_dobjects();

#include <supp/dag_undef_COREIMP.h>

/// @}
/// @}
