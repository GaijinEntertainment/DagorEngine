// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderSave.h"
#include "deSerializationContext.h"

const char *VarMapAdapter::getName(int id) { return get_shaders_de_serialization_ctx()->varNameMap().getName(id); }
int VarMapAdapter::addName(const char *name) { return get_shaders_de_serialization_ctx()->varNameMap().addVarId(name); }