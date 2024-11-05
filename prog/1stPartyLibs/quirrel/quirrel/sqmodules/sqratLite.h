#pragma once

#include <squirrel.h>


#if 0

  #include <util/dag_globDef.h>

  #define SQRAT_ASSERT G_ASSERT
  #define SQRAT_ASSERTF G_ASSERTF
  #define SQRAT_VERIFY G_VERIFY

  #define SQRAT_HAS_EASTL
  #define SQRAT_STD eastl

#else

  #include <assert.h>

  #define SQRAT_ASSERT assert
  #define SQRAT_ASSERTF(cond, msg, ...) assert((cond) && (msg))
  #define SQRAT_VERIFY(cond) do { if (!(cond)) assert(#cond); } while(0)

  #define SQRAT_STD std

#endif

template <typename T>
void SQRAT_UNUSED(const T&) {}


#if defined(SQRAT_HAS_EASTL)
# include <EASTL/string.h>
# include <EASTL/string_view.h>
# include <EASTL/unordered_map.h>
# include <EASTL/unordered_set.h>
# include <EASTL/vector_map.h>
# include <EASTL/shared_ptr.h>
EA_DISABLE_ALL_VC_WARNINGS()
#else
# include <string>
# include <unordered_map>
# include <unordered_set>
# include <memory>
# include <tuple>
# include <type_traits>
# if __cplusplus >= 201703L
# include <string_view>
# endif
#endif

namespace Sqrat
{

#if defined(SQRAT_HAS_EASTL)
  using string = eastl::basic_string<SQChar>;
  using string_view = eastl::basic_string_view<SQChar>;
  template <class T> using hash = eastl::hash<T>;
  template <class T> using shared_ptr = eastl::shared_ptr<T>;
  template <class T> using weak_ptr = eastl::weak_ptr<T>;

#else
  using string = std::basic_string<SQChar>;
  template <class T> using hash = std::hash<T>;
  template <class T> using shared_ptr = std::shared_ptr<T>;
  template <class T> using weak_ptr = std::weak_ptr<T>;

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) // c++17
  using string_view = std::basic_string_view<SQChar>;
#else
  using string_view = string;
#endif
#endif //defined(SQRAT_HAS_EASTL)



struct Object
{
    HSQUIRRELVM vm = nullptr;
    HSQOBJECT obj = {OT_NULL, 0};
    ~Object() { sq_release(vm, &obj); }
    Object() : vm(nullptr) { sq_resetobject(&obj); }
    Object(HSQOBJECT ho, HSQUIRRELVM vm_) : vm(vm_), obj(ho) { sq_addref(vm, &obj); }
    Object(const Object &ptr) : vm(ptr.vm), obj(ptr.obj) { sq_addref(vm, &obj); }
    Object(Object &&ptr) { sq_release(vm, &obj); vm=ptr.vm; obj=ptr.obj; sq_resetobject(&ptr.obj); }
    Object& operator=(const Object &ptr) { vm = ptr.vm; obj = ptr.obj; sq_addref(vm, &obj); return *this; }
    Object& operator=(Object &&ptr) { sq_release(vm, &obj); vm=ptr.vm; obj=ptr.obj; sq_resetobject(&ptr.obj); return *this; }

    HSQOBJECT GetObject() const { return obj; }
    HSQOBJECT& GetObject() { return obj; }

    void Release()
    {
        sq_release(vm, &obj);
        sq_resetobject(&obj);
    }
};

template <typename T>
struct Var
{
};

template <>
struct Var<Object>
{
    Object value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as an Object
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT sqValue;
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &sqValue)));
        value = Object(sqValue, vm);
    }
};


}
