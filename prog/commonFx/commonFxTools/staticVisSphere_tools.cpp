// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>


ScriptHelpers::TunedElement *StaticVisSphereParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("center", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 10));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}
