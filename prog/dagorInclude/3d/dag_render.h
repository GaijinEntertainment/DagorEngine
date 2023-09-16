//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

#include <supp/dag_define_COREIMP.h>

struct DagorCurView
{
  TMatrix tm, itm;
  Point3 pos; // current scene view position
};

struct DagorCurFog
{
  E3DCOLOR color;
  real dens;
};

// global render states
extern bool grs_draw_wire, grs_paused, grs_slow;

extern DagorCurView grs_cur_view;
extern DagorCurFog grs_cur_fog;


// compute and set gamma correction
void set_gamma(real p);
void set_gamma_shadervar(real p);

// returns current gamma
real get_current_gamma();

// script interface - to be refactored
class DagorScriptRenderInterface
{
public:
  virtual ~DagorScriptRenderInterface() {}
  DagorScriptRenderInterface() {}

public:
  virtual Point3 get_curviewpos();
  virtual void get_curviewtm(TMatrix &tm);
  virtual void get_curviewitm(TMatrix &tm);
  virtual void get_m2vtm(TMatrix &tm);
  virtual void get_im2vtm(TMatrix &tm);
  virtual int get_curclippl();
  virtual void set_curclippl(int);
  virtual int get_curfogmode();
  virtual void set_curfogmode(int);
  virtual int get_curfogcolor();
  virtual void set_curfogcolor(int);
  virtual float get_curparticlesquality();
  virtual void set_curparticlesquality(float);
};

KRNLIMP DagorScriptRenderInterface *get_DagorScriptRenderInterface();

#include <supp/dag_undef_COREIMP.h>
