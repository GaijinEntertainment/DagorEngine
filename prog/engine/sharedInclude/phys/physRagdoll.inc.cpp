#include <phys/dag_physRagdoll.h>
#include <phys/dag_physics.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomNodeUtils.h>
#include <math/twistCtrl.h>
#include <stddef.h>
#include <perfMon/dag_statDrv.h>

PhysRagdoll::PhysRagdoll() :
  physSys(NULL),
  physWorld(NULL),
  dynamicClipoutT(0.0f),
  addVelImpulse(0, 0, 0),
  recalcWtms(false),
  ccdMode(false),
  shouldOverrideVelocity(false),
  shouldOverrideOmega(false)
{
  dynamicClipoutPos.zero();
  overrideVelocity.zero();
  overrideOmega.zero();
  clear_and_shrink(nodeBody);
}


PhysRagdoll::~PhysRagdoll() { del_it(physSys); }

void PhysRagdoll::initNodeHelpers(const GeomNodeTree &tree, dag::Span<TMatrix> last_node_tms)
{
  G_ASSERT(last_node_tms.empty() || last_node_tms.size() == tree.nodeCount());
  nodeHelpers.resize(tree.nodeCount());
  for (int i = 0; i < nodeHelpers.size(); ++i)
  {
    nodeHelpers[i] = physSys->getTmHelper(tree.getNodeName(dag::Index16(i)));
    if (last_node_tms.empty())
      continue;
    else if (nodeHelpers[i])
      tree.getNodeWtmScalar(dag::Index16(i), last_node_tms[i]);
    else
      last_node_tms[i] = TMatrix::IDENT;
  }
}

void PhysRagdoll::setBodiesTm(const GeomNodeTree &tree, dag::Span<TMatrix> last_node_tms, float dt)
{
  G_ASSERT(last_node_tms.empty() || last_node_tms.size() == tree.nodeCount());

  for (int i = 0; i < nodeHelpers.size(); ++i)
    if (nodeHelpers[i])
    {
      TMatrix _tm;
      tree.getNodeWtmScalar(dag::Index16(i), _tm);
      if (shouldOverrideVelocity || last_node_tms.empty())
        physSys->setBodyTmByTmHelper(tree.getNodeName(dag::Index16(i)), _tm);
      else
      {
        last_node_tms[i].setcol(3, last_node_tms[i].getcol(3) - addVelImpulse * dt);
        physSys->setBodyTmAndVelByTmHelper(tree.getNodeName(dag::Index16(i)), last_node_tms[i], _tm, dt);
      }
    }
}

void PhysRagdoll::applyOverriddenVelocities()
{
  if (shouldOverrideVelocity || shouldOverrideOmega)
    for (int i = 0; i < physSys->getBodyCount(); ++i)
    {
      PhysBody *body = physSys->getBody(i);
      if (shouldOverrideVelocity)
        body->setVelocity(overrideVelocity + addVelImpulse);
      if (shouldOverrideOmega)
        body->setAngularVelocity(overrideOmega);
    }
}

void PhysRagdoll::update(real dt, GeomNodeTree &tree, AnimV20::AnimcharBaseComponent *)
{
  if (!physSys)
    return;

  TIME_PROFILE(ragdoll_update);

  dynamicClipoutT -= dt;

  if (DAGOR_UNLIKELY(!lastNodeTms.empty()))
  {
    if (dt <= 0)
      return;
    tree.calcWtm();
    setBodiesTm(tree, make_span(lastNodeTms), dt);
    applyOverriddenVelocities();
    clear_and_shrink(lastNodeTms);
  }
  if (DAGOR_UNLIKELY(nodeHelpers.empty()))
  {
    clear_and_resize(lastNodeTms, tree.nodeCount());
    initNodeHelpers(tree, make_span(lastNodeTms));
    return;
  }
  else
    G_ASSERT(nodeHelpers.size() == tree.nodeCount());

  if (!nodeAlignCtrl.size() && physRes && physRes->getNodeAlignCtrl().size())
  {
    dag::ConstSpan<PhysicsResource::NodeAlignCtrl> c = physRes->getNodeAlignCtrl();
    nodeAlignCtrl.reserve(c.size());
    for (int i = 0; i < c.size(); i++)
    {
      NodeAlignCtrl &ctrl = nodeAlignCtrl.push_back();
      ctrl.node0Id = tree.findNodeIndex(c[i].node0);
      ctrl.node1Id = tree.findNodeIndex(c[i].node1);
      ctrl.angDiff = c[i].angDiff;
      for (int j = 0; j < countof(c[i].twist) && !c[i].twist[j].empty(); j++)
      {
        ctrl.twistId[j] = tree.findNodeIndex(c[i].twist[j]);
        if (!ctrl.twistId[j])
        {
          ctrl.twistCnt = 0;
          break;
        }
        ctrl.twistCnt = j + 1;
      }
      if (!ctrl.node0Id || !ctrl.node1Id || !ctrl.twistCnt)
        nodeAlignCtrl.pop_back();
    }
  }

  physSys->updateTms();

  for (dag::Index16 i(0), ie(nodeHelpers.size()); i != ie; ++i)
  {
    if (!nodeHelpers[i.index()])
      continue;
    if (recalcWtms)
      tree.partialCalcWtm(i);
    tree.setNodeWtmScalar(i, *nodeHelpers[i.index()]);
    if (recalcWtms)
      tree.invalidateWtm(i);
  }
  for (int j = 0; j < nodeAlignCtrl.size(); j++)
    apply_twist_ctrl(tree, nodeAlignCtrl[j].node0Id, nodeAlignCtrl[j].node1Id,
      make_span(nodeAlignCtrl[j].twistId, nodeAlignCtrl[j].twistCnt), nodeAlignCtrl[j].angDiff);
}


