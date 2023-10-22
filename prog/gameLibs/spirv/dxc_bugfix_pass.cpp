#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

// dxc has some minor bugs right now that can be fixed with some pre passes
void spirv::fixDXCBugs(ModuleBuilder &builder, ErrorHandler &e_handler)
{
  // put in here stuff to fix some problems with DXC, right now everything has been fixed
}