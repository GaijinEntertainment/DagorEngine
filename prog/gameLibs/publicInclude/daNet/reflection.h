//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/*
  usage example :

  class TestUnit : public danet::ReflectableObject
  {
  public:

    DECL_REFLECTION(TestUnit);

    //
    // reflection - serialize/deserialize changed var in serializeChangedReflectables/deserializeReflectables
    //

    // standart number declaration
    // Warning : unsigned type prefix ignored, if you need unsigned type use RVF_UNSIGNED flag explicitly
    REFL_VAR(int, test);

    // could freely interleaves with any other vars or methods
    String nonReflectVar;

    // extended syntax var def, params are:
    // type, var_name, addition flags, num bits to encode (0 for sizeof) and custom coder func (NULL for auto dispatch)
    REFL_VAR_EXT(float, extSyntaxVar, 0, 0, my_coder_func);

    // exclude prev var from reflection
    EXCLUDE_VAR(extSyntaxVar);

    // code float in range 0.f, 500.f in 16 bits
    // sizeof(theFloat) == sizeof(ReflectionVarMeta) + sizeof(float)*3
    REFL_FLOAT_MIN_MAX(theFloat, 0.f, 500.f, 0, 16)

  };

  //
  // for replication define as
  //
  class MyObject : public danet::ReplicatedObject
  {
  public:

    DECL_REPLICATION(MyObject)
  };

  // in *.cpp

  IMPLEMENT_REPLICATION(MyObject)

  // serialize data needed for object creation (this data passed on remote machine in createReplicatedObject())
  virtual void serializeReplicaCreationData(danet::BitStream& bs) const {}

  // this function create new client object (or get existing if it alreay exist (for example on host migration))
  danet::ReplicatedObject* MyObject::createReplicatedObject(danet::BitStream& bs) { return something; }

  to make serialization work:

  1) create object on server as usual
  2) call on server for that object MyObject->genReplicationEvent(myBitStream) and send that bitStream
      to remote machine manually (message id must be writed manually before function call)
  3) on remote machine (client) call ReplicatedObject::onRecvReplicationEvent for received bitStream
    (automatically will be called SomeObject::createReplicatedObject with serialized on server data)

  (same principle is apply for genReplicationEventForAll/onRecvReplicationEventForAll)

  Warning: change flag is not raised when you explicitly change field member in vector type (x,y,z...)
    like that

    reflVec.x = 1.f; // wrong

    do

    reflVec.getForModify().x = 1.f; // right

    instead
*/

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include "daNetTypes.h"
#include <string.h>
#include "bitStream.h"
#include <stddef.h> // for offsetof macros
#include <osApiWrappers/dag_critSec.h>

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>

#include "mpi.h"

// operation codes for coder func
#define DANET_REFLECTION_OP_ENCODE 0 // encode data
#define DANET_REFLECTION_OP_DECODE 1 // decode data

#define DANET_REFLECTION_MAX_VARS_PER_OBJECT 255
#define DANET_REPLICATION_MAX_CLASSES        256

class Point2;
class DPoint2;
class IPoint2;

class Point3;
class DPoint3;
class IPoint3;

class Point4;
class IPoint4;

class TMatrix;
class Quat;
class String;
class SimpleString;

// it's by design out of namespace (do not force user use our namespace name for reflection var flags declaration)
enum DanetReflectionVarFlags
{
  RVF_CHANGED = 1 << 0,
  // from serialization, but still deserializes (split on two flags?)
  RVF_EXCLUDED = 1 << 1,
  // to distinguish newly deserialized vars in ReflObjOnVarsDeserialized() callback (cleared after that)
  RVF_DESERIALIZED = 1 << 2,
  // for float - (-1.f .. 1.f), together with RVF_UNSIGNED - (0.f .. 1.f)
  RVF_NORMALIZED = 1 << 3,
  RVF_UNSIGNED = 1 << 4,
  // like RVF_CALL_HANDLER, but works even with RVF_EXCLUDED (or ReflectableObject::EXCLUDED) flag
  RVF_CALL_HANDLER_FORCE = 1 << 5,

  RVF_UNUSED3 = 1 << 6,

  // marked this flags on deserialize, then called onBeforeVarsDeserialization(), if this method clears this flag - that var will not
  // be deserialized
  RVF_NEED_DESERIALIZE = 1 << 7,
  // this flag used for float & vectors vars, for auto clamp
  RVF_MIN_MAX_SPECIFIED = 1 << 8,

  RVF_UNUSED0 = 1 << 9,
  RVF_UNUSED1 = 1 << 10,

  // call ReflectableObject::onReflectionVarChanged() method when var changed (rpc vars marked with this flag automatically)
  RVF_CALL_HANDLER = 1 << 11,

  RVF_UNUSED2 = 1 << 12,

  // dump serialization/deserialization of this
  RVF_DEBUG = 1 << 13,
  // this flas are user flag, not used directly in reflection but can be used for convenience
  // by user (in declaration & serialization)
  RVF_USER_FLAG = 1 << 14,
  RVF_UNRELIABLE = 1 << 15,
};

namespace danet
{
class ReflectionVarMeta;
class ReflectableObject;
class ReplicatedObject;

#define DANET_ENCODER_SIGNATURE int op, danet::ReflectionVarMeta *meta, const danet::ReflectableObject *ro, danet::BitStream *bs

typedef int (*reflection_var_encoder)(DANET_ENCODER_SIGNATURE);

template <typename T, bool mt_safe>
struct List
{
  T *head, *tail;
  List() : head(NULL), tail(NULL) {}
  void lock() {}
  void unlock() {}
};

extern WinCritSec reflection_lists_critical_section;

template <typename T>
struct List<T, true>
{
  T *head, *tail;

