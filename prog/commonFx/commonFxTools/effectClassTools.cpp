#include <generic/dag_tab.h>
#include <fx/effectClassTools.h>

#include <debug/dag_debug.h>


static Tab<IEffectClassTools *> list(inimem_ptr());


void register_effect_class_tools(IEffectClassTools *iface)
{
  if (!iface)
    return;

  for (int i = 0; i < list.size(); ++i)
  {
    if (list[i] == iface)
      return;
  }

  IEffectClassTools *f = get_effect_class_tools_interface(iface->getClassName());
  if (f)
  {
    debug_ctx("duplicate effect class tools: '%s', %p and %p", iface->getClassName(), iface, f);
    return;
  }

  list.push_back(iface);
}


void unregister_effect_class_tools(IEffectClassTools *iface)
{
  for (int i = 0; i < list.size(); ++i)
    if (list[i] == iface)
    {
      erase_items(list, i, 1);
      return;
    }
}


IEffectClassTools *get_effect_class_tools_interface(const char *class_name)
{
  for (int i = 0; i < list.size(); ++i)
    if (strcmp(list[i]->getClassName(), class_name) == 0)
      return list[i];

  return NULL;
}
