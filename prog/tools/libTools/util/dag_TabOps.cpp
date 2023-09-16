#include <libTools/containers/dag_TabOps.h>

#include <generic/dag_tab.h>
#include <util/dag_string.h>


int check_and_add_stri(Tab<String> &tab, const char *str)
{
  int idx = -1;

  for (int i = 0; i < tab.size(); ++i)
  {
    if (!stricmp(tab[i], str))
    {
      idx = i;
      break;
    }
  }

  if (idx == -1)
  {
    idx = tab.size();
    tab.push_back() = str;
  }

  return idx;
}