  List() : head(NULL), tail(NULL) {}
  void lock() { reflection_lists_critical_section.lock(); }
  void unlock() { reflection_lists_critical_section.unlock(); }
};

#define DANET_DLIST_PUSH(l, who, prevName, nextName) \
  {                                                  \
    l.lock();                                        \
    if (!who->prevName && (l).head != who)           \
    {                                                \
      if ((l).tail)                                  \
      {                                              \
        (l).tail->nextName = who;                    \
        who->prevName = (l).tail;                    \
        (l).tail = who;                              \
      }                                              \
      else                                           \
      {                                              \
        G_ASSERT(!(l).head);                         \
        (l).head = (l).tail = who;                   \
      }                                              \
    }                                                \
    l.unlock();                                      \
  }

#define DANET_DLIST_REMOVE(l, who, prevName, nextName) \
  {                                                    \
    l.lock();                                          \
    if (who->prevName)                                 \
      who->prevName->nextName = who->nextName;         \
    if (who->nextName)                                 \
      who->nextName->prevName = who->prevName;         \
    if ((l).head == who)                               \
      (l).head = (l).head->nextName;                   \
    if ((l).tail == who)                               \
      (l).tail = (l).tail->prevName;                   \
    who->prevName = who->nextName = NULL;              \
    l.unlock();                                        \
  }

// vars of this type are linked in one list inside of ReflectableObject
class ReflectionVarMeta
{
public:
  uint8_t persistentId;
  uint16_t flags;
  uint16_t numBits;
  const char *name;
  reflection_var_encoder coder;
  ReflectionVarMeta *next; // next var

  const char *getVarName() const { return name; }
  void *getValueRaw(size_t align) const
  {
    return (void *)(reinterpret_cast<const uint8_t *>(this) + ((sizeof(ReflectionVarMeta) + align - 1) / align) * align);
  }
  template <class T>
  T &getValue() const
  {
    return *(T *)getValueRaw(alignof(T));
  }
  void setChanged(bool f) { flags = f ? (flags | RVF_CHANGED) : (flags & ~RVF_CHANGED); }
};

struct ReplicatedObjectCreator
{
  int *class_id_ptr;
  const char *name;
  ReplicatedObject *(*create)(danet::BitStream &);
};

extern List<ReflectableObject, true> changed_reflectables;
extern List<ReflectableObject, true> all_reflectables;
extern ReplicatedObjectCreator registered_repl_obj_creators[DANET_REPLICATION_MAX_CLASSES];
extern int num_registered_obj_creators;

#define DANET_WATERMARK      _MAKE4C('DNET')
#define DANET_DEAD_WATERMARK _MAKE4C('DEAD')

// class, you need inherit all your objects from
// all this objects are linked in all_reflectables list, as well may be in changed_reflectables list
class ReflectableObject : public mpi::IObject
{
public:
  enum Flags
  {
    /* free */
    EXCLUDED = 2, // this object will not synchronized (but will deserializes)
    /* free */
    REPLICATED = 8,        // this is instance of ReplicatedObject
    FULL_SYNC = 16,        // for this objects forced full sync of all wars
    DEBUG_REFLECTION = 32, // like define for all vars of this object RVF_DEBUG
  };
  List<ReflectionVarMeta, false> varList;
  uint32_t debugWatermark;

  ReflectableObject *volatile prevChanged, *volatile nextChanged; // for changed_reflectables list
  ReflectableObject *volatile prev, *volatile next;               // for all_reflectables list

  void setRelfectionFlag(Flags flag) { reflectionFlags |= flag; }
  bool isRelfectionFlagSet(Flags flag) const { return (bool)(reflectionFlags & flag); }
  uint32_t getRelfectionFlags() const { return reflectionFlags; }

private:
  uint32_t reflectionFlags;

  // dummy mpi implementation
  virtual mpi::Message *dispatchMpiMessage(mpi::MessageID /*mid*/) { return NULL; }
  virtual void applyMpiMessage(const mpi::Message * /*m*/) {}

protected: // only childs can create and delete this class
  static const int EndReflVarQuotaNumber = 0;
  static const int SizeReflVarQuotaNumber = 64;

  ReflectableObject(mpi::ObjectID uid = mpi::INVALID_OBJECT_ID) :
    mpi::IObject(uid),
    reflectionFlags(EXCLUDED),
    prevChanged(NULL),
    nextChanged(NULL),
    prev(NULL),
    next(NULL),
    debugWatermark(DANET_WATERMARK)
  {}

  ~ReflectableObject()
  {
    checkWatermark();
    disableReflection(true);
    debugWatermark = DANET_DEAD_WATERMARK;
  }

public:
  void checkWatermark() const
  {
    if (debugWatermark != DANET_WATERMARK)
      DAG_FATAL("Reflection: invalid object 0x%p", this);
  }

  void enableReflection()
  {
    checkWatermark();

    if (reflectionFlags & EXCLUDED)
    {
      G_ASSERT(!prevChanged);
      G_ASSERT(!nextChanged);

      DANET_DLIST_PUSH(all_reflectables, this, prev, next);

      reflectionFlags &= ~EXCLUDED;
    }
  }

  void disableReflection(bool full)
  {
    checkWatermark();

    all_reflectables.lock();
    resetChangeFlag();
    if (full)
      DANET_DLIST_REMOVE(all_reflectables, this, prev, next);
    reflectionFlags |= EXCLUDED;
    all_reflectables.unlock();
  }

