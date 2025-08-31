//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/resourceType.h>
#include <daScript/ast/ast.h>
#include <render/daFrameGraph/detail/rtti.h>

namespace dafg
{
namespace detail
{
extern void register_das_interop_type(const char *mangled_name, dafg::ResourceSubtypeTag tag, RTTI &&rtti);
}

namespace das
{
template <class T>
void register_interop_type(const ::das::ModuleLibrary &library)
{
  detail::register_das_interop_type(::das::makeType<T>(library)->getMangledName().c_str(), tag_for<T>(), detail::make_rtti<T>());
}
} // namespace das

} // namespace dafg
