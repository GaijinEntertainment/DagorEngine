//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

extern long long iso8601_parse(const char *str);
extern long long iso8601_parse(const char *str, size_t len);
extern void iso8601_format(long long timeMsec, char *buffer, size_t N);
