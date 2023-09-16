#pragma once

/** \addtogroup de3Common
  @{
*/

#include "de3_debugObject.h"
#include <phys/dag_physDebug.h>

/**
\brief Initialize/terminate debug physics rendering.

\param[in] #PhysWorld physical world.

@see PhysWorld physics_draw_debug()
*/
static void physics_init_draw_debug(PhysWorld *) { physdbg::init<PhysWorld>(); }
static void physics_term_draw_debug(PhysWorld *) { physdbg::term<PhysWorld>(); }

/**
\brief Draws debug bounding bodies.

\param[in] #PhysWorld physical world.

@see PhysWorld physics_init_draw_debug()
*/
static void physics_draw_debug(PhysWorld *physWorld)
{
  physdbg::renderWorld(physWorld,
    physdbg::RenderFlag::BODIES | physdbg::RenderFlag::CONSTRAINTS | physdbg::RenderFlag::CONSTRAINT_LIMITS |
      physdbg::RenderFlag::CONSTRAINT_REFSYS |
      /*physdbg::RenderFlag::BODY_BBOX|physdbg::RenderFlag::BODY_CENTER|*/ physdbg::RenderFlag::CONTACT_POINTS,
    &::grs_cur_view.pos, 200);
}

/** @} */
