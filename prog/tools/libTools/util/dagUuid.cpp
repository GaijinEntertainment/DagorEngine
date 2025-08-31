// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/dagUuid.h>
#include <debug/dag_assert.h>

#if _TARGET_PC_WIN
#include <rpc.h>

#pragma comment(lib, "rpcrt4.lib")

void DagUUID::generate(DagUUID &out) { ::UuidCreate((UUID *)(void *)&out); }
#elif _TARGET_PC_LINUX
#include <uuid/uuid.h>

void DagUUID::generate(DagUUID &out)
{
  G_STATIC_ASSERT(sizeof(uuid_t) == sizeof(out));
  uuid_generate(reinterpret_cast<unsigned char *>(&out));
}
#endif