  int calcNumVars() const
  {
    checkWatermark();

    int r = 0;
    for (const ReflectionVarMeta *m = varList.head; m; m = m->next)
      ++r;
    return r;
  }

  const ReflectionVarMeta *searchVarByName(const char *name) const
  {
    checkWatermark();

    for (const ReflectionVarMeta *m = varList.head; m; m = m->next)
      if (strcmp(m->getVarName(), name) == 0)
        return m;
    return NULL;
  }

  int lookUpVarIndex(const ReflectionVarMeta *var) const
  {
    checkWatermark();

    int idx = 0;
    for (const ReflectionVarMeta *m = varList.head; m; m = m->next, ++idx)
      if (m == var)
        return idx;
    return -1;
  }

  ReflectionVarMeta *getVarByIndex(int idx) const
  {
    int i = 0;
    for (ReflectionVarMeta *m = varList.head; m; m = m->next, ++i)
      if (idx == i)
        return m;
    return NULL;
  }

  ReflectionVarMeta *getVarByPersistentId(int id) const
  {
    for (ReflectionVarMeta *m = varList.head; m; m = m->next)
      if (id == m->persistentId)
        return m;
    return NULL;
  }

  void resetChangeFlag()
  {
    checkWatermark();

    reflectionFlags &= ~FULL_SYNC;
    for (ReflectionVarMeta *m = varList.head; m; m = m->next)
      m->flags &= ~RVF_CHANGED;

    DANET_DLIST_REMOVE(changed_reflectables, this, prevChanged, nextChanged);
  }

  bool isReflObjChanged() const
  {
    return prevChanged || this == changed_reflectables.head; // TODO: circular linked list
  }

  void forceFullSync()
  {
    checkWatermark();

    if (reflectionFlags & EXCLUDED)
      return;

    for (ReflectionVarMeta *m = varList.head; m; m = m->next)
      if ((m->flags & RVF_EXCLUDED) == 0)
        m->flags |= RVF_CHANGED;

    reflectionFlags |= FULL_SYNC;

    markAsChanged();
  }

  void markAsChanged() { DANET_DLIST_PUSH(changed_reflectables, this, prevChanged, nextChanged); }

  void relocate(const ReflectableObject *old_ptr)
  {
    G_ASSERT(old_ptr);
    G_ASSERT(old_ptr != this);
    G_ASSERT((reflectionFlags & EXCLUDED) && (!prev && !next && all_reflectables.head != this)); // only allowed in disabled reflection

#define RELOC(x) (ReflectionVarMeta *)(uintptr_t(x) - uintptr_t(old_ptr) + uintptr_t(this))
    for (ReflectionVarMeta **m = &varList.head; *m; m = &(*m)->next)
      *m = RELOC(*m);
#undef RELOC
  }

  bool checkAndRemoveFromChangedList()
  {
    checkWatermark();

    for (ReflectionVarMeta *v = varList.head; v; v = v->next)
      if (v->flags & RVF_CHANGED)
        return false;

    reflectionFlags &= ~FULL_SYNC;
    DANET_DLIST_REMOVE(changed_reflectables, this, prevChanged, nextChanged);

    return true;
  }

  // set or unset flag for var (parameters for rpc function marked as well)
  void markVarWithFlag(danet::ReflectionVarMeta *var, uint16_t f, bool set);

  // return how many vars was serialized
  // reset RVF_CHANGED flag for variables and remove object from changed_reflectables list
  // (if none changed wars was left and do_reset_changed_flag==true)
  int serialize(BitStream &bs, uint16_t flags_to_have = RVF_CHANGED, bool fth_all = true, uint16_t flags_to_ignore = 0,
    bool fti_all = false, bool do_reset_changed_flag = true);
  virtual bool deserialize(BitStream &bs, int data_size);

  // called automatically before vars deserialization
  virtual void onBeforeVarsDeserialization() {}
  // called automatically after vars deserialization
  virtual void onAfterVarsDeserialization() {}
  // called after every var change (if RVF_CALL_HANDLER specified for it)
  virtual void onReflectionVarChanged(danet::ReflectionVarMeta *, void * /*newVal*/) {}
  // called before serialization (if returned false - no serialization performed, change flags discarded)
  virtual bool isNeedSerialize() const { return true; }
  // implemented automatically in DECL_REFLECTION macros
#if DAGOR_DBGLEVEL > 0
  virtual const char *getClassName() const = 0;
#else
  const char *getClassName() const { return ""; } // in release just stub
#endif
};

class ReplicatedObject : public ReflectableObject
{
public:
  ReplicatedObject(mpi::ObjectID uid = mpi::INVALID_OBJECT_ID) : ReflectableObject(uid) { setRelfectionFlag(REPLICATED); }

  bool isNetworkOwner() const { return !(isRelfectionFlagSet(EXCLUDED)) && (!prev || all_reflectables.head == this); }

  virtual int getClassId() const = 0; // implemented automatically in DECL_REFLECTION
  // serialize data needed for replica creation (this data is passed to createReplicatedObject())
  virtual void serializeReplicaCreationData(BitStream &bs) const = 0;

  // two proxy functions who create & receieve replication packet
  void genReplicationEvent(BitStream &out_bs) const;
  static ReplicatedObject *onRecvReplicationEvent(BitStream &bs);

