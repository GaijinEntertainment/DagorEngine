#include <daRg/dag_guiGlobals.h>
#include "guiGlobals.h"
#include <cstring>
#include <cstdlib>
#include <math/dag_bounds2.h>
#include <gui/dag_stdGuiRender.h>


namespace darg
{

DebugFramesMode debug_frames_mode = DFM_NONE;
bool require_font_size_slot = false;

void (*do_ui_blur)(const Tab<BBox2> &boxes) = nullptr;


bool set_debug_frames_mode_by_str(const char *str)
{
  DebugFramesMode mode = DFM_NONE;
  if (stricmp(str, "create") == 0)
    mode = darg::DFM_ELEM_CREATE;
  else if (stricmp(str, "update") == 0)
    mode = darg::DFM_ELEM_UPDATE;
  else
  {
    int val = strtol(str, NULL, 0);
    if (val >= DFM_FIRST && val <= DFM_LAST)
      mode = (darg::DebugFramesMode)val;
    else
      return false;
  }
  debug_frames_mode = mode;
  return true;
}


} // namespace darg
