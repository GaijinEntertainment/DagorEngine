//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

bool init_single_process_checker(const char *mutex_name); // false - Another instance of game is already started
void close_single_process_checker();
bool is_process_single();