  static void genReplicationEventForAll(BitStream &bs);
  static bool onRecvReplicationEventForAll(BitStream &bs);
};

// serialize changed vars, flags parameter means additional flags must exist (all of them)
// vars with RVF_EXCLUDED do not serializes
// accept flags that must exists in changed vars (all or any of that flags must exist ruled by bool)
// return how many objects was serialized (could be zero)
int serializeChangedReflectables(BitStream &bs, uint16_t flags_to_have = RVF_CHANGED, bool fth_all = true,
  uint16_t flags_to_ignore = 0, bool fti_all = false, bool do_reset_changed_flag = true);
int serializeAllReflectables(BitStream &bs, uint16_t flags_to_have = 0, bool fth_all = true, uint16_t flags_to_ignore = 0,
  bool fti_all = false, bool do_reset_changed_flag = true);
// return -1 on error, or how many reflectables was deserialized
int deserializeReflectables(BitStream &bs, mpi::object_dispatcher resolver);
int forceFullSyncForAllReflectables(); // return num reflectables
// this method is needed to guarantee independance of executable build process from replication mechanism
// (otherwise you need to keep build exactly the same for local and remote machines)
void normalizeReplication();

template <class _Ty>
class IsPtr;
template <class _Ty>
class IsPtr<_Ty *>
{
public:
  static constexpr int Value = 1;
  typedef _Ty Type;
};
template <class _Ty>
class IsPtr
{
public:
  static constexpr int Value = 0;
  typedef _Ty Type;
};

template <typename T>
struct DeRef
{
  typedef T Type;
};
template <typename T>
struct DeRef<const T &>
{
  typedef T Type;
};
template <typename T>
struct DeRef<T &>
{
  typedef T Type;
};

template <class Base, class Derived>
struct IsBase
{
  typedef char yes[2];
  typedef char no[1];

  template <class B>
  static yes &f(B *);
  template <class>
  static no &f(...);

  static constexpr int Value = (sizeof(f<Base>((Derived *)0)) == sizeof(yes));
};

// this class define base operations with reflection var
template <typename T, class EventListener>
class ReflectionVarBase : public ReflectionVarMeta
{
public:
  typedef T ElemType;

  ReflectionVarBase() : ReflectionVarMeta() { EventListener::OnVarCreated(this); }

  explicit ReflectionVarBase(const T &other) : ReflectionVarMeta()
  {
    EventListener::OnVarCreated(this);
    ReflectionVarMeta::getValue<T>() = other;
  }

  ReflectionVarBase(const ReflectionVarBase &other) :
    ReflectionVarBase(other.get()) {} // trivial copy ctor in order to make class non trivially copyable

  void markAsChanged(void *new_val = NULL) { EventListener::OnStateChanged(this, new_val ? new_val : getValueRaw(alignof(T))); }

  T &getForModify()
  {
    markAsChanged();
    return ReflectionVarMeta::getValue<T>();
  }

  const T &get() const { return ReflectionVarMeta::getValue<T>(); }
  operator const T &() const { return get(); }

  T operator->() const
  {
    G_STATIC_ASSERT(IsPtr<T>::Value == true);
    return ReflectionVarMeta::getValue<T>();
  }

  const T &Set(const T &new_val)
  {
    T &val = ReflectionVarMeta::getValue<T>();
    if (val != new_val)
    {
      markAsChanged((void *)&new_val);
      val = new_val;
    }
    return val;
  }

  template <typename T1>
  const T &operator=(const T1 &val)
  {
    return Set((const T)val);
  }

  template <typename T1>
  const T &operator=(const ReflectionVarBase<T1, EventListener> &val)
  {
    return Set((const T)val.ReflectionVarMeta::template getValue<T1>());
  }

  template <typename T1>
  bool operator==(const T1 &a) const
  {
    return ReflectionVarMeta::getValue<T>() == a;
  }

  template <typename T1>
  bool operator!=(const T1 &a) const
  {
    return ReflectionVarMeta::getValue<T>() != a;
  }

  template <typename T1>
  bool operator==(const ReflectionVarBase<T1, EventListener> &a) const
  {
    return ReflectionVarMeta::getValue<T>() == a.template getValue<T1>();
  }

  template <typename T1>
  bool operator!=(const ReflectionVarBase<T1, EventListener> &a) const
  {
    return ReflectionVarMeta::getValue<T>() != a.template getValue<T1>();
  }
};

// this class actually contains data for all types except vectors
template <typename T, class EventListener>
class ReflectionVarScalar : public ReflectionVarBase<T, EventListener>
{
public:
  typedef ReflectionVarBase<T, EventListener> BaseClass;

  ReflectionVarScalar() : ReflectionVarBase<T, EventListener>() {}

  explicit ReflectionVarScalar(const T &other) : ReflectionVarBase<T, EventListener>(other) {}

  template <typename T1>
  const T &operator=(const T1 &val)
  {
    return BaseClass::Set((const T)val);
  }

  template <typename T1>
  const T &operator=(const ReflectionVarBase<T1, EventListener> &val)
  {
    return BaseClass::Set((const T)val.ReflectionVarMeta::template getValue<T1>());
  }

  T value;
};

// this class defines operations for numeric types
template <typename T, class EventListener>
class ReflectionVarNumber : public ReflectionVarScalar<T, EventListener>
{
public:
  typedef ReflectionVarScalar<T, EventListener> BaseClass;

  ReflectionVarNumber() : ReflectionVarScalar<T, EventListener>() {}

  explicit ReflectionVarNumber(const T &other) : ReflectionVarScalar<T, EventListener>(other) {}

  template <typename T1>
  const T &operator=(const T1 &val)
  {
    return BaseClass::Set((const T)val);
  }

  template <typename T1>
  const T &operator=(const ReflectionVarNumber<T1, EventListener> &val)
  {
    return BaseClass::Set((const T)val.ReflectionVarMeta::template getValue<T1>());
  }

