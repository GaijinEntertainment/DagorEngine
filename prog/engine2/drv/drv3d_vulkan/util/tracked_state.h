#pragma once
#include <EASTL/bitset.h>
#include "type_lists.h"
#include <debug/dag_debug.h>

#define VULKAN_TRACKED_STATE_FIELD_CB_NON_CONST_DEFENITIONS() \
  template <typename Storage, typename Context>               \
  void applyTo(Storage &data, Context &target);               \
  template <typename Storage>                                 \
  void dumpLog(Storage &data) const;                          \
  template <typename Storage, typename Context>               \
  void transit(Storage &data, Context &target) const

#define VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS()   \
  template <typename Storage, typename Context>       \
  void applyTo(Storage &data, Context &target) const; \
  template <typename Storage>                         \
  void dumpLog(Storage &data) const;                  \
  template <typename Storage, typename Context>       \
  void transit(Storage &data, Context &target) const

#define VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS()             \
  template <typename Storage, typename Context>                       \
  void applyTo(uint32_t index, Storage &data, Context &target) const; \
  template <typename Storage>                                         \
  void dumpLog(uint32_t index, Storage &data) const;                  \
  template <typename Storage, typename Context>                       \
  void transit(uint32_t index, Storage &data, Context &target) const

#define VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS() \
  template <typename T>                               \
  T &ref();                                           \
  template <typename T>                               \
  T &refRO() const;                                   \
  template <typename T>                               \
  void applyTo(T &) const                             \
  {}                                                  \
  template <typename T>                               \
  void transit(T &) const                             \
  {}                                                  \
  void makeDirty();                                   \
  void clearDirty();

#define VULKAN_TRACKED_STATE_FIELD_REF(RetType, FieldName, StorageType) \
  template <>                                                           \
  RetType &StorageType::ref()                                           \
  {                                                                     \
    return FieldName;                                                   \
  }                                                                     \
  template <>                                                           \
  const RetType &StorageType::refRO() const                             \
  {                                                                     \
    return FieldName;                                                   \
  }

#define VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB_NO_RESET() \
  template <typename Storage, typename Context>                 \
  void applyTo(Storage &, Context &target)                      \
  {                                                             \
    TrackedState::apply(target);                                \
  };                                                            \
  template <typename Storage>                                   \
  void dumpLog(Storage &) const                                 \
  {                                                             \
    TrackedState::dumpLog();                                    \
  };                                                            \
  template <typename Storage, typename Context>                 \
  void transit(Storage &, Context &target)                      \
  {                                                             \
    TrackedState::transit(target);                              \
  }                                                             \
  static constexpr bool is_nested_field()                       \
  {                                                             \
    return true;                                                \
  }

#define VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB()    \
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB_NO_RESET() \
  template <typename T>                                   \
  void reset(T &)                                         \
  {                                                       \
    TrackedState::reset();                                \
  }

template <typename Field>
struct TrackedStateIndexedDataRORef
{
  uint32_t index;
  const Field &val;
};

template <typename T, uint32_t N, bool AllowDiff, bool BranchOnDiff>
struct TrackedStateFieldArray
{
  TrackedStateFieldArray() = default;
  static constexpr bool allow_diff() { return AllowDiff; }
  static constexpr bool branch_on_diff() { return BranchOnDiff; }
  static constexpr bool is_nested_field() { return true; }

  T data[N];
  eastl::bitset<N> df;

  template <typename Storage>
  void reset(Storage &storage)
  {
    df.reset();
    for (uint32_t i = 0; i < N; ++i)
      data[i].reset(storage);
  }

  template <typename InputType>
  void set(const InputType &value)
  {
    data[value.index].set(value.val);
    df.set(value.index);
  }

  template <typename InputType>
  bool diff(const InputType &value) const
  {
    return data[value.index].diff(value.val);
  }

  void set(uint32_t index_to_mark_dirty) { df.set(index_to_mark_dirty); }
  bool diff(uint32_t index_to_mark_dirty) const { return !df.test(index_to_mark_dirty); }

  template <typename Storage, typename Context>
  void applyTo(Storage &storage, Context &target)
  {
    if (!df.any())
      return;

    for (uint32_t i = 0; i < N; ++i)
      if (df.test(i))
        data[i].applyTo(i, storage, target);
    df.reset();
  }

  template <typename Storage>
  void dumpLog(Storage &storage) const
  {
    for (uint32_t i = 0; i < N; ++i)
      data[i].dumpLog(i, storage);
  }

  template <typename Storage, typename Context>
  void transit(Storage &storage, Context &target)
  {
    if (!df.any())
      return;

    for (uint32_t i = 0; i < N; ++i)
      if (df.test(i))
        data[i].transit(i, storage, target);
    df.reset();
  }

