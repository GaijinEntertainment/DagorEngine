#include <util/dag_globDef.h>
#include <ioSys/dag_zstdIo.h>

void ZstdLoadFromMemCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
void ZstdSaveCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