  template <typename T1>
  const T &operator+=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() + (const T)val);
  }

  template <typename T1>
  const T &operator-=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() - (const T)val);
  }

  template <typename T1>
  const T &operator/=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() / (const T)val);
  }

  template <typename T1>
  const T &operator*=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() * (const T)val);
  }

  template <typename T1>
  const T &operator^=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() ^ (const T)val);
  }

  template <typename T1>
  const T &operator|=(const T1 &val)
  {
    return BaseClass::Set(ReflectionVarMeta::getValue<T>() | (const T)val);
  }

  const T &operator++() { return (*this += 1); }

  T operator--() { return (*this -= 1); }

  T operator++(int) // postfix
  {
    T val = ReflectionVarMeta::getValue<T>();
    *this += 1;
    return val;
  }

  T operator--(int) // postfix
  {
    T val = ReflectionVarMeta::getValue<T>();
    *this -= 1;
    return val;
  }
};

inline bool is_const_ptr(void *) { return false; }
inline bool is_const_ptr(const void *) { return true; }

// this class defines some common operations for vector types
// WARNING : we implicitly assume that vector types ctor does nothing
template <typename T, class EventListener>
class ReflectionVarVectorBase : public ReflectionVarBase<T, EventListener>
{
public:
  typedef ReflectionVarBase<T, EventListener> BaseClass;

  ReflectionVarVectorBase() : ReflectionVarBase<T, EventListener>() {}

  explicit ReflectionVarVectorBase(const T &other) : ReflectionVarBase<T, EventListener>(other) {}

#define PROXY_METHOD(ret, methname, args_decl, args, attrs)                                        \
  inline ret methname(args_decl) attrs                                                             \
  {                                                                                                \
    if (!is_const_ptr(this))                                                                       \
      EventListener::OnStateChanged((ReflectionVarMeta *)this, &ReflectionVarMeta::getValue<T>()); \
    return ReflectionVarMeta::getValue<T>().methname(args);                                        \
  }

  PROXY_METHOD(const float &, operator[], int i, i, const)
  PROXY_METHOD(float &, operator[], int i, i, )

  PROXY_METHOD(, operator const float *, , , const)
  PROXY_METHOD(, operator float *, , , )

  PROXY_METHOD(T, operator-, , , const)
  PROXY_METHOD(T, operator+, , , const)

  PROXY_METHOD(Point2, operator+, const T &a, a, const)
  PROXY_METHOD(Point2, operator-, const T &a, a, const)
  PROXY_METHOD(float, operator*, const T &a, a, const)
  PROXY_METHOD(T, operator*, float a, a, const)
  PROXY_METHOD(T, operator/, float a, a, const)

  PROXY_METHOD(T &, operator+=, const T &a, a, )
  PROXY_METHOD(T &, operator-=, const T &a, a, )
  PROXY_METHOD(T &, operator*=, float a, a, )
  PROXY_METHOD(T &, operator/=, float a, a, )

  PROXY_METHOD(float, lengthSq, , , const)
  PROXY_METHOD(float, length, , , const)
  PROXY_METHOD(void, normalize, , , )
  PROXY_METHOD(float, lengthF, , , const)
  PROXY_METHOD(void, normalizeF, , , )
  PROXY_METHOD(void, zero, , , )
};

template <typename T, class EventListener>
struct ReflectionVarTypeDispatcher // default type
{
  typedef ReflectionVarScalar<T, EventListener> VarType;
};

#define DANET_COMMA ,

