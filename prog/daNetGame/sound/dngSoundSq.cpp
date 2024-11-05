// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dngSound.h"
#include <sqrat.h>
#include <bindQuirrelEx/autoBind.h>
#include <math/dag_math3d.h>


static void apply_audio_settings(Sqrat::Array changed_fields)
{
  auto wasChanged = [&changed_fields](const char *name) -> bool {
    for (SQInteger i = 0, n = changed_fields.Length(); i < n; ++i)
      if (strncmp(sq_objtostring(&changed_fields.GetSlot(i).GetObject()), name, strlen(name)) == 0)
        return true;
    return false;
  };

  if (wasChanged("sound/volume"))
    dngsound::apply_config_volumes();
}


// To consider: split client & server scripts and remove (most of) this binding from server
SQ_DEF_AUTO_BINDING_MODULE(bind_sndcontrol, "dngsound")
{
  Sqrat::Table exports(vm);

  exports.Func("apply_audio_settings", apply_audio_settings);

  return exports;
}
