// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/riLandClass.h>

void (*get_rendinst_land_class_data)(TempRiLandclassVec &ri_landclasses) = nullptr;
bool (*get_land_class_detail_data)(DataBlock &dest_data, const char *land_cls_res_nm) = nullptr;
