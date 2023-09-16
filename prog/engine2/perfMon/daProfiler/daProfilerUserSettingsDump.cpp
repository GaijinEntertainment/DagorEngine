#include <ioSys/dag_memIo.h>
#include "dumpLayer.h"
#include "daProfilerUserSettings.h"

namespace da_profiler
{

void response_settings(IGenSave &cb, const UserSettings &s)
{
  DynamicMemGeneralSaveCB stream(tmpmem_ptr(), 256, 256);
  write_settings(stream, s);
  send_data(cb, DataResponse::SettingsPack, stream);
}

} // namespace da_profiler