  T &getValue() { return data[0]; }
  const T &getValueRO() const { return data[0]; }

  void makeDirty(uint32_t index) { df.set(index); }
  void makeDirty() { df.set(); }
  void clearDirty() { df.reset(); }
  bool isDirty() const { return df.any(); }
  static uint32_t size() { return N; }
};

template <bool AllowDiff, bool BranchOnDiff>
struct TrackedStateFieldBase
{
  TrackedStateFieldBase() = default;
  static constexpr bool allow_diff() { return AllowDiff; }
  static constexpr bool branch_on_diff() { return BranchOnDiff; }
  static constexpr bool is_nested_field() { return false; }
};

template <typename PtrType>
struct TrackedStateFieldGenericPtr
{
  PtrType *ptr;

  template <typename StorageType>
  void reset(StorageType &)
  {
    ptr = nullptr;
  }

  void set(PtrType *value) { ptr = value; }
  bool diff(PtrType *value) const { return ptr != value; }
};

template <typename PodType>
struct TrackedStateFieldGenericSmallPOD
{
  PodType data;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = PodType{};
  }

  void set(PodType value) { data = value; }
  bool diff(PodType value) const { return data != value; }
};

template <typename PodType>
struct TrackedStateFieldGenericPOD
{
  PodType data;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = PodType{};
  }

  void set(const PodType &value) { data = value; }
  bool diff(const PodType &value) const { return data != value; }
  PodType &getValue() { return data; }
};

template <typename HandleType>
struct TrackedStateFieldGenericTaggedHandle
{
  HandleType handle;

  template <typename StorageType>
  void reset(StorageType &)
  {
    handle = HandleType::Null();
  }

  void set(HandleType value) { handle = value; }
  bool diff(HandleType value) const { return handle != value; }
};

template <typename HandleType>
struct TrackedStateFieldGenericWrappedHandle
{
  HandleType handle;

  template <typename StorageType>
  void reset(StorageType &)
  {
    handle = HandleType();
  }

  void set(HandleType value) { handle = value; }
  bool diff(HandleType value) const { return handle != value; }
};


template <typename T, bool AllowDiff, bool BranchOnDiff>
struct TrackedStateField : public T, public TrackedStateFieldBase<AllowDiff, BranchOnDiff>
{};

template <typename StateStorage, typename... FieldTypes>
class TrackedState
{
  typedef TrackedState<StateStorage, FieldTypes...> ThisType;
  typedef eastl::bitset<sizeof...(FieldTypes)> DiffBits;
  typedef TypePack<FieldTypes...> FieldTypesPack;

  StateStorage data;
  DiffBits diff;
  DiffBits nestedFields;
  bool applyDisabled = false;

  template <typename Ctx, typename Cb, typename FieldType>
  void callOpRO(Ctx &ctx, Cb &cb) const
  {
    const FieldType &ref = data.template refRO<const FieldType>();
    cb(ctx, ref);
  }

  template <typename T, typename U>
  void dispatchRO(uint32_t tid, T &ctx, U &&cb) const
  {
    typedef void (ThisType::*CallType)(T &, U &) const;
    static const CallType callList[] = {&ThisType::callOpRO<T, U, FieldTypes>...};

    (this->*(callList[tid]))(ctx, cb);
  }

  template <typename Ctx, typename Cb, typename FieldType>
  void callOp(Ctx &ctx, Cb &cb)
  {
    FieldType &ref = data.template ref<FieldType>();
    cb(ctx, ref);
  }

  template <typename T, typename U>
  void dispatch(uint32_t tid, T &ctx, U &&cb)
  {
    typedef void (ThisType::*CallType)(T &, U &);
    static const CallType callList[] = {&ThisType::callOp<T, U, FieldTypes>...};

    (this->*(callList[tid]))(ctx, cb);
  }

protected:
  StateStorage &getData() { return data; }
  const StateStorage &getDataRO() const { return data; }

public:
  TrackedState()
  {
    for (int i = 0; i != sizeof...(FieldTypes); ++i)
      dispatch(i, nestedFields, [i](DiffBits &ctx, auto &data_ref) {
        if (data_ref.is_nested_field())
          ctx.set(i);
      });
  }

  template <typename T, typename Data>
  bool set(const Data &v)
  {
    bool ret = false;
    constexpr uint32_t tid = TypeIndexOf<T, FieldTypesPack>::value;
    T &ref = data.template ref<T>();
    if (T::allow_diff())
    {
      if (T::branch_on_diff())
      {
        if (!diff.test(tid))
        {
          ret = ref.diff(v);
          if (!ret)
            return ret;
          diff.set(tid, ret);
        }
      }
      else
      {
        DiffBits nb;
        ret = ref.diff(v);
        nb.set(tid, ret);
        diff |= nb;
      }
    }
    else
    {
      ret = true;
      diff.set(tid, 1);
    }

    ref.set(v);

    return ret;
  }

