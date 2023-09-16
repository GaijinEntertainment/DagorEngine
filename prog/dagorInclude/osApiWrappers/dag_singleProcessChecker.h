//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

bool init_single_process_checker(const char *mutex_name); // false - Another instance of game is already started
void close_single_process_checker();
bool is_process_single();
