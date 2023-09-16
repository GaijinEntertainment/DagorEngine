#include <vulkan/vulkan.h>

namespace drv3d_vulkan
{
struct DriverInfo
{};
namespace nvidia
{
void get_driver_info(DriverInfo &info);
}
namespace amd
{
void get_driver_info(DriverInfo &info);
}
namespace intel
{
inline void get_driver_info(DriverInfo &info) { (void)info; }
} // namespace intel
} // namespace drv3d_vulkan