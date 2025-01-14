// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <ioSys/dag_zlibIo.h>

void ZlibLoadCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
void ZlibSaveCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
void ZlibDecompressSaveCB::issueFatal() { G_ASSERT(0 && "restricted by design"); }
