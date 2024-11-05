// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;


void ec_init_stat3d();

void ec_stat3d_save_enabled_graphs(DataBlock &blk);
void ec_stat3d_load_enabled_graphs(int vp_idx, const DataBlock *blk);
void ec_stat3d_wait_frame_end(bool frame_start);
void ec_stat3d_on_unit_begin();
void ec_stat3d_on_unit_end();
