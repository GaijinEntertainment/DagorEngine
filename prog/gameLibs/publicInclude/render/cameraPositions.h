//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class TMatrix;
void save_camera_position(const char *file_name, const char *slot_name, const TMatrix &camera_itm);

bool load_camera_position(const char *file_name, const char *slot_name, TMatrix &camTm);
