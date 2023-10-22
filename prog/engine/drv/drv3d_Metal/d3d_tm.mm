
#include <3d/dag_drv3d.h>
#include <3d/dag_3dConst_base.h>


#include "frameStateTM.inc.h"

namespace drv3d_metal
{
    FrameStateTM g_frameState;
}

using namespace drv3d_metal;

#define CHECK_MAIN_THREAD()

#include "frameStateTM.inc.cpp"

