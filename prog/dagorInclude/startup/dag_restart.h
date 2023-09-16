//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>


#define RESTART_VIDEO             (1 << 0)
#define RESTART_AUDIO             (1 << 1)
#define RESTART_INPUT             (1 << 2)
#define RESTART_ALE               (1 << 3)
#define RESTART_UI                (1 << 4)
#define RESTART_GAME              (1 << 5)
#define RESTART_SCRIPTSYS         (1 << 6)
#define RESTART_NETWORK           (1 << 7)
#define RESTART_VIDEODRV          (1 << 8)
#define RESTART_DRIVER_VIDEO_MODE (1 << 9)
#define RESTART_OTHER             0x40000000
#define RESTART_ALL               0x3FFFFFFF
#define RESTART_GO                0x80000000

#define RESTART_ERASE_RPROC 0x40000000

class RestartProc
{
public:
  int priority;

  KRNLIMP RestartProc(int pr = 100);
  KRNLIMP virtual ~RestartProc();
  virtual void destroy() = 0; // delete this

  virtual void startupf(int flags) = 0;
  virtual void shutdownf(int flags) = 0;
  virtual const char *procname() = 0;
};

class SRestartProc : public RestartProc
{
public:
  int flags;
  int inited;

  KRNLIMP SRestartProc(int flg, int pr = 100);
  KRNLIMP virtual void destroy(); // shutdown (don't delete)
  KRNLIMP virtual void startupf(int flags);
  KRNLIMP virtual void shutdownf(int flags);

  virtual void startup() = 0;
  virtual void shutdown() = 0;
};

KRNLIMP void add_restart_proc(RestartProc *);
KRNLIMP int remove_restart_proc(RestartProc *); // do not destroy
KRNLIMP int del_restart_proc(RestartProc *);    // destroy
KRNLIMP int find_restart_proc(RestartProc *);   // returns non-zero if found

KRNLIMP void set_restart_flags(int flags);
KRNLIMP int is_restart_required(int flags);
KRNLIMP void startup_game(int flags, void (*on_before_proc)(const char *) = NULL, void (*on_after_proc)(const char *) = NULL);
KRNLIMP void shutdown_game(int flags);

#include <supp/dag_undef_COREIMP.h>
