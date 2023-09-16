//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>


namespace wingw
{
long copyStringToClipboard(void *handle, const SimpleString &str_buffer);
long pasteStringFromClipboard(void *handle, SimpleString &str_buffer);
} // namespace wingw
