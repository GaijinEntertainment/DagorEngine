// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/asyncHostnameResolver.h>


namespace sepclient::details
{

AsyncHostnameResolverPtr create_async_hostname_resolver(eastl::string_view /* hostname */, ResolveOptions /* options */)
{
  return nullptr;
}

} // namespace sepclient::details
