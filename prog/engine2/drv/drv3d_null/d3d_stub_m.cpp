#include <3d/dag_drv3d.h>

D3dInterfaceTable d3di;
bool d3d::HALF_TEXEL_OFS = false;
float d3d::HALF_TEXEL_OFSFU = 0.f;

bool d3d::is_inited() { return false; }
unsigned d3d::get_driver_code() { return 0; }

static void *na_func()
{
  fatal("D3DI function not implemented");
  return nullptr;
}
bool d3d::fill_interface_table(D3dInterfaceTable &d3dit)
{
  void *(**funcTbl)() = (void *(**)()) & d3dit;
  for (int i = 0; i < sizeof(d3di) / sizeof(*funcTbl); i++)
    funcTbl[i] = &na_func;
  return false;
}
bool d3d::init_driver()
{
  fill_interface_table(d3di);
  return false;
}