#define DECL_VEC_CLASS(typ, decl, additional_decl)                                                           \
  template <class EventListener>                                                                             \
  class ReflectionVarVector##typ : public ReflectionVarVectorBase<typ, EventListener>                        \
  {                                                                                                          \
  public:                                                                                                    \
    typedef ReflectionVarVectorBase<typ, EventListener> BaseClass;                                           \
    ReflectionVarVector##typ() : ReflectionVarVectorBase<typ, EventListener>()                               \
    {                                                                                                        \
      G_STATIC_ASSERT(sizeof(*this) == (sizeof(typ) + sizeof(ReflectionVarMeta)));                           \
    }                                                                                                        \
    explicit ReflectionVarVector##typ(const typ &other) : ReflectionVarVectorBase<typ, EventListener>(other) \
    {}                                                                                                       \
    const typ &operator=(const typ &val)                                                                     \
    {                                                                                                        \
      return BaseClass::Set(val);                                                                            \
    }                                                                                                        \
    const typ &operator=(const ReflectionVarVector##typ<EventListener> &val)                                 \
    {                                                                                                        \
      return BaseClass::Set(val.BaseClass::template getValue<typ>());                                        \
    }                                                                                                        \
    struct ArrayElem                                                                                         \
    {                                                                                                        \
      decl                                                                                                   \
    };                                                                                                       \
    decl;                                                                                                    \
    additional_decl                                                                                          \
  };                                                                                                         \
  template <class EventListener>                                                                             \
  struct ReflectionVarTypeDispatcher<typ, EventListener>                                                     \
  {                                                                                                          \
    typedef ReflectionVarVector##typ<EventListener> VarType;                                                 \
  };

#define _MC BaseClass::markAsChanged() // mark changed

DECL_VEC_CLASS(
  Point2, float x; float y;,

  template <class T> static Point2 xy(const T &a) { return Point2(a.x DANET_COMMA a.y); } template <class T>
  static Point2 xz(const T &a) { return Point2(a.x DANET_COMMA a.z); } template <class T>
  static Point2 yz(const T &a) { return Point2(a.y DANET_COMMA a.z); }

  template <class T>
  void set_xy(const T &a) {
    _MC;
    x = a.x;
    y = a.y;
  } template <class T>
  void set_xz(const T &a) {
    _MC;
    x = a.x;
    y = a.z;
  } template <class T>
  void set_yz(const T &a) {
    _MC;
    x = a.y;
    y = a.z;
  })

DECL_VEC_CLASS(
  DPoint2, double x; double y;,
  template <class T> static DPoint2 xy(const T &a) { return DPoint2(a.x DANET_COMMA a.y); } template <class T>
  static DPoint2 xz(const T &a) { return DPoint2(a.x DANET_COMMA a.z); } template <class T>
  static DPoint2 yz(const T &a) { return DPoint2(a.y DANET_COMMA a.z); }

  template <class T>
  void set_xy(const T &a) {
    _MC;
    x = a.x;
    y = a.y;
  } template <class T>
  void set_xz(const T &a) {
    _MC;
    x = a.x;
    y = a.z;
  } template <class T>
  void set_yz(const T &a) {
    _MC;
    x = a.y;
    y = a.z;
  })

DECL_VEC_CLASS(
  IPoint2, int x; int y;,
  template <class T> static IPoint2 xy(const T &a) { return IPoint2((int)a.x DANET_COMMA(int) a.y); } template <class T>
  static IPoint2 xz(const T &a) { return IPoint2((int)a.x DANET_COMMA(int) a.z); } template <class T>
  static IPoint2 yz(const T &a) { return IPoint2((int)a.y DANET_COMMA(int) a.z); }

  template <class T>
  void set_xy(const T &a) {
    _MC;
    x = (int)a.x;
    y = (int)a.y;
  } template <class T>
  void set_xz(const T &a) {
    _MC;
    x = (int)a.x;
    y = (int)a.z;
  } template <class T>
  void set_yz(const T &a) {
    _MC;
    x = (int)a.y;
    y = (int)a.z;
  })

DECL_VEC_CLASS(Point3, float x; float y; float z;, )
DECL_VEC_CLASS(DPoint3, double x; double y; double z;, )
DECL_VEC_CLASS(IPoint3, int x; int y; int z;, )

DECL_VEC_CLASS(Point4, float x; float y; float z; float w;, )
DECL_VEC_CLASS(IPoint4, int x; int y; int z; int w;, )

#undef CHECK_VEC_CLASS_SIZE
#undef DECL_VEC_CLASS
#undef _MC

#undef PROXY_METHOD

template <typename T>
int readwrite_var_raw(DANET_ENCODER_SIGNATURE)
{
  if (op == DANET_REFLECTION_OP_ENCODE)
    bs->Write(meta->getValue<T>());
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    if (meta->flags & RVF_NEED_DESERIALIZE)
    {
      if (!(meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE)))
        return bs->Read(meta->getValue<T>());
      else
      {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4700)
#endif
        T new_val;
        if (!bs->Read(new_val))
          return 0;
        T &val = meta->getValue<T>();
        if (val != new_val)
        {
          const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &new_val);
          val = new_val;
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
      }
    }
    else
    {
      T tempVar;
      bs->Read(tempVar);
    }
  }
  return 1;
}

template <typename T>
int readwrite_var_bytes(DANET_ENCODER_SIGNATURE) // like readwrite_var_raw but data is not converted to/from big endian (writes as is)
{
  if (op == DANET_REFLECTION_OP_ENCODE)
    bs->Write((const char *)&meta->getValue<T>(), sizeof(T));
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    if (meta->flags & RVF_NEED_DESERIALIZE)
    {
      if (!(meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE)))
        return bs->Read((char *)&meta->getValue<T>(), sizeof(T));
      else
      {
        T new_val;
        if (!bs->Read(new_val))
          return 0;
        T &val = meta->getValue<T>();
        if (memcmp(&val, &new_val, sizeof(T)) != 0)
        {
          const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &new_val);
          memcpy(&val, &new_val, sizeof(T));
        }
      }
    }
    else
      bs->IgnoreBytes(sizeof(T));
  }
  return 1;
}

template <typename T>
static inline reflection_var_encoder get_encoder_for(T *) // default coder (instantiated when no overloaded function found)
{
  return &readwrite_var_raw<T>;
}

template <typename T>
constexpr inline T c_expr(T value)
{
  return value;
}

// dummy encoder - does nothing
static inline int empty_encoder(DANET_ENCODER_SIGNATURE)
{
  (void)op;
  (void)meta;
  (void)ro;
  (void)bs;
  return 1;
}

#define DANET_DEFINE_ALIAS_CODER_FOR_TYPE(f_name, typ)               \
  namespace danet                                                    \
  {                                                                  \
  static inline danet::reflection_var_encoder get_encoder_for(typ *) \
  {                                                                  \
    return &f_name;                                                  \
  }                                                                  \
  }

#define DANET_DEFINE_CODER_FOR_TYPE(f_name, typ) \
  namespace danet                                \
  {                                              \
  extern int f_name(DANET_ENCODER_SIGNATURE);    \
  }                                              \
  DANET_DEFINE_ALIAS_CODER_FOR_TYPE(f_name, typ)

// like DANET_DEFINE_CODER_FOR_TYPE but defines coder in global namespace
#define DANET_DEFINE_EXTERN_CODER_FOR_TYPE(f_name, typ) \
  extern int f_name(DANET_ENCODER_SIGNATURE);           \
  DANET_DEFINE_ALIAS_CODER_FOR_TYPE(f_name, typ)

