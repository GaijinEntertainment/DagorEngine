#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

namespace dafx
{
struct CommandQueue
{
  struct CreateInstance
  {
    SystemId sid;
    InstanceId iid;
  };

  struct InstancePos
  {
    InstanceId iid;
    Point4 pos;
  };

  struct InstanceWarmup
  {
    InstanceId iid;
    float time;
  };

  struct InstanceEmissionRate
  {
    InstanceId iid;
    float rate;
  };

  struct InstanceStatus
  {
    InstanceId iid;
    bool enabled;
  };

  struct InstanceVisibility
  {
    InstanceId iid;
    bool visible;
  };

  struct InstanceValue
  {
    InstanceId iid;
    bool isOpt;
    int size;
    eastl::string name;
    int srcOffset;
  };

  struct InstanceValueDirect
  {
    InstanceId iid;
    int subIdx;
    int size;
    int srcOffset;
    int dstOffset;
  };

  eastl::vector<CreateInstance> createInstance;
  eastl::vector<InstanceId> destroyInstance;
  eastl::vector<InstanceId> resetInstance;

  eastl::vector<InstancePos> instancePos;
  eastl::vector<InstanceStatus> instanceStatus;
  eastl::vector<InstanceVisibility> instanceVisibility;
  eastl::vector<InstanceWarmup> instanceWarmup;
  eastl::vector<InstanceEmissionRate> instanceEmissionRate;
  eastl::vector<InstanceValue> instanceValue;
  eastl::vector<InstanceValueDirect> instanceValueDirect;

  eastl::vector<unsigned char> instanceValueData;
};
} // namespace dafx