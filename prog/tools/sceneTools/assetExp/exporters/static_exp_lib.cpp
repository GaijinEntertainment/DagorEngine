#include "dabuild_exp_plugin_chain.h"
#include <ioSys/dag_dataBlock.h>

#define LIST    \
  DO(a2d)       \
  DO(anim)      \
  DO(modelExp)  \
  DO(collision) \
  DO(fastPhys)  \
  DO(physObj)   \
  DO(apex)      \
  DO(fx)        \
  DO(land)      \
  DO(locShader) \
  DO(mat)       \
  DO(tex)       \
  DO(spline)    \
  DO(composit)

#define DO(X) extern int pull_##X;
LIST;
#undef DO

#define DO(X) +pull_##X
int pull_all_dabuild_plugins = LIST;
#undef DO

#if !HAVE_APEX_DESTR
int pull_apex = 0;
#endif