#if DAGOR_DBGLEVEL > 0
#define DANET_DEF_GET_DEBUG_NAME(class_name)                                               \
  const char *getClassName() const override                                                \
  {                                                                                        \
    G_STATIC_ASSERT(danet::IsBase<danet::ReflectableObject DANET_COMMA ThisClass>::Value); \
    return #class_name;                                                                    \
  }
#else
#define DANET_DEF_GET_DEBUG_NAME(class_name)
#endif

#define DECL_REFLECTION(class_name, base_class)                                              \
  typedef base_class BaseClass;                                                              \
  typedef class_name ThisClass;                                                              \
  static const int StartReflVarQuotaNumber = BaseClass::EndReflVarQuotaNumber + 1;           \
  static const int EndReflVarQuotaNumber = StartReflVarQuotaNumber + SizeReflVarQuotaNumber; \
  DANET_DEF_GET_DEBUG_NAME(class_name)

#define DECL_REPLICATION_FOOTER(class_name)                                               \
  static int you_forget_to_put_IMPLEMENT_REPLICATION_4_##class_name;                      \
  int getClassId() const override                                                         \
  {                                                                                       \
    G_STATIC_ASSERT(danet::IsBase<danet::ReplicatedObject DANET_COMMA ThisClass>::Value); \
    return you_forget_to_put_IMPLEMENT_REPLICATION_4_##class_name;                        \
  }                                                                                       \
  void serializeReplicaCreationData(danet::BitStream &bs) const override;                 \
  static danet::ReplicatedObject *createReplicatedObject(danet::BitStream &bs);

#define DECL_REPLICATION(class_name, base_class) \
  DECL_REFLECTION(class_name, base_class)        \
  DECL_REPLICATION_FOOTER(class_name)

#define DECL_REPLICATION_NO_REFLECTION(class_name) \
  typedef class_name ThisClass;                    \
  DANET_DEF_GET_DEBUG_NAME(class_name)             \
  DECL_REPLICATION_FOOTER(class_name)

#define DECL_REFLECTION_STUB       \
  const char *getClassName() const \
  {                                \
    return "";                     \
  }

#define DECL_REPLICATION_STUB                                          \
  DECL_REFLECTION_STUB                                                 \
  int getClassId() const override                                      \
  {                                                                    \
    return -1;                                                         \
  }                                                                    \
  void serializeReplicaCreationData(danet::BitStream &) const override \
  {}

#define IMPLEMENT_REPLICATION_BODY(class_name, str_name, templ_name, pref)                                           \
  pref int class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name = -1;                                  \
  static struct ReplicationTypeRegistator4_##class_name                                                              \
  {                                                                                                                  \
    ReplicationTypeRegistator4_##class_name()                                                                        \
    {                                                                                                                \
      G_ASSERT(danet::num_registered_obj_creators < DANET_REPLICATION_MAX_CLASSES);                                  \
      class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name = danet::num_registered_obj_creators;       \
      danet::ReplicatedObjectCreator &c = danet::registered_repl_obj_creators[danet::num_registered_obj_creators++]; \
      c.class_id_ptr = &class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name;                          \
      c.create = &class_name::createReplicatedObject;                                                                \
      c.name = #str_name;                                                                                            \
    }                                                                                                                \
  } replication_registrator_4_##class_name;

#define IMPLEMENT_REPLICATION(class_name)                   IMPLEMENT_REPLICATION_BODY(class_name, class_name, class_name, )
#define IMPLEMENT_REPLICATION_TEMPL(class_name, templ_name) IMPLEMENT_REPLICATION_BODY(class_name, class_name, templ_name, template <>)

#define DANET_DECL_NUM_TYPE(typ)                             \
  template <class EventListener>                             \
  struct ReflectionVarTypeDispatcher<typ, EventListener>     \
  {                                                          \
    typedef ReflectionVarNumber<typ, EventListener> VarType; \
  };

DANET_DECL_NUM_TYPE(char)
DANET_DECL_NUM_TYPE(uint8_t)
DANET_DECL_NUM_TYPE(short)
DANET_DECL_NUM_TYPE(uint16_t)
DANET_DECL_NUM_TYPE(int)
DANET_DECL_NUM_TYPE(uint32_t)
DANET_DECL_NUM_TYPE(int64_t)
DANET_DECL_NUM_TYPE(uint64_t)
DANET_DECL_NUM_TYPE(float)
DANET_DECL_NUM_TYPE(double)

#undef DANET_DECL_NUM_TYPE

void on_reflection_var_created(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, uint8_t persistent_id,
  const char *var_name, uint16_t var_flags, uint16_t var_bits);
void on_reflection_var_changed(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, void *new_val);

#define DANET_STD_ENCODER(t) meta->coder = danet::get_encoder_for((t *)0);
#define DANET_RAW_ENCODER(t) meta->coder = danet::readwrite_var_raw<t>;
#define DANET_ENCODER(t)     meta->coder = t;