  template <typename Target, typename Data, typename A, typename... Ts>
  bool set(const Data &v)
  {
    A &ref = data.template ref<A>();
    bool changed = ref.template set<Target, Data, Ts...>(v);

    DiffBits nb;
    constexpr uint32_t tid = TypeIndexOf<A, FieldTypesPack>::value;
    nb.set(tid, changed);
    diff |= nb;
    return changed;
  }

  template <typename T>
  void makeFieldDirty()
  {
    constexpr uint32_t tid = TypeIndexOf<T, FieldTypesPack>::value;
    diff.set(tid, 1);
  }

  template <typename Target, typename A, typename... Ts>
  void makeFieldDirty()
  {
    A &ref = data.template ref<A>();
    ref.template makeFieldDirty<Target, Ts...>();

    constexpr uint32_t tid = TypeIndexOf<A, FieldTypesPack>::value;
    diff.set(tid, 1);
  }

  template <typename T>
  bool changed()
  {
    constexpr uint32_t tid = TypeIndexOf<T, FieldTypesPack>::value;
    return diff.test(tid);
  }

  // FIXME:temporal solution to port old code faster
  // gets should not be available
  template <typename T, typename Data>
  Data &get()
  {
    T &ref = data.template ref<T>();
    return ref.getValue();
  }

  template <typename Target, typename Data, typename A, typename... Ts>
  Data &get()
  {
    A &ref = data.template ref<A>();
    return ref.template get<Target, Data, Ts...>();
  }

  template <typename T, typename Data>
  const Data &getRO() const
  {
    const T &ref = data.template refRO<const T>();
    return ref.getValueRO();
  }

  template <typename Target, typename Data, typename A, typename... Ts>
  const Data &getRO() const
  {
    const A &ref = data.template refRO<const A>();
    return ref.template getRO<Target, Data, Ts...>();
  }

  template <typename T, typename Data>
  void set_raw(const Data &v)
  {
    constexpr uint32_t tid = TypeIndexOf<T, FieldTypesPack>::value;
    T &ref = data.template ref<T>();
    diff.set(tid, true);
    ref.set(v);
  }

  template <typename Target, typename Data, typename A, typename... Ts>
  void set_raw(const Data &v)
  {
    A &ref = data.template ref<A>();
    ref.template set_raw<Target, Data, Ts...>(v);

    DiffBits nb;
    constexpr uint32_t tid = TypeIndexOf<A, FieldTypesPack>::value;
    diff.set(tid, true);
  }

  template <typename T>
  void transit(T &target)
  {
    if (diff.any())
      for (int i = 0; i != sizeof...(FieldTypes); ++i)
        if (diff.test(i))
          dispatch(i, target, [this](T &ctx, auto &data_ref) { data_ref.transit(this->data, ctx); });
    data.transit(target);
    diff.reset();
  }

  template <typename T>
  void apply(T &target)
  {
    if (applyDisabled)
      return;

    DiffBits ldiff = diff | nestedFields;

    if (ldiff.any())
      for (int i = 0; i != sizeof...(FieldTypes); ++i)
        if (ldiff.test(i))
          dispatch(i, target, [this](T &ctx, auto &data_ref) { data_ref.applyTo(this->data, ctx); });
    data.applyTo(target);
    diff.reset();
  }

  void disableApply(bool v) { applyDisabled = v; }

  // note: if nested field changes directly (via stored pointer or get)
  // dirty flag will be not visible on parent as this does not drop into nested fields code
  bool isDirty() const { return diff.any(); }
  void makeDirty()
  {
    diff.set();
    // for proper nested fields processing
    data.makeDirty();
  }

  void clearDirty()
  {
    diff.reset();
    // for proper nested fields processing
    data.clearDirty();
  }

  void reset()
  {
    for (int i = 0; i != sizeof...(FieldTypes); ++i)
      dispatch(i, data, [](StateStorage &ctx, auto &data_ref) { data_ref.reset(ctx); });
    data.reset();
    diff.set();
  }

  void dumpLog() const
  {
    debug("==");
    for (int i = 0; i != sizeof...(FieldTypes); ++i)
    {
      bool dirty = diff.test(i);
      bool nested = nestedFields.test(i);
      dispatchRO(i, data, [i, dirty, nested](const StateStorage &ctx, auto &data_ref) {
        debug("%i [ %s %s ]", i, dirty ? "dirty" : "intact", nested ? "nested" : "regular");
        data_ref.dumpLog(ctx);
      });
    }
    data.dumpLog();
  }
};
