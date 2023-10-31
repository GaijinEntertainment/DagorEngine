/*
 * Dagor Engine 3 - Game Libraries
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of use see prog/license.txt)
 */

#ifndef DAGOR_GAMELIBS_BREAKPAD_SENDER_SENDER_H_
#define DAGOR_GAMELIBS_BREAKPAD_SENDER_SENDER_H_
#pragma once

namespace breakpad
{

int process_report(int argc, char **argv);

} // namespace breakpad

#endif // DAGOR_GAMELIBS_BREAKPAD_SENDER_SENDER_H_