#define REFL_VAR_DECL(num, decl_typ, dispatch_typ, var_name, var_flags, var_bits, coder_f_init, additional_decl, init_code, templ) \
  class EventListenerFor_##num                                                                                                     \
  {                                                                                                                                \
    G_STATIC_ASSERT(1 <= danet::c_expr(num) && num <= SizeReflVarQuotaNumber);                                                     \
                                                                                                                                   \
  public:                                                                                                                          \
    static inline void OnVarCreated(danet::ReflectionVarMeta *meta)                                                                \
    {                                                                                                                              \
      danet::ReflectableObject *robj = (danet::ReflectableObject *)((char *)meta - offsetof(ThisClass, var_name));                 \
      on_reflection_var_created(meta, robj, StartReflVarQuotaNumber + num, #var_name, var_flags,                                   \
        var_bits ? var_bits : BYTES_TO_BITS(sizeof(dispatch_typ)));                                                                \
      coder_f_init init_code                                                                                                       \
    }                                                                                                                              \
    static inline void OnStateChanged(danet::ReflectionVarMeta *meta, void *new_val)                                               \
    {                                                                                                                              \
      danet::ReflectableObject *robj = (danet::ReflectableObject *)((char *)meta - offsetof(ThisClass, var_name));                 \
      on_reflection_var_changed(meta, robj, new_val);                                                                              \
    }                                                                                                                              \
  };                                                                                                                               \
  templ danet::ReflectionVarTypeDispatcher<decl_typ, EventListenerFor_##num>::VarType var_name;                                    \
  additional_decl

#define REFL_VAR(num, typ, var_name) REFL_VAR_DECL(num, typ, typ, var_name, 0, 0, DANET_STD_ENCODER(typ), , , )

#define REFL_VAR_UNRELIABLE(num, typ, var_name) REFL_VAR_DECL(num, typ, typ, var_name, RVF_UNRELIABLE, 0, DANET_STD_ENCODER(typ), , , )

#define REFL_VAR_EXT(num, typ, var_name, var_flags, var_bits, coder_f) \
  REFL_VAR_DECL(num, typ, typ, var_name, var_flags, var_bits, coder_f, , , )

#define REFL_FLOAT_MIN_MAX(num, var_name, var_min, var_max, var_flags, var_bits)                                      \
  REFL_VAR_DECL(num, float, float, var_name, (var_flags) | RVF_MIN_MAX_SPECIFIED, var_bits, DANET_STD_ENCODER(float), \
                float min_max_for##var_name[2];                                                                       \
                , G_ASSERT(var_min < var_max); float *fa = &meta->getValue<float>() + 1; fa[0] = var_min; fa[1] = var_max;, )

#define REFL_FVEC_MIN_MAX(num, typ, var_name, var_min, var_max, var_flags, var_bits)                                                \
  REFL_VAR_DECL(                                                                                                                    \
    num, typ, typ, var_name, (var_flags) | RVF_MIN_MAX_SPECIFIED, var_bits, DANET_STD_ENCODER(typ), float min_max_for##var_name[2]; \
    , G_ASSERT(var_min < var_max); float *fa = (float *)(&meta->getValue<typ>() + 1); fa[0] = var_min; fa[1] = var_max;, )

#define EXCLUDE_VAR(var_name)                                                                                                    \
  class ExcludeVarFor##var_name##Class                                                                                           \
  {                                                                                                                              \
  public:                                                                                                                        \
    ExcludeVarFor##var_name##Class()                                                                                             \
    {                                                                                                                            \
      danet::ReflectableObject *robj = (danet::ReflectableObject *)((char *)this - offsetof(ThisClass, exclude##var_name##Var)); \
      DANET_CHECK_WATERMARK(robj);                                                                                               \
      danet::ReflectionVarMeta *nameMeta = (danet::ReflectionVarMeta *)robj->searchVarByName(#var_name);                         \
      if (nameMeta)                                                                                                              \
        nameMeta->flags |= RVF_EXCLUDED;                                                                                         \
      else                                                                                                                       \
        G_ASSERT(0 && "can't find var for exclusion " #var_name);                                                                \
    }                                                                                                                            \
  };                                                                                                                             \
  ExcludeVarFor##var_name##Class exclude##var_name##Var;

}; // namespace danet

DANET_DEFINE_CODER_FOR_TYPE(default_fvec_coder, Point4);
DANET_DEFINE_ALIAS_CODER_FOR_TYPE(default_fvec_coder, Quat);
DANET_DEFINE_ALIAS_CODER_FOR_TYPE(default_fvec_coder, Point3);
DANET_DEFINE_ALIAS_CODER_FOR_TYPE(default_fvec_coder, Point2);
DANET_DEFINE_CODER_FOR_TYPE(default_tm_coder, TMatrix);
DANET_DEFINE_CODER_FOR_TYPE(default_int_coder, int);
DANET_DEFINE_ALIAS_CODER_FOR_TYPE(default_int_coder, unsigned);
DANET_DEFINE_CODER_FOR_TYPE(default_float_coder, float);

#undef PROXY_METHOD

// min/max specialization
#undef min
#undef max
template <typename T, class EventListener>
inline T min(const T a, const danet::ReflectionVarBase<T, EventListener> &b)
{
  return min<T>(a, (T)b);
}
template <typename T, class EventListener>
inline T max(const T a, const danet::ReflectionVarBase<T, EventListener> &b)
{
  return max<T>(a, (T)b);
}

template <typename T, class EventListener>
inline T min(const danet::ReflectionVarBase<T, EventListener> &a, const T b)
{
  return min<T>((T)a, b);
}
template <typename T, class EventListener>
inline T max(const danet::ReflectionVarBase<T, EventListener> &a, const T b)
{
  return max<T>((T)a, b);
}

// clamp specialization
template <typename T, class EventListener>
inline T clamp(const danet::ReflectionVarBase<T, EventListener> &t, const T a, const T b)
{
  return clamp((T)t, a, b);
}
template <typename T, class EventListener>
inline T clamp(const T t, const danet::ReflectionVarBase<T, EventListener> &a, const T b)
{
  return clamp(t, (T)a, b);
}
template <typename T, class EventListener>
inline T clamp(const T t, const T a, const danet::ReflectionVarBase<T, EventListener> &b)
{
  return clamp(t, a, (T)b);
}
