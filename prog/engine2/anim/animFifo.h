#ifndef __DAGOR_ANIM_V20_FIFO_H
#define __DAGOR_ANIM_V20_FIFO_H
#pragma once

static constexpr int USE_DEBUG = 0;

#include <anim/dag_animBlend.h>
#include <anim/dag_animBlendCtrl.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <util/dag_simpleString.h>


namespace AnimV20
{
struct AnimFifo3Queue
{
public:
  enum
  {
    ST_0,   // no items
    ST_1,   // only 1 item - stable animation
    ST_1_2, // has >=2 items, and current animation is blended with next
  };
  IAnimBlendNode *node[3];
  real morphTime[2];
  int state;
  real t0;

  AnimFifo3Queue() : state(ST_0), t0(0)
  {
    node[0] = node[1] = node[2] = NULL;
    morphTime[0] = morphTime[1] = 0.f;
  }

  void save(IGenSave &cb, AnimationGraph & /*graph*/)
  {
    cb.writeInt(state);
    cb.writeReal(t0);
    cb.writeReal(morphTime[0]);
    cb.writeReal(morphTime[1]);
    for (int i = 0; i < countof(node); i++)
      cb.writeInt(node[i] ? node[i]->getAnimNodeId() : -1);
  }


  void load(IGenLoad &cb, AnimationGraph &graph)
  {
    cb.readInt(state);
    cb.readReal(t0);
    cb.readReal(morphTime[0]);
    cb.readReal(morphTime[1]);

    for (int i = 0; i < 3; i++)
    {
      int nodeId;
      cb.readInt(nodeId);
      if (nodeId != -1)
      {
        node[i] = graph.getBlendNodePtr(nodeId);
        if (!node[i])
          DAGOR_THROW(IGenLoad::LoadException("FIFO node not in AnimGraph", cb.tell()));
      }
      else
        node[i] = NULL;
    }
  }

  void reset(bool leave_cur_state)
  {
    if (leave_cur_state && node[0])
    {
      node[2] = NULL;
      if (state != ST_1_2)
      {
        node[1] = NULL;
        state = ST_1;
      }
    }
    else
    {
      node[0] = NULL;
      node[1] = NULL;
      node[2] = NULL;
      state = ST_0;
      t0 = 0;
    }
  }

  void enqueueItem(real ctime, IPureAnimStateHolder &st, IAnimBlendNode *n, real overlap_time, real max_lag)
  {
    if (overlap_time < 0.0)
      overlap_time = 0.0;

    (void)(max_lag);
    if (USE_DEBUG)
      debug_ctx("%p.enqueue: state=%d, ctime=%.3f t0=%.3f n=%p mt=%.3f max_lag=%.3f", this, state, ctime, t0, n, overlap_time,
        max_lag);

    // debug_ctx ( "enqueue: state=%d, ctime=%.3f t0=%.3f n=%p mt=%.3f max_lag=%.3f\n  node=%p %p %p\n  morph=   %.3f %.3f", state,
    // ctime, t0, n, overlap_time, max_lag, node[0], node[1], node[2], morphTime[0], morphTime[1] );
    switch (state)
    {
      case ST_0:
        node[0] = n;
        state = ST_1;
        break;

      case ST_1:
        if (node[0] == n)
          break;

        if (overlap_time > 0.0)
        {
          node[1] = n;
          morphTime[0] = overlap_time;
          t0 = ctime;
          state = ST_1_2;
        }
        else
        {
          node[0] = n;
          state = ST_1; //-V1048 variable was assigned the same value.
        }

        n->resume(st, false);
        break;

      case ST_1_2:
        if (node[1] == n)
        {
          // already blending to it - just clear node2
          node[2] = NULL;
          break;
        }

        if (node[0] == n)
        {
          // reverse blending
          if (overlap_time > 0.0)
          {
            real a1 = (ctime - t0) / morphTime[0], a0 = 1 - a1;

            // swap node0 and node1 and setup new morph time
            node[0] = node[1];
            node[1] = n;
            morphTime[0] = overlap_time;
            t0 = ctime - a0 * overlap_time;
          }
          else
          {
            node[0] = n; //-V1048 variable was assigned the same value.
            node[1] = NULL;
            state = ST_1;
          }

          // clear node2
          node[2] = NULL;
          break;
        }

        // enqueue node2 and its morph time
        node[2] = n;
        morphTime[1] = overlap_time;
        node[2]->pause(st);
        node[0]->resume(st, false);
        node[1]->resume(st, false);
        break;
    }
    // debug  ( "  state=%d, t0=%.3f  node=%p %p %p  morph=   %.3f %.3f", state, t0, node[0], node[1], node[2], morphTime[0],
    // morphTime[1] );
  }
  bool isEnqueued(IAnimBlendNode *n)
  {
    if (state == ST_1)
      return node[0] == n;
    if (state == ST_1_2)
      return node[1] == n || node[0] == n;
    return false;
  }

  void update(IPureAnimStateHolder &st, real ctime, real /*dt*/)
  {
    if (USE_DEBUG)
      debug_ctx("%p.update: state=%d, ctime=%.3f t0=%.3f", this, state, ctime, t0);

    if (state <= ST_1)
      return;

    // ST_1_2 case
    if (ctime - t0 >= morphTime[0])
    {
      // blend done, advance queue
      node[0] = node[1];
      if (node[2] && morphTime[1] > 0)
      {
        node[1] = node[2];
        morphTime[0] = morphTime[1];
        node[2] = NULL;
        t0 = ctime;
        state = ST_1_2;
        node[1]->resume(st, false);
      }
      else if (node[2])
      {
        node[0] = node[2];
        node[1] = node[2] = NULL;
        state = ST_1;
        node[0]->resume(st, false);
      }
      else
      {
        node[1] = NULL;
        state = ST_1;
      }
    }
  }

  void buildBlendingList(IAnimBlendNode::BlendCtx &bctx, real ctime, real w0)
  {
    if (USE_DEBUG)
      debug_ctx("%p.bbl: state=%d, ctime=%.3f t0=%.3f", this, state, ctime, t0);

    if (state == ST_0)
      return;

    if (state == ST_1)
    {
      node[0]->buildBlendingList(bctx, w0);
      return;
    }

    if (morphTime[0] < 1e-6)
    {
      if (node[1])
        node[1]->buildBlendingList(bctx, w0);
      return;
    }

    // ST_1_2 case
    real a1 = (ctime - t0) / morphTime[0];
    node[0]->buildBlendingList(bctx, w0 * (1 - a1));
    if (state > ST_1 && node[1])
      node[1]->buildBlendingList(bctx, w0 * a1);
  }
  float getEnqueuedWeight(IAnimBlendNode *n, real ctime)
  {
    if (state == ST_0)
      return 0;

    if (state == ST_1)
      return node[0] == n ? 1 : 0;

    if (morphTime[0] < 1e-6)
      return node[1] == n ? 1 : 0;

    // ST_1_2 case
    real a1 = (ctime - t0) / morphTime[0];
    if (node[0] == n)
      return (1 - a1);
    if (state > ST_1 && node[1] == n)
      return a1;
    return 0;
  }
};
} // end of namespace AnimV20

#endif
