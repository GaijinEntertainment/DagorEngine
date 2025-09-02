#include <fstream>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <daScript/daScript.h>
#include <daScript/daScriptModule.h>
#include <daScript/simulate/fs_file_info.h>

void os_debug_break() {printf("unhandled exception?\n");_exit(1);}
void foo() {dd_get_fname("");} //== pull in directoryService.obj
void require_project_specific_modules() {}//

das::smart_ptr<das::FileAccess> get_file_access( char * pak )
{
  if ( pak )
    return das::make_smart<das::FsFileAccess>(pak, das::make_smart<das::FsFileAccess>());
  return das::make_smart<das::FsFileAccess>();
}

das::Context* get_clone_context( das::Context * ctx, uint32_t category )
{
    return new das::Context(*ctx, category);
}


das::Context* get_context(int stack_size)
{
    return new das::Context(stack_size);
}

namespace das
{
vector<void *> force_aot_stub() { return {}; }
} // namespace das
