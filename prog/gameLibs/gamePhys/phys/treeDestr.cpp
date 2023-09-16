#include <gamePhys/phys/treeDestr.h>

static rendinstdestr::TreeDestr tree_destr;

float rendinstdestr::TreeDestr::getRadiusToImpulse(float radius) const
{
  return radiusToImpulse.empty() ? 1.f : radiusToImpulse.interpolate(radius);
}

void rendinstdestr::tree_destr_load_from_blk(const DataBlock &blk)
{
  tree_destr.heightThreshold = blk.getReal("heightThreshold", 5.0f);
  tree_destr.radiusThreshold = blk.getReal("radiusThreshold", 1.0f);
  tree_destr.impulseThresholdMult = blk.getReal("impulseThresholdMult", 1000.0f);
  tree_destr.canopyRestitution = blk.getReal("canopyRestitution", 0.0f);
  tree_destr.canopyStiffnessMult = blk.getReal("canopyStiffnessMult", 1.0f);
  tree_destr.canopyAngularLimit = blk.getReal("canopyAngularLimit", 0.3f);
  tree_destr.canopyMaxRaduis = blk.getReal("canopyMaxRaduis", 5.f);
  tree_destr.canopyDensity = blk.getReal("canopyDensity", 60.f);
  tree_destr.canopyRollingFriction = blk.getReal("canopyRollingFriction", 0.1f);
  tree_destr.canopyInertia = blk.getPoint3("canopyInertia", Point3(1.f, 1.f, 1.f));
  tree_destr.treeDensity = blk.getReal("treeDensity", 1000.f);
  tree_destr.minSpeed = blk.getReal("minSpeed", 5.f);
  tree_destr.maxLifeDistance = blk.getReal("maxLifeDistance", -1.f);
  tree_destr.constraintLimitY = blk.getPoint2("constraintLimitY", Point2(0.f, 0.f));
  tree_destr.canopyLinearDamping = blk.getReal("canopyLinearDamping", 0.9f);
  tree_destr.canopyAngularDamping = blk.getReal("canopyAngularDamping", 0.9f);

  read_interpolate_tab_float_p2(tree_destr.radiusToImpulse, blk.getBlockByNameEx("radiusToImpulse"));
}

const rendinstdestr::TreeDestr &rendinstdestr::get_tree_destr() { return tree_destr; }