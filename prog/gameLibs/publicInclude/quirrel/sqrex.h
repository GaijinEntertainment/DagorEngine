//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <sqrat.h>
#include <EASTL/optional.h>
#include <EASTL/type_traits.h>

namespace sqrex
{

class VMContext
{
private:
  HSQUIRRELVM vm;
  Sqrat::RootTable vmRoot;

public:
  VMContext() = delete; // no default constructor. VM must be always initialized

  explicit VMContext(HSQUIRRELVM vm_) : vm(vm_), vmRoot(vm_) {}

  class TableContext
  {
    Sqrat::Table table;

    TableContext(Sqrat::Table &&tbl) : table(tbl) {}

    TableContext(Sqrat::Table &tbl) : table(tbl) {}

    // This constructor is not needed in C++17 with guaranted copy ellision
    // allowing to return non-copyable and non-movable objects by value
    TableContext(TableContext &&rhs) : table(rhs.table) {}

    TableContext(const TableContext &) = delete;
    void operator=(const TableContext &) = delete;
    void operator=(TableContext &&) = delete;

    friend class VMContext;
    friend TableContext within(Sqrat::Table);
    friend TableContext within(HSQUIRRELVM, const char *);

  public:
    template <typename R, typename... Args>
    eastl::optional<R> eval(const char *sq_func, const Args &...args)
    {
      Sqrat::Function func = table.GetFunction(sq_func);
      R result;
      if (!func.IsNull() && func.Evaluate(args..., result))
        return eastl::make_optional(eastl::move(result));
      return eastl::optional<R>();
    }

    template <typename R, typename... Args>
    R try_eval(const R &result_def, const char *sq_func, const Args &...args)
    {
      Sqrat::Function func = table.GetFunction(sq_func);
      R result{};
      if (!func.IsNull() && func.Evaluate(args..., result))
        return result;
      return result_def;
    }

    template <typename... Args>
    bool exec(const char *sq_func, const Args &...args)
    {
      Sqrat::Function func = table.GetFunction(sq_func);
      if (func.IsNull())
        return false;
      return func.Execute(args...);
    }
  };

  TableContext at(const char *table_name) { return TableContext(vmRoot.GetSlot(table_name)); }

  TableContext at(Sqrat::Table &&table) { return TableContext(table); }

  TableContext at(Sqrat::Table &table) { return TableContext(table); }

  /*
   * Evaluates script function and places result into first argument
   * Returns calling success
   */
  template <typename R, typename... Args>
  eastl::optional<R> eval(const char *sq_func, const Args &...args)
  {
    return at(vmRoot).eval<R>(sq_func, args...);
  }

  /*
   * Evaluates script function and returns result
   * On failure returns default result passed in first argument
   */
  template <typename R, typename... Args>
  R try_eval(const R &result_def, const char *sq_func, const Args &...args)
  {
    return at(vmRoot).try_eval<R>(result_def, sq_func, args...);
  }

  /*
   * Execute script function with given arguments
   * Returns calling success
   */
  template <typename... Args>
  bool exec(const char *sq_func, const Args &...args)
  {
    return at(vmRoot).exec(sq_func, args...);
  }

  /*
   * Set value in const or root table
   */
  template <typename T>
  VMContext &bind_const(const T var, const char *const_name) // builder pattern
  {
#ifdef USE_CONST_TABLE
    Sqrat::ConstTable(vm).Const(const_name, (int64_t)var);
#else
    vmRoot.SetValue(const_name, (int64_t)var);
#endif
    return *this;
  }

  /*
   * Script objects constructors
   */

  Sqrat::Object make_null() { return Sqrat::Object(vm); }

  Sqrat::Array make_array(int size = 0) { return Sqrat::Array(vm, size); }

  Sqrat::Table make_table() { return Sqrat::Table(vm); }

  Sqrat::Function make_func(Sqrat::Object env, Sqrat::Object closure) { return Sqrat::Function(vm, env, closure); }

  Sqrat::RootTable root() { return vmRoot; }
};

inline VMContext within(HSQUIRRELVM vm) { return VMContext(vm); }

inline VMContext::TableContext within(Sqrat::Table table) { return VMContext(table.GetVM()).at(table); }

inline VMContext::TableContext within(HSQUIRRELVM vm, const char *table_name) { return VMContext(vm).at(table_name); }

} // namespace sqrex
