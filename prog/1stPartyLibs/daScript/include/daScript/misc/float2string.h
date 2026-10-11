#pragma once

#include "daScript/misc/platform.h"

namespace das {
    enum { DAS_F2S_BUFFER_SIZE = 48 };
    // shortest round-trip float printing, spelled byte-for-byte as fmt's default "{}";
    // returns the end of the written text, which is NOT null-terminated; the buffer is at
    // least DAS_F2S_BUFFER_SIZE bytes - the emitter's fixed-length copies overshoot the text
    DAS_API char * float2string ( char * buf, float value );
    DAS_API char * double2string ( char * buf, double value );
}
