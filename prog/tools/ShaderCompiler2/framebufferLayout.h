// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include <EASTL/string.h>
#include <generic/dag_tab.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace shc
{
class TargetContext;
}

struct Parser;

namespace BaseParNamespace
{
struct Terminal;
}

namespace ShaderTerminal
{
struct framebuffer_layout_decl;
}

class RenderPassTable;
class RenderPassGroupTable;
class RenderPassObject;
class RenderPassGroup;

struct RenderPassNameAdapter
{
  static const char *getName(int id);
  static int addName(const char *name);
};
using RenderPassNameId = NameId<RenderPassNameAdapter>;

class FramebufferLayoutObject
{
public:
  struct Entry
  {
    RenderPassNameId name;
    uint32_t format = 0;
    int samples = 1;
  };

  RenderPassNameId name;
  int tableIdx = -1;
  SerializableTab<Entry> entries;
  SerializableTab<RenderPassNameId> members;

  BINDUMP_NON_SERIALIZABLE(const VarNameMap *names = nullptr;);

  const char *str(int name_id) const { return names->getName(name_id); }
  const char *nameStr() const { return str(name); }
  bool isGroup() const { return !members.empty(); }
  const Entry *findEntry(int name_id) const;
  eastl::string shapeDesc() const;
};

template <class T>
class RenderPassNamedTable
{
protected:
  VarNameMap &mNames;
  Tab<T *> mItems;
  ska::flat_hash_map<int, T *> mByName;

  void adopt(T *o)
  {
    o->names = &mNames;
    o->tableIdx = mItems.size();
    mByName.emplace(int(o->name), o);
    mItems.push_back(o);
  }

public:
  explicit RenderPassNamedTable(VarNameMap &names) : mNames{names} {}
  ~RenderPassNamedTable()
  {
    for (T *o : mItems)
      delete o;
  }
  RenderPassNamedTable(const RenderPassNamedTable &) = delete;
  RenderPassNamedTable &operator=(const RenderPassNamedTable &) = delete;

  T *findByName(int name_id) const
  {
    auto it = mByName.find(name_id);
    return it != mByName.end() ? it->second : nullptr;
  }
  T *findByName(const char *name) const { return findByName(mNames.getVarId(name)); }
  T *byId(int id) const { return id >= 0 && id < mItems.size() ? mItems[id] : nullptr; }
  const Tab<T *> &all() const { return mItems; }
  void bindNames(dag::ConstSpan<T *> loaded) const
  {
    for (T *o : loaded)
      o->names = &mNames;
  }
  Tab<T *> release()
  {
    Tab<T *> out = eastl::move(mItems);
    mItems.clear();
    mByName.clear();
    return out;
  }
};

class FramebufferLayoutTable : public RenderPassNamedTable<FramebufferLayoutObject>
{
public:
  using RenderPassNamedTable::RenderPassNamedTable;

  void registerLayout(FramebufferLayoutObject *l) { adopt(l); }

  void link(Tab<FramebufferLayoutObject *> &loaded_layouts);
};

enum class FramebufferFormatClass
{
  Float,
  Uint,
  Sint,
};

struct FramebufferFormatInfo
{
  const char *token;
  uint32_t texfmt;
  FramebufferFormatClass cls;
  int channels;
  bool hasAlpha;
  bool isDepth;
};

bool framebuffer_format_by_token(const char *token, uint32_t &out_texfmt);
const FramebufferFormatInfo *framebuffer_format_info(uint32_t texfmt);
const char *framebuffer_format_class_name(FramebufferFormatClass c);
bool framebuffer_hlsl_type_class(const char *type, FramebufferFormatClass &out_cls, int &out_components);
bool framebuffer_samples_by_token(const char *token, int &out_samples);
bool framebuffer_resolve_format_tokens(const char *owner_kind, const char *owner_name, const char *target_name,
  const BaseParNamespace::Terminal *fmt_tok, const BaseParNamespace::Terminal *samples_tok, Parser &parser, uint32_t &out_format,
  int &out_samples);
eastl::string framebuffer_format_token_list();

void framebuffer_layout_flatten(const FramebufferLayoutObject &l, Tab<int> &out);
void framebuffer_sort_names_by_text(Tab<int> &name_ids, const VarNameMap &names);

struct FramebufferLayoutRef
{
  const RenderPassObject *pass = nullptr;
  const FramebufferLayoutObject *layout = nullptr; // null: the pass's inline layout

  const char *name() const;
  bool targetFormat(int target_name_id, uint32_t &out_format, int &out_samples) const;
  const FramebufferFormatInfo *targetFormatInfo(int target_name_id) const;
};

void framebuffer_layouts_of_pass(const RenderPassObject &rp, const FramebufferLayoutTable &layouts, Tab<FramebufferLayoutRef> &out);
void framebuffer_layouts_of_group(const RenderPassGroup &grp, const RenderPassTable &passes, const FramebufferLayoutTable &layouts,
  Tab<FramebufferLayoutRef> &out);

namespace ShaderParser
{
void add_framebuffer_layout(ShaderTerminal::framebuffer_layout_decl *fl, Parser &parser, shc::TargetContext &ctx);
}
