//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace jetseat
{
enum
{
  EJS_OK = 0,
  EJS_NO_DEVICE,
  EJS_NOT_CONNECTED,
  EJS_ALREADY_IN_USE
};

void init();
void shutdown();

bool is_enabled();
bool is_installed();
int get_status();

void set_vibro_strength(float param);
void set_vibro(float leftSeat, float rightSeat, float leftWaist, float rightWaist, float leftBack, float rightBack);

void update(float dt);
}; // namespace jetseat
