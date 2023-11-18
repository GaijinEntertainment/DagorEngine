#include <render/daBfg/autoResolutionRequest.h>

#include <frontend/internalRegistry.h>


namespace dabfg
{

IPoint2 AutoResolutionRequest::get() const { return provider->resolutions[autoResTypeId]; }

} // namespace dabfg
