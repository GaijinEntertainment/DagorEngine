// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void set_timespeed(float ts);
float get_timespeed();
void toggle_pause();

void load_res_package(const char *folder, bool optional = true);

// here we init and term stuff needed by game (rendinsts for instance)
void init_game();
void term_game();
