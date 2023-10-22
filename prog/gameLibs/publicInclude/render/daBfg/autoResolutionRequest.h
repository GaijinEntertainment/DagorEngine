#pragma once

#include <render/daBfg/detail/autoResTypeNameId.h>

#include <math/integer/dag_IPoint2.h>


namespace dabfg
{

struct InternalRegistry;
namespace detail
{
struct VirtualResourceRequestBase;
}

/**
 * \brief This class represents a daBfg-managed automatic resolution
 * type for a 2D texture. If this resolution is specified for a texture,
 * the actual texture's resolution at runtime will be the dynamic
 * resolution scaled by the multiplier, but the consumed memory will
 * always be equal to the static resolution times the multiplier.
 * See NameSpace::setResolution and NameSpace::setDynamicResolution.
 * Note that objects of this type MAY be captured into the execution
 * callback and used to access the actual resolution on a particular
 * frame, but the resolution should NEVER be accessed in the declaration
 * callback, as the value will be undefined.
 */
class AutoResolutionRequest
{
  friend class NameSpaceRequest;
  friend struct detail::VirtualResourceRequestBase;

  AutoResolutionRequest(AutoResTypeNameId id, float mult, InternalRegistry *r) : autoResTypeId{id}, multiplier{mult}, registry{r} {}

public:
  /**
   * \brief Returns the current dynamic resolution for this auto-res type.
   * \warning Should only be used for setting the d3d viewport/scissor, NEVER create
   * textures with this resolution, as it might be changing every single frame!!!
   * Also never call this outside of the execution callback for the same reason!
   *
   * \return The current dynamic resolution for this type.
   */
  IPoint2 get() const;

private:
  AutoResTypeNameId autoResTypeId;
  float multiplier = 1.f;
  InternalRegistry *registry;
};

} // namespace dabfg