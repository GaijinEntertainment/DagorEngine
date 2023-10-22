#include <animChar/dag_animate2ndPass.h>
#include <anim/dag_animBlendCtrl.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>

bool Animate2ndPass::load(const AnimV20::AnimcharBaseComponent *animchar, const DataBlock &b, const char *a2d_name, float dur)
{
  using namespace AnimV20;
  if (const DataBlock *fb = b.getBlockByName("fadeFrom"))
  {
    fadeInDur = fb->getReal("duration", 0);
    for (int i = 0, nid = fb->getNameId("state"); i < fb->paramCount(); i++)
      if (fb->getParamType(i) == fb->TYPE_STRING && fb->getParamNameId(i) == nid)
      {
        int idx = animchar->getAnimGraph()->getStateIdx(fb->getStr(i));
        if (idx >= 0)
          fadeInStates.push_back(idx);
        else
          logerr("failed to resolve state \"%s\" in %s", fb->getStr(i), b.resolveFilename());
      }
  }
  if (const DataBlock *fb = b.getBlockByName("fadeTo"))
  {
    fadeOutDur = fb->getReal("duration", 0);
    for (int i = 0, nid = fb->getNameId("state"); i < fb->paramCount(); i++)
      if (fb->getParamType(i) == fb->TYPE_STRING && fb->getParamNameId(i) == nid)
      {
        int idx = animchar->getAnimGraph()->getStateIdx(fb->getStr(i));
        if (idx >= 0)
          fadeOutStates.push_back(idx);
        else
          logerr("failed to resolve state \"%s\" in %s", fb->getStr(i), b.resolveFilename());
      }
  }

  if (*a2d_name == '*')
  {
    a2d_name++;
    a2d = (AnimV20::AnimData *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(a2d_name), Anim2DataGameResClassId);
    if (!a2d.get())
    {
      logerr("failed to get a2d \"%s\" gameres in %s", a2d_name, b.resolveFilename());
      return false;
    }
    ::release_game_resource((GameResource *)a2d.get());
  }
  else
  {
    a2d = new AnimData;
    FullFileLoadCB crd(a2d_name);
    if (crd.fileHandle)
    {
      crd.beginBlock();
      if (!a2d->load(crd, midmem))
      {
        logerr("failed to load a2d \"%s\" in %s", a2d_name, b.resolveFilename());
        return false;
      }
      crd.close();
    }
    else
    {
      logerr("failed to open \"%s\" in %s", a2d_name, b.resolveFilename());
      return false;
    }
  }

  duration = dur;
  ag = new (midmem) AnimationGraph;
  ag->setInitState(*b.getBlockByNameEx("initAnimState"));
  ag->allocateGlobalTimer();
  DataBlock bnl_props;
  bnl_props.setStr("name", "anim");
  bnl_props.setStr("varname", "progress");
  bnl_props.setStr("key_start", "start");
  bnl_props.setStr("key_end", "end");
  bnl_props.setReal("p_end", duration);
  AnimBlendNodeParametricLeaf::createNode(*ag, bnl_props, a2d, false, true, NULL);
  ag->replaceRoot(ag->getBlendNodePtr("anim"));
  return true;
}
void Animate2ndPass::release()
{
  clear_and_shrink(fadeInStates);
  clear_and_shrink(fadeOutStates);
  fadeInDur = fadeOutDur = 0;
  duration = 1;
  ag = NULL;
  a2d = NULL;
}

void Animate2ndPass::startAnim(AnimV20::AnimcharBaseComponent *animchar)
{
  for (int i = 0; i < fadeInStates.size(); i++)
    animchar->getAnimGraph()->enqueueState(*animchar->getAnimState(), animchar->getAnimGraph()->getState(fadeInStates[i]), fadeInDur);
}
void Animate2ndPass::endAnim(AnimV20::AnimcharBaseComponent *animchar)
{
  for (int i = 0; i < fadeOutStates.size(); i++)
    animchar->getAnimGraph()->enqueueState(*animchar->getAnimState(), animchar->getAnimGraph()->getState(fadeOutStates[i]),
      fadeOutDur);
}

void Animate2ndPassCtx::Ctrl::initAnimState(const GeomNodeTree &tree)
{
  Animate2ndPass &anim = getAnim2ndPass();
  state.reset(new AnimV20::AnimCommonStateHolder(anim.getGraph()));
  animMap.clear();
  animMap.reserve(tree.nodeCount());
  for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
  {
    AnimMap am;
    am.animId = anim.getGraph().getNodeId(tree.getNodeName(i));
    if (am.animId != -1)
    {
      if (!anim.getA2D().getPoint3Anim(AnimV20::CHTYPE_POSITION, tree.getNodeName(i)) &&
          !anim.getA2D().getPoint3Anim(AnimV20::CHTYPE_SCALE, tree.getNodeName(i)) &&
          !anim.getA2D().getQuatAnim(AnimV20::CHTYPE_ROTATION, tree.getNodeName(i)))
        continue; // skip not-animated node
      am.geomId = i;
      animMap.push_back(am);
    }
  }
  animMap.shrink_to_fit();
}

void Animate2ndPassCtx::Ctrl::update(real dt, GeomNodeTree &tree, AnimV20::AnimcharBaseComponent *animchar)
{
  Animate2ndPass &anim = getAnim2ndPass();
  if (!animchar || !anim.isLoaded())
    return;
  if (!state)
    initAnimState(tree);
  float w = anim.getAnimWeightForTime(curT);
  if (animMap.size() && w > 0)
  {
    AnimV20::AnimBlender::TlsContext &tls = anim.getGraph().selectBlenderCtx();
    state->setParam(2, curT);
    anim.getGraph().blend(tls, *state, NULL);
    if (w > 0.999)
      for (int i = 0, e = animMap.size(); i < e; i++)
        anim.getGraph().getNodeTm(tls, animMap[i].animId, tree.getNodeTm(animMap[i].geomId), NULL);
    else
    {
      vec4f wt = v_splat4(&w);
      for (int i = 0, e = animMap.size(); i < e; i++)
      {
        const vec4f *p1 = NULL, *s1 = NULL;
        const quat4f *r1 = NULL;
        if (anim.getGraph().getNodePRS(tls, animMap[i].animId, &p1, &r1, &s1, NULL))
        {
          vec4f p0, s0;
          quat4f r0;
          v_mat4_decompose(tree.getNodeTm(animMap[i].geomId), p0, r0, s0);
          if (p1)
            p0 = v_lerp_vec4f(wt, p0, *p1);
          if (r1)
            r0 = v_quat_qslerp(w, r0, *r1);
          if (s1)
            s0 = v_lerp_vec4f(wt, s0, *s1);

          v_mat44_compose(tree.getNodeTm(animMap[i].geomId), p0, r0, s0);
        }
      }
    }
    tree.invalidateWtm(animMap[0].geomId.preceeding());
    vec3f world_translate = tree.translateToZero();
    tree.calcWtm();
    tree.translate(world_translate);
  }

  if (curT <= anim.getFadeOutStartTime() && anim.getFadeOutStartTime() < curT + dt)
    anim.endAnim(animchar);
  if (curT > anim.getDuration() && animchar->getPostController() == this)
    animchar->setPostController(NULL);
  curT += dt;
}
