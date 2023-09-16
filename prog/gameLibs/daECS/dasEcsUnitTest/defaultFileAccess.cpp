#include <dasModules/dasFsFileAccess.h>
#include <daScript/misc/smart_ptr.h>
#include <daScript/misc/platform.h>

namespace bind_dascript
{
extern HotReload globally_hot_reload;
}

das::smart_ptr<das::FileAccess> get_file_access(char *pak)
{
  if (pak)
    return das::make_smart<bind_dascript::DagFileAccess>(pak, bind_dascript::globally_hot_reload);
  return das::make_smart<bind_dascript::DagFileAccess>(bind_dascript::globally_hot_reload);
}
