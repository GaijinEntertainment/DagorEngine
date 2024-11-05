// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <asyncHTTPClient/asyncHTTPClient.h>

namespace httprequests
{

void init_async(InitAsyncParams const &) {}

void shutdown_async() {}

RequestId async_request(AsyncRequestParams const &) { return 0; }

void abort_request(RequestId) {}

void abort_all_requests(AbortMode = AbortMode::Abort) {}

void poll() {}

} // namespace httprequests
