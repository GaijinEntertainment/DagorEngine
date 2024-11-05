// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <ioSys/dag_dataBlock.h>

#define LIST    \
  DO(a2d)       \
  DO(anim)      \
  DO(modelExp)  \
  DO(collision) \
  DO(fastPhys)  \
  DO(physObj)   \
  DO(fx)        \
  DO(land)      \
  DO(locShader) \
  DO(mat)       \
  DO(tex)       \
  DO(spline)    \
  DO(composit)

#define LIST_OPT   \
  DO(vehicle)      \
  DO(shipSections) \
  DO(apex)

#define DO(X) extern int pull_##X;
LIST;
LIST_OPT;
#undef DO

#define DO(X) +pull_##X
int pull_all_dabuild_plugins = LIST LIST_OPT;
#undef DO
