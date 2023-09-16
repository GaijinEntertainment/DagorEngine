#include <phys/dag_physStat.h>

int PhysCounters::rayCastObjCreated = 0;
int PhysCounters::rayCastUpdate = 0;
int PhysCounters::memoryUsed = 0;
int PhysCounters::memoryUsedMax = 0;

void PhysCounters::resetCounters()
{
  rayCastObjCreated = 0;
  rayCastUpdate = 0;
  memoryUsedMax = memoryUsed;
}
