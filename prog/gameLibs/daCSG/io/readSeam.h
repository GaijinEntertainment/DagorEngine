// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/mesh_api.h>
#include <osApiWrappers/dag_files.h>
#include <EASTL/string.h>

namespace dacsg
{
using ObjReader = int (*)(file_ptr_t fp, void *buf, int len);

// read_obj with every block read through reader, not null, which read_obj passes as df_read and a test as a reader that
// cuts the blocks or ends early: the file still opens, and a reader's end before df_length fails the read
bool read_obj_through(const char *path, Mesh &m, eastl::string &err, ObjReader reader);
} // namespace dacsg
