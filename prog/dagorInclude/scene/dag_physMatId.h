//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <scene/dag_physMatIdDecl.h>

namespace PhysMat
{
//************************************************************************
// return total count of phys materials
int physMatCount();

// get material id. return PHYSMAT_INVALID, if failed
MatID getMaterialId(const char *name);

// get interact id. return PHYSMAT_INVALID, if failed
InteractID getInteractId(MatID pmid1, MatID pmid2);
}; // namespace PhysMat

inline bool IsPhysMatID_Valid(PhysMat::MatID id) { return (uint32_t)id < (uint32_t)PhysMat::physMatCount(); }
void AssertPhysMatID_Valid(const char *file, int ln, PhysMat::MatID id);
#define ASSERT_PHYS_MAT_ID_VALID(id) AssertPhysMatID_Valid(__FILE__, __LINE__, id)
