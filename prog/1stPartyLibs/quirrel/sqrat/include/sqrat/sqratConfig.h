#include <util/dag_globDef.h>
#include <dag/dag_relocatable.h>

#define SQRAT_ASSERT G_ASSERT
#define SQRAT_ASSERTF G_ASSERTF
#define SQRAT_VERIFY G_VERIFY

#define SQRAT_HAS_SKA_HASH_MAP
#define SQRAT_HAS_EASTL

#define SQRAT_STD eastl


namespace Sqrat
{
    class Object;
    class Function;
}

DAG_DECLARE_RELOCATABLE(Sqrat::Object);
DAG_DECLARE_RELOCATABLE(Sqrat::Function);
