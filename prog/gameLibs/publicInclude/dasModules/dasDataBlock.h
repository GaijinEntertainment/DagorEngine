//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>

MAKE_TYPE_FACTORY(DataBlock, DataBlock);
MAKE_TYPE_FACTORY(SharedDataBlock, eastl::shared_ptr<DataBlock>);
MAKE_TYPE_FACTORY(DataBlockReadFlags, dblk::ReadFlags);

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::DataBlock::ParamType, DataBlockParamType);
DAS_BIND_ENUM_CAST(dblk::ReadFlag);

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
