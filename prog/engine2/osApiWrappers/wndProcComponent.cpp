#include <osApiWrappers/dag_wndProcComponent.h>
#include <stdlib.h>
#include <string.h>

static constexpr int MAX_COMP_NUM = 32;
static IWndProcComponent *comp[MAX_COMP_NUM] = {0};

static int find_component(IWndProcComponent *c)
{
  for (int i = 0; i < MAX_COMP_NUM; i++)
    if (comp[i] == c)
      return i;
  return -1;
}

void add_wnd_proc_component(IWndProcComponent *c)
{
  if (find_component(c) != -1)
    return;

  for (int i = 0; i < MAX_COMP_NUM; i++)
    if (!comp[i])
    {
      comp[i] = c;
      return;
    }
}
void del_wnd_proc_component(IWndProcComponent *c)
{
  int id = find_component(c);
  if (id != -1)
    comp[id] = NULL;
}

static bool is_inside_wnd_proc_components = false;

bool perform_wnd_proc_components(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
{
  if (is_inside_wnd_proc_components)
    return false;

  is_inside_wnd_proc_components = true;
  struct OnReturn
  {
    ~OnReturn() { is_inside_wnd_proc_components = false; }
  } onReturn;

  IWndProcComponent::RetCode ret;

  for (int i = 0; i < MAX_COMP_NUM; i++)
    if (comp[i])
    {
      ret = comp[i]->process(hwnd, msg, wParam, lParam, result);
      if (ret == IWndProcComponent::IMMEDIATE_RETURN)
        return true;
      if (ret == IWndProcComponent::PROCEED_DEF_WND_PROC)
        return false;
    }

  return false;
}

void *detach_all_wnd_proc_components()
{
  void *ret = malloc(sizeof(comp));
  memcpy(ret, comp, sizeof(comp));
  memset(comp, 0, sizeof(comp));
  return ret;
}

void attach_all_wnd_proc_components(void *h)
{
  if (!h)
    return;
  memcpy(comp, h, sizeof(comp));
  free(h);
}

#define EXPORT_PULL dll_pull_osapiwrappers_wndProcComponent
#include <supp/exportPull.h>
