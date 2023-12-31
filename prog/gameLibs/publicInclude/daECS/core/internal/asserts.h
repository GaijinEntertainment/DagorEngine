#pragma once

#if DAECS_EXTENSIVE_CHECKS
#define DAECS_VALIDATE_ARCHETYPE(archetypeId) \
  G_ASSERTF((archetypeId) == INVALID_ARCHETYPE || (archetypeId) < archetypes.size(), "%d", archetypeId)
#define DAECS_EXT_FAST_ASSERT     G_FAST_ASSERT
#define DAECS_EXT_ASSERT          G_ASSERT
#define DAECS_EXT_ASSERTF         G_ASSERTF
#define DAECS_EXT_ASSERT_RETURN   G_ASSERT_RETURN
#define DAECS_EXT_ASSERTF_RETURN  G_ASSERTF_RETURN
#define DAECS_EXT_ASSERT_CONTINUE G_ASSERT_CONTINUE
#else
#define DAECS_VALIDATE_ARCHETYPE(archetypeId)
#define DAECS_EXT_FAST_ASSERT(...)
#define DAECS_EXT_ASSERT(...)
#define DAECS_EXT_ASSERTF(...)
#define DAECS_EXT_ASSERT_RETURN(...)
#define DAECS_EXT_ASSERTF_RETURN(...)
#define DAECS_EXT_ASSERT_CONTINUE(...)
#endif
