#include <api/das/nodeEcsRegistration.h>

bool NodeEcsRegistrationAnnotation::apply(const das::FunctionPtr &func, das::ModuleGroup &libGroup,
  const das::AnnotationArgumentList &args, das::string &err)
{
  logerr("[bfg_ecs_node] requires the daBfg lib compiled with the DABFG_ENABLE_DAECS_INTEGRATION flag!");
  return false;
}

void NodeEcsRegistrationAnnotation::complete(das::Context *context, const das::FunctionPtr &funcPtr) {}
