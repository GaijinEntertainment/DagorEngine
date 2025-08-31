// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "nameMap.h"
#include <EASTL/bitvector.h>


struct ShaderMessages
{
  SCFastNameMap strings;
  eastl::bitvector<> nonFilenameMessages;

  int addMessage(const char *message, bool file_name)
  {
    int id = strings.addNameId(message);
    if (!file_name)
      nonFilenameMessages.set(id, true);
    return id;
  }

  bool isFilenameMessage(int id) const { return !nonFilenameMessages.test(id, false); }
};
