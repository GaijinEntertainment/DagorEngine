//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/detail/resourceType.h>
#include <daScript/ast/ast_typedecl.h>

namespace dabfg
{
namespace detail
{
extern void register_external_interop_type(const char *mangled_name, dabfg::ResourceSubtypeTag tag);
}

namespace das
{
template <class T>
void register_external_interop_type(const ::das::ModuleLibrary &library)
{
  detail::register_external_interop_type(::das::makeType<T>(library)->getMangledName().c_str(), tag_for<T>());
}
} // namespace das

} // namespace dabfg
