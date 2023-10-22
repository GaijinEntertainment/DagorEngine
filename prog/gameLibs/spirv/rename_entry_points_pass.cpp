#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

// very simple, just loop over all entires and change the matching names or to the default one if
// provided
void spirv::renameEntryPoints(ModuleBuilder &builder, const EntryPointRename *list /* = nullptr*/, unsigned list_len /* = 0*/,
  const char *default_name /* = "main"*/)
{
  builder.enumerateEntryPoints([=](auto function, auto exec_model, auto &name, auto &interface) //
    {
      auto ref = eastl::find_if(list, list + list_len,
        [&](auto &element) //
        { return element.from == name; });
      if (ref != list + list_len)
        name = ref->to;
      else if (default_name)
        name = default_name;
    });
}