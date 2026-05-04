//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>


namespace sepclient::details
{

class AsyncHostnameResolver
{
public:
  virtual ~AsyncHostnameResolver() = default;
  virtual void poll() = 0;
  virtual bool isFinished() const = 0;
  virtual const eastl::vector<eastl::string> &getResults() const = 0;
};


using AsyncHostnameResolverPtr = eastl::unique_ptr<AsyncHostnameResolver>;


enum class ResolveOptions
{
  IPV4,
  IPV6,
  IPV4_AND_IPV6,
};

using AsyncHostnameResolverFactory = eastl::function<AsyncHostnameResolverPtr(eastl::string_view hostname, ResolveOptions options)>;

AsyncHostnameResolverPtr create_async_hostname_resolver(eastl::string_view hostname, ResolveOptions options);

} // namespace sepclient::details
