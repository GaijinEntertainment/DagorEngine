//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <ioSys/dag_dataBlock.h>

MAKE_TYPE_FACTORY(DataBlock, DataBlock);
MAKE_TYPE_FACTORY(SharedDataBlock, eastl::shared_ptr<DataBlock>);
MAKE_TYPE_FACTORY(DataBlockReadFlags, dblk::ReadFlags);

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::DataBlock::ParamType, DataBlockParamType);
DAS_BIND_ENUM_CAST(dblk::ReadFlag);
DAS_BASE_BIND_ENUM_FACTORY(dblk::ReadFlag, "DataBlockReadFlag");

namespace das
{
template <>
struct ToBasicType<dblk::ReadFlags>
{
  static constexpr int type = Type::tUInt8;
};
template <>
struct cast<dblk::ReadFlags>
{
  static __forceinline dblk::ReadFlags to(vec4f x) { return dblk::ReadFlag(v_extract_xi(v_cast_vec4i(x))); }
  static __forceinline vec4f from(dblk::ReadFlags x) { return v_cast_vec4f(v_splatsi(x.asInteger())); }
};

template <typename FuncT, FuncT fun>
struct das_call_member_no_ref2;

template <typename R, typename CC, typename First, typename Second, typename... Args, R (CC::*func)(First, Second, Args...)>
struct das_call_member_no_ref2<R (CC::*)(First, Second, Args...), func>
{
  static_assert(std::is_reference_v<Second> && std::is_const_v<std::remove_reference_t<Second>>, "Second parameter must be const T&");
  __forceinline static R invoke(CC &THIS, First first, std::remove_reference_t<Second> second, Args... rest)
  {
    return ((THIS).*(func))(std::forward<First>(first), second, std::forward<Args>(rest)...);
  }
};

template <typename R, typename CC, typename First, typename Second, typename... Args, R (CC::*func)(First, Second, Args...) const>
struct das_call_member_no_ref2<R (CC::*)(First, Second, Args...) const, func>
{
  static_assert(std::is_reference_v<Second> && std::is_const_v<std::remove_reference_t<Second>>, "Second parameter must be const T&");
  __forceinline static R invoke(const CC &THIS, First first, std::remove_reference_t<Second> second, Args... rest)
  {
    return ((THIS).*(func))(std::forward<First>(first), second, std::forward<Args>(rest)...);
  }
};

} // namespace das

class DataBlock;
class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
class TMatrix;
struct E3DCOLOR;

namespace bind_dascript
{
void datablock_debug_print_datablock(const char *name, const DataBlock &blk);
const char *datablock_to_string(const DataBlock &blk, das::Context *context, das::LineInfoArg *at);
const char *datablock_to_compact_string(const DataBlock &blk, das::Context *context, das::LineInfoArg *at);
const DataBlock &datablock_get_empty();
void datablock_set_from(DataBlock &blk, const DataBlock &from, const char *src_fname);
} // namespace bind_dascript
