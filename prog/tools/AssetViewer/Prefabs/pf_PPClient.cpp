#include "prefabs.h"
#include "pf_cm.h"

// #include <libTools/staticGeom/geomObject.h>
// #include <libTools/staticGeom/staticGeometryContainer.h>

#include <util/dag_bitArray.h>
#include <propPanel2/c_panel_base.h>


#define PP_COMPARE_PARAM(panel, pid, var, get, def) \
  case pid:                                         \
    var = panel.##get(pid, def);                    \
                                                    \
    if (var != def)                                 \
      def = var;                                    \
    break;


#define PP_COMPARE_PARAMS_IN_P2(panel, pid, var, p1, p2) \
  case pid:                                              \
    var = panel.getPoint2(pid, Point2(p1, p2));          \
                                                         \
    if (var.x != p1)                                     \
      p1 = var.x;                                        \
    else if (var.y != p2)                                \
      p2 = var.y;                                        \
    break;


// IPropPanelClient
//==============================================================================
void PrefabsPlugin::onChange(int pid, PropertyContainerControlBase *panel) { nodeModif->onPPChange(*panel, pid); }


//==============================================================================
void PrefabsPlugin::onClick(int pid, PropertyContainerControlBase *panel) { nodeModif->onPPBtnPressed(*panel, pid); }