// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/phys/collRes.h>
#include <ecs/anim/anim.h>

template <typename T>
inline bool get_animchar_collision_transform_ecs_query(ecs::EntityId eid, T cb);

bool get_collres_body_tm(const ecs::EntityId eid, const int coll_node_id, TMatrix &tm, const char *fnname)
{
  bool ret = false;
  get_animchar_collision_transform_ecs_query(eid,
    [&](const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar, const TMatrix &transform = TMatrix::IDENT) {
      const CollisionNode *cnode = collres.getNode(coll_node_id);
      G_FAST_ASSERT(cnode);
      const GeomNodeTree &tree = animchar.getNodeTree();
      collres.getCollisionNodeTm(collres.getNode(coll_node_id), transform, &tree, tm);
      if (DAGOR_LIKELY(v_test_vec_x_lt(v_length3_sq_x(v_ldu(&tm.getcol(3).x)), V_C_MAX_VAL)))
      {
        ret = true;
        return;
      }
      static bool loggedOnce = false;
      if (loggedOnce)
        return;
      logerr("%s: %d<%s> collnode=%d(%s) nodeTree[%d(%s)][3]=%@ rtm[3]=%@ etm[3]=%@", fnname, (ecs::entity_id_t)eid,
        g_entity_mgr->getEntityTemplateName(eid), coll_node_id, cnode ? cnode->name.c_str() : nullptr,
        cnode ? (int)cnode->geomNodeId : -2, cnode ? tree.getNodeName(cnode->geomNodeId) : nullptr, tm.getcol(3),
        tree.getRootWposScalar(), transform.getcol(3));
      debug_flush(false);
      loggedOnce = true;
    });
  return ret;
}