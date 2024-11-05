// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace breakpad
{

struct Configuration;

namespace stats
{

void init(const Configuration &cfg);
void send(const char *metric, const char *status, const char *type, long value);
void shutdown();

inline void count(const char *m, const char *s, long v = 1) { send(m, s, "c", v); }

} // namespace stats
} // namespace breakpad
