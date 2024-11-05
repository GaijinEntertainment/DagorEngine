// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>


namespace event_log
{
namespace http
{
void post(const char *url, const void *data, uint32_t size, const char *user_agent, uint32_t timeout);
void post_async(const char *url, const void *data, uint32_t size, const char *user_agent, uint32_t timeout);
} // namespace http
} // namespace event_log