/*
 * Dagor Engine 3 - Game Libraries
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef DAGOR_GAMELIBS_BREAKPAD_SENDER_STATS_H_
#define DAGOR_GAMELIBS_BREAKPAD_SENDER_STATS_H_
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

#endif // DAGOR_GAMELIBS_BREAKPAD_SENDER_STATS_H_
