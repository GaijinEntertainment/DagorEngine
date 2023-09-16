#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_range.h"

extern "C" int64_t ref_time_ticks ();
extern "C" int get_time_usec (int64_t reft);

namespace das
{
    float builtin_profile ( int32_t count, const char * category, const Block & block, Context * context, LineInfoArg * at ) {
        count = das::max(count, 1);
        int minT = INT32_MAX;
        for ( int32_t i = 0; i != count; ++i ) {
            int64_t reft = ref_time_ticks();
            context->invoke(block, nullptr, nullptr, at);
            minT = das::min(get_time_usec(reft), minT);
        }
        double tSec = minT/1000000.;
        if ( category ) {
            TextWriter ss;
            ss << "\"" << category << "\", " << tSec << ", " << count << "\n";
            context->to_out(at, ss.str().c_str());
        }
        return (float) tSec;
    }
}

