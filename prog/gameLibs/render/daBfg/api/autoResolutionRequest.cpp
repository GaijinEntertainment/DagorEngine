#include <render/daBfg/autoResolutionRequest.h>

#include <api/internalRegistry.h>


namespace dabfg
{

IPoint2 AutoResolutionRequest::get() const { return registry->autoResTypes[autoResTypeId].dynamicResolution; }

} // namespace dabfg
