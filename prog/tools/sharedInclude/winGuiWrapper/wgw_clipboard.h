//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_simpleString.h>


namespace wingw
{
long copyStringToClipboard(void *handle, const SimpleString &str_buffer);
long pasteStringFromClipboard(void *handle, SimpleString &str_buffer);
} // namespace wingw
