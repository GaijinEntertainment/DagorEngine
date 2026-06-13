//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_resId.h>
#include <math/dag_TMatrix4.h>
#include <generic/dag_expected.h>
#include <EASTL/variant.h>

namespace refined_block
{

struct BindlessTexVar
{
  BaseTexture *tex;
};

using VarValue = eastl::variant<int, float, Color4, IPoint4, TMatrix4, BaseTexture *, BindlessTexVar, Sbuffer *>;

struct FlushedVar
{
  int stage;
  int slotOrBindlessId;
  VarValue var;
};

enum class GetError
{
  VarNotFound,
  WrongVarType,
};

enum class FlushError : uint8_t
{
  NoDump,
  InvalidStcodeId,
  EmptyStcode,
  CbufCreationFailed,
  CbufUpdateFailed
};

class GlobalBlockHandle;
class ViewBlockHandle;
class PassBlockHandle;
struct GlobalBlock;
struct ViewBlock;
struct PassBlock;

template <typename BlockHandle>
class BaseBlockHandle
{
  template <typename>
  friend class BaseBlockHandle;

  template <typename T>
  friend T stcode_get_from_block(const PassBlockHandle &block, int id);

  uint32_t id;
  template <typename T>
  dag::Expected<T, GetError> getByGid(int gid) const;

protected:
  BaseBlockHandle(uint32_t id) : id(id) {}

public:
  uint32_t getId() const { return id; }

  const char *getName() const;

  template <typename T>
  void set(int var_id, const T &val);

  template <typename T>
  dag::Expected<T, GetError> get(int var_id) const;
};

class GlobalBlockHandle : public BaseBlockHandle<GlobalBlockHandle>
{
public:
  using BlockType = GlobalBlock;
  using ChildHandle = ViewBlockHandle;

  GlobalBlockHandle() = delete;
  GlobalBlockHandle(const GlobalBlockHandle &) = default;
  GlobalBlockHandle &operator=(const GlobalBlockHandle &) = default;
  GlobalBlockHandle(GlobalBlockHandle &&) = default;
  GlobalBlockHandle &operator=(GlobalBlockHandle &&) = default;

  ViewBlockHandle refineBlock(const char *name);

private:
  friend GlobalBlockHandle get_global();

  GlobalBlockHandle(uint32_t id) : BaseBlockHandle<GlobalBlockHandle>(id) {}
};


class ViewBlockHandle : public BaseBlockHandle<ViewBlockHandle>
{
public:
  using BlockType = ViewBlock;
  using ParentHandle = GlobalBlockHandle;
  using ChildHandle = PassBlockHandle;

  ViewBlockHandle() = delete;
  ViewBlockHandle(const ViewBlockHandle &) = default;
  ViewBlockHandle &operator=(const ViewBlockHandle &) = default;
  ViewBlockHandle(ViewBlockHandle &&) = default;
  ViewBlockHandle &operator=(ViewBlockHandle &&) = default;

  PassBlockHandle refineBlock(const char *name);

  static ViewBlockHandle invalid;

private:
  template <typename ParentHandle>
  friend typename ParentHandle::ChildHandle refine_block(const char *name, ParentHandle parent);

  ViewBlockHandle(uint32_t id) : BaseBlockHandle<ViewBlockHandle>(id) {}
};

class PassBlockHandle : public BaseBlockHandle<PassBlockHandle>
{
public:
  using BlockType = PassBlock;
  using ParentHandle = ViewBlockHandle;

  PassBlockHandle() = delete;
  PassBlockHandle(const PassBlockHandle &) = default;
  PassBlockHandle &operator=(const PassBlockHandle &) = default;
  PassBlockHandle(PassBlockHandle &&) = default;
  PassBlockHandle &operator=(PassBlockHandle &&) = default;

  void setState() const;

  static PassBlockHandle invalid;

private:
  template <typename ParentHandle>
  friend typename ParentHandle::ChildHandle refine_block(const char *name, ParentHandle parent);

  friend void flush();

  PassBlockHandle(uint32_t id);
};

GlobalBlockHandle get_global();
void clear();
void flush();

} // namespace refined_block
