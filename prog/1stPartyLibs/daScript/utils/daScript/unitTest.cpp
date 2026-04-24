#include <daScript/daScript.h>
#include "../../modules/dasUnitTest/unitTest.h"

namespace das
{
void install_das_crash_handler() {}
} // namespace das

void require_project_specific_modules() { NEED_MODULE(Module_UnitTest); }