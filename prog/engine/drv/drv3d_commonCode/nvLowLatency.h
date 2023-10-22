#pragma once

namespace nvlowlatency
{
// only needs to be called once at startup
// in case of deferred_render_submission RENDERSUBMIT_START and RENDERSUBMIT_END markers are
// required,
//  RENDERRECORD_START and RENDERRECORD_END markers are optional (only used for debugging)
// otherwise RENDERRECORD_START and RENDERRECORD_END are equivalent to
//  RENDERSUBMIT_START/RENDERSUBMIT_END. Only one type (submit/record)
//  should be called
void init(void *device, bool deferred_render_submission);
void close();
} // namespace nvlowlatency
