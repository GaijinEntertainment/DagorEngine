#pragma once

class DataBlock;

#include <generic/dag_tab.h>

#include <libTools/dagFileRW/geomMeshHelper.h>

void read_data_from_blk(Tab<GeomMeshHelperDagObject> &dagObjectsList, const DataBlock &blk);