// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifdef IMMEDIATE_CB_NAMESPACE
IMMEDIATE_CB_NAMESPACE
{
#endif

#ifndef IMMEDAITE_CB_REGISTER_NO
#define IMMEDAITE_CB_REGISTER_NO 8
#endif

  static constexpr int IMMEDAITE_CB_REGISTER = IMMEDAITE_CB_REGISTER_NO;
  bool init_immediate_cb();
  void term_immediate_cb();
#ifdef IMMEDIATE_CB_NAMESPACE
}
#endif
