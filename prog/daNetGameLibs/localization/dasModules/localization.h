// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <util/dag_localization.h>

namespace bind_dascript
{
inline const char *get_localized_text(const char *str) { return ::get_localized_text(str); }

inline const char *get_localized_text_silent(const char *str) { return ::get_localized_text_silent(str); }
} // namespace bind_dascript
