#include <gamePhys/phys/physVars.h>
#include <debug/dag_assert.h>

int PhysVars::registerVarImpl(const char *name, float val, bool pull)
{
  int nid = names.getNameId(name);
  if (nid < 0)
  {
    nid = names.addNameId(name);
    vars.resize(names.nameCount());
    vars[nid] = val;
    if (pull)
      varsPullable.set(nid, true);
  }
  return nid;
}

int PhysVars::registerVar(const char *name, float val) { return registerVarImpl(name, val, false); }
int PhysVars::registerPullVar(const char *name, float val) { return registerVarImpl(name, val, true); }

float PhysVars::getVar(int var_id) const
{
  G_ASSERTF_RETURN(var_id >= 0 && var_id < vars.size(), 0.f, "Invalid var_id %d (%d total)", var_id, (int)vars.size());
  return vars[var_id];
}

void PhysVars::setupVar(const char *name, float val) { setVar(registerVar(name, val), val); }
