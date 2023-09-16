#include "varMap.h"
#include "hashStrings.h"
#include "nameMap.h"


namespace VarMap
{
static SCFastNameMap map;

void init() {}
void release() { clear(); }

void clear() { map.reset(); }

int getVarId(const char *var_name) { return map.getNameId(var_name); }
int addVarId(const char *var_name) { return map.addNameId(var_name); }

int getIdCount() { return map.nameCount(); }
const char *getName(int id) { return map.getName(id); }
} // namespace VarMap