PhysRagdoll *PhysRagdoll::create(PhysicsResource *phys, PhysWorld *world)
{
  PhysRagdoll *rd = new (midmem) PhysRagdoll;
  rd->init(phys, world);
  return rd;
}


void PhysRagdoll::init(PhysicsResource *phys, PhysWorld *world)
{
  physRes = phys;
  physWorld = world;
}

void PhysRagdoll::resizeCollMap(int size)
{
  nodeBody.resize(size);
  for (int i = 0; i < nodeBody.size(); i++)
    nodeBody[i] = -1;
}

void PhysRagdoll::addCollNode(int node_id, int body_id) { nodeBody[node_id] = body_id; }

void PhysRagdoll::setStartAddLinVel(const Point3 &add_vel) { addVelImpulse = add_vel; }

void PhysRagdoll::setDynamicClipout(const Point3 &toPos, float timeout)
{
  dynamicClipoutPos = toPos;
  dynamicClipoutT = timeout;
}

void PhysRagdoll::setOverrideVel(const Point3 &vel)
{
  overrideVelocity = vel;
  shouldOverrideVelocity = true;
}

void PhysRagdoll::setOverrideOmega(const Point3 &omega)
{
  overrideOmega = omega;
  shouldOverrideOmega = true;
}

void PhysRagdoll::startRagdoll(int interact_layer, int interact_mask, const GeomNodeTree *tree)
{
  if (physSys) // Wake up if it's already started
    wakeUp();
  if (physSys || !physRes || !physWorld)
    return;

  TIME_PROFILE(startRagdoll);

  physSys = new PhysSystemInstance(physRes, physWorld, nullptr, &ud, interact_layer, interact_mask);
  clear_and_shrink(lastNodeTms);

  if (tree)
  {
    if (nodeHelpers.empty())
    {
      if (!shouldOverrideVelocity)
        clear_and_resize(lastNodeTms, tree->nodeCount());
      initNodeHelpers(*tree, make_span(lastNodeTms));
    }
    setBodiesTm(*tree);
  }

  for (int i = 0; i < physSys->getBodyCount(); ++i)
  {
    PhysBody *body = physSys->getBody(i);
    body->setContinuousCollisionMode((body->getGroupMask() | body->getInteractionLayer()) && ccdMode); // Ignore empty collision
    if (dynamicClipoutT > 0.0f)
      body->setPreSolveCallback(&PhysRagdoll::onPreSolve);
    physWorld->addBody(body, /* kinematic */ false); // After `setBodiesTm` (for correct broadphase quadtree setup)
    if (tree)                                        // After world addition
    {
      if (shouldOverrideVelocity)
        body->setVelocity(overrideVelocity + addVelImpulse);
      if (shouldOverrideOmega)
        body->setAngularVelocity(overrideOmega);
    }
  }
}


void PhysRagdoll::endRagdoll()
{
  del_it(physSys);
  clear_and_shrink(nodeHelpers);
  clear_and_shrink(nodeAlignCtrl);
}

// sets ccd mode
void PhysRagdoll::setContinuousCollisionMode(bool mode) { ccdMode = mode; }

void PhysRagdoll::applyImpulse(int node_id, const Point3 &pos, const Point3 &impulse)
{
  if (!physSys)
    return;

  G_ASSERTF_RETURN(node_id >= 0 && node_id < nodeBody.size(), , "Invalid collision nodeId %d", node_id);
  if (nodeBody[node_id] >= 0)
  {
    PhysBody *body = physSys->getBody(nodeBody[node_id]);
    body->addImpulse(pos, impulse);
  }
}

void PhysRagdoll::onPreSolve(PhysBody *body, void *other, const Point3 &pos, Point3 &norm, IPhysContactData *&contact_data)
{
  void *ud = body->getUserData();
  G_ASSERT(ud && ((const PhysObjectUserData *)ud)->physObjectType == _MAKE4C('PRGD'));
  auto ragdoll = (PhysRagdoll *)(void *)((char *)ud - offsetof(PhysRagdoll, ud));
  G_ASSERT(ragdoll);
  if ((ragdoll->dynamicClipoutT > 0.0f) && ((ragdoll->dynamicClipoutPos - pos) * norm < 0))
  {
    // Handle dynamic clipout. If contact normal is facing away from our initial ragdoll position (e.g. real collision capsule)
    // then we're inverting it, making physics solver push out the body in the direction of initial position. This effectively prevents
    // ragdolls stucking in walls, since initial position (e.g. collision capsule) is known for sure to be not inside some geometry.
    // We also have some timeout on this, we don't want to do it forever since ragdoll can fly far away from original position, in that
    // case we can't be sure that clipping out this way is the right thing to do.
    norm = -norm;
  }
}


bool PhysRagdoll::wakeUp()
{
  for (int i = 0, bodiesCount = physSys ? physSys->getBodyCount() : 0; i < bodiesCount; ++i)
    physSys->getBody(i)->wakeUp();
  return physSys != nullptr;
}
