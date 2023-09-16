#include <fx/commonFxTools.h>
#include <util/dag_string.h>

String fx_devres_base_path;

void register_all_common_fx_tools()
{
#ifndef _TARGET_EXPORTERS_STATIC
  register_anim_planes_fx_tools();
#endif

  register_trail_flow_fx_tools();

#ifndef _TARGET_EXPORTERS_STATIC
  register_emitterflow_ps_fx_tools();
  register_anim_planes_2_fx_tools();
#endif

  register_light_fx_tools();

  register_dafx_sparks_fx_tools();
  register_dafx_modfx_fx_tools();
  register_dafx_compound_fx_tools();

#ifndef _TARGET_EXPORTERS_STATIC
  register_compound_ps_fx_tools();
  register_flow_ps_2_fx_tools();
#endif
}
