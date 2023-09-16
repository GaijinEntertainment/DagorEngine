#include <libTools/util/dagUuid.h>
#include <rpc.h>

#pragma comment(lib, "rpcrt4.lib")

void DagUUID::generate(DagUUID &out) { ::UuidCreate((UUID *)(void *)&out); }
