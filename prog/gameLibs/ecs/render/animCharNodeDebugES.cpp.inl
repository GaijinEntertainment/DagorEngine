#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <math/dag_geomTree.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

ECS_NO_ORDER
ECS_TAG(render, dev)
static inline void debug_draw_animchar_node_es(const ecs::UpdateStageInfoRenderDebug &, const AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Array &animchar__debugNodes, bool animchar__debugNodesPos = false)
{
  for (auto &debugNode : animchar__debugNodes)
  {
    auto idx = animchar.getNodeTree().findNodeIndex(debugNode.getStr());
    G_ASSERTF_CONTINUE(idx, "cannot find node %s", debugNode.getStr());
    Point3 pos = animchar.getNodeTree().getNodeWposScalar(idx);
    ANIMCHAR_VERIFY_NODE_POS_S(pos, idx, animchar);
    draw_debug_box_buffered(BBox3(pos, 0.01f));
    if (animchar__debugNodesPos)
    {
      String str(32, "%@", pos);
      add_debug_text_mark(pos, str.str());
      debug(str.str());
    }
  }
}
