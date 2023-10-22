#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <phys/dag_physResource.h>
#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_localConv.h>

// #include <debug/dag_debug.h>


PhysicsResource::PhysicsResource() :
  bodies(midmem), rdBallJoints(midmem), rdHingeJoints(midmem), revoluteJoints(midmem), sphericalJoints(midmem)
{}


void PhysicsResource::load(IGenLoad &cb, int load_flags)
{
  G_UNREFERENCED(load_flags);
  cb.beginBlock();

  while (cb.getBlockRest() > 0)
  {
    cb.beginBlock();
    int id = cb.readInt();

    if (id == MAKE4C('b', 'o', 'd', 'y'))
    {
      int bi = append_items(bodies, 1);
      bodies[bi].load(cb);
    }
    else if (id == _MAKE4C('RJba'))
    {
      RdBallJoint &jnt = rdBallJoints.push_back();
      cb.readInt(jnt.body1);
      cb.readInt(jnt.body2);
      cb.read(&jnt.tm, sizeof(jnt.tm));
      cb.read(&jnt.minLimit, sizeof(jnt.minLimit));
      cb.read(&jnt.maxLimit, sizeof(jnt.maxLimit));
      cb.readReal(jnt.damping);
      cb.readReal(jnt.twistDamping);
      cb.readReal(jnt.stiffness);
      if (cb.getBlockRest())
        cb.readString(jnt.name);
    }
    else if (id == _MAKE4C('RJhi'))
    {
      RdHingeJoint &jnt = rdHingeJoints.push_back();
      cb.readInt(jnt.body1);
      cb.readInt(jnt.body2);
      cb.read(&jnt.pos, sizeof(jnt.pos));
      cb.read(&jnt.axis, sizeof(jnt.axis));
      cb.read(&jnt.midAxis, sizeof(jnt.midAxis));
      cb.read(&jnt.xAxis, sizeof(jnt.xAxis));
      cb.readReal(jnt.angleLimit);
      cb.readReal(jnt.damping);
      cb.readReal(jnt.stiffness);
      if (cb.getBlockRest())
        cb.readString(jnt.name);
    }
    else if (id == _MAKE4C('Jrev'))
    {
      RevoluteJoint &jnt = revoluteJoints.push_back();
      cb.readInt(jnt.body1);
      cb.readInt(jnt.body2);
      cb.read(&jnt.pos, sizeof(jnt.pos));
      cb.read(&jnt.axis, sizeof(jnt.axis));

      cb.readReal(jnt.minAngle);
      cb.readReal(jnt.maxAngle);

      cb.readReal(jnt.maxRestitution);
      cb.readReal(jnt.minRestitution);
      cb.readReal(jnt.projAngle);
      cb.readReal(jnt.projDistance);
      cb.readReal(jnt.spring);
      jnt.projType = cb.readIntP<2>();
      jnt.flags = cb.readIntP<2>();

      cb.readReal(jnt.damping);
      if (cb.getBlockRest())
        cb.readString(jnt.name);
    }
    else if (id == _MAKE4C('Jsph'))
    {
      SphericalJoint &jnt = sphericalJoints.push_back();
      cb.readInt(jnt.body1);
      cb.readInt(jnt.body2);

      cb.read(&jnt.pos, sizeof(jnt.pos));
      cb.read(&jnt.dir, sizeof(jnt.dir));
      cb.read(&jnt.axis, sizeof(jnt.axis));

      cb.readReal(jnt.minAngle);
      cb.readReal(jnt.maxAngle);
      cb.readReal(jnt.maxRestitution);
      cb.readReal(jnt.minRestitution);
      cb.readReal(jnt.swingValue);
      cb.readReal(jnt.swingRestitution);

      cb.readReal(jnt.spring);
      cb.readReal(jnt.damping);
      cb.readReal(jnt.twistSpring);
      cb.readReal(jnt.twistDamping);
      cb.readReal(jnt.swingSpring);
      cb.readReal(jnt.swingDamping);

      cb.readReal(jnt.projDistance);
      jnt.projType = cb.readIntP<2>();
      jnt.flags = cb.readIntP<2>();

      if (cb.getBlockRest())
        cb.readString(jnt.name);
    }
    else if (id == _MAKE4C('Na1c')) // node align controller, type1, obsolete, skip it
    {}
    else if (id == _MAKE4C('Nt1c')) // node twist controller, type1
    {
      NodeAlignCtrl &c = nodeAlignCtrl.push_back();
      cb.readString(c.node0);
      cb.readString(c.node1);
      c.angDiff = cb.readReal();
      for (int i = 0; i < countof(c.twist) && cb.getBlockRest(); i++)
        cb.readString(c.twist[i]);
    }
    else
      logerr("Unknown block in physics resource: '%c%c%c%c'", DUMP4C(id));

    cb.endBlock();
  }

  cb.endBlock();
}


