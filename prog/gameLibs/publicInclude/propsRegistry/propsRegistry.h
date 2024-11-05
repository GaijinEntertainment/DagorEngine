//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <EASTL/string.h>

class DataBlock;

namespace propsreg
{
class PropsRegistry
{
public:
  virtual ~PropsRegistry() {}
  virtual void registerProps(int props_id, const DataBlock *blk) = 0;
  virtual void clearProps() = 0;
};

int register_props(const char *name, const DataBlock *blk, int prop_class_id);
int register_props(const char *name, const DataBlock *blk, const char *class_name);
int register_props(const char *filename, int prop_class_id);
int register_props(const char *filename, const char *class_name);
void register_props_list(const DataBlock *blk, const char *class_name);
int register_net_props(const char *blk_name, const char *class_name);
int get_props_id(const char *name, const char *class_name);
bool is_props_valid(int props_id, int prop_class_id);
bool is_props_valid(int props_id, const char *class_name);
void clear_props(const char *class_name);
void unreg_props(const char *class_name);
void clear_registry();
void cleanup_props();

const char *get_props_registered_name(int props_id, const char *class_name);

void register_props_class_internal(PropsRegistry &new_registry, const char *prop_class, size_t length);
template <size_t N>
inline void register_props_class(PropsRegistry &new_registry, const char (&prop_class)[N])
{
  G_STATIC_ASSERT(N > 1); // not empty string
  return register_props_class_internal(new_registry, &prop_class[0], N - 1);
}
int get_prop_class_dyn(const eastl::string_view &prop_class);
template <size_t N>
inline int get_prop_class(const char (&prop_class)[N])
{
  G_STATIC_ASSERT(N > 1); // not empty string
  return get_prop_class_dyn(eastl::string_view(&prop_class[0], N - 1));
}
}; // namespace propsreg
