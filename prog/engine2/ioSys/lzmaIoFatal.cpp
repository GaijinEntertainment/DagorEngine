#include <util/dag_globDef.h>
#include <ioSys/dag_lzmaIo.h>

void LzmaLoadCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