bool PhysicsResource::getBodyTm(const char *name, TMatrix &tm)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    Body &b = bodies[i];
    if (dd_stricmp(b.name, name) != 0)
      continue;
    tm = b.tm;
    return true;
  }

  return false;
}


bool PhysicsResource::getTmHelperWtm(const char *name, TMatrix &wtm)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    Body &b = bodies[i];
    int id = b.findTmHelper(name);
    if (id < 0)
      continue;

    wtm = b.tm * b.tmHelpers[id].tm;
    return true;
  }

  return false;
}


void PhysicsResource::Body::load(IGenLoad &cb)
{
  cb.readString(name);

  cb.read(&tm, sizeof(tm));
  tmInvert = inverse(tm);
  cb.read(&mass, sizeof(mass));
  cb.read(&momj, sizeof(momj));

  while (cb.getBlockRest() > 0)
  {
    cb.beginBlock();
    int id = cb.readInt();

    if (id == MAKE4C('C', 's', 'p', 'h'))
    {
      SphColl &coll = sphColl.push_back();

      cb.read(&coll.center, sizeof(coll.center));
      cb.read(&coll.radius, sizeof(coll.radius));
      if (cb.getBlockRest())
        cb.readString(coll.materialName);
    }
    else if (id == MAKE4C('C', 'b', 'o', 'x'))
    {
      BoxColl &coll = boxColl.push_back();

      cb.read(&coll.tm, sizeof(coll.tm));
      cb.read(&coll.size, sizeof(coll.size));
      if (cb.getBlockRest())
        cb.readString(coll.materialName);
    }
    else if (id == MAKE4C('C', 'c', 'a', 'p'))
    {
      CapColl &coll = capColl.push_back();

      cb.read(&coll.center, sizeof(coll.center));
      cb.read(&coll.extent, sizeof(coll.extent));
      cb.read(&coll.radius, sizeof(coll.radius));
      if (cb.getBlockRest())
        cb.readString(coll.materialName);
    }
    else if (id == MAKE4C('H', 't', 'm', ' '))
    {
      TmHelper &helper = tmHelpers.push_back();

      cb.readString(helper.name);
      cb.read(&helper.tm, sizeof(helper.tm));

      // debug_ctx("added tm helper '%s'", (const char*)helper.name);
    }
    else if (id == MAKE4C('M', 'a', 't', ' '))
    {
      cb.readString(materialName);
    }
    else if (id == MAKE4C('A', 'l', 'i', 'a'))
    {
      cb.readString(aliasName);
    }
    else
      logerr("Unknown block in physics resource body: '%c%c%c%c'", DUMP4C(id));

    cb.endBlock();
  }
}


int PhysicsResource::Body::findTmHelper(const char *helper_name)
{
  for (int i = 0; i < tmHelpers.size(); ++i)
    if (strcmp(tmHelpers[i].name, helper_name) == 0)
      return i;

  return -1;
}

PhysicsResource *PhysicsResource::loadResource(IGenLoad &crd, int load_flags)
{
  PhysicsResource *r = new PhysicsResource;
  r->load(crd, load_flags);
  return r;
}
