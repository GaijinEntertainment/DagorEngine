#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_profile.h"

#include "misc/include_fmt.h"

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
            // fmt::format return std::string, which is not always das::string.
            ss << "\"" << category << "\", " << fmt::format("{:.9f}",tSec).c_str() << ", " << count << "\n";
            context->to_out(at, ss.str().c_str());
        }
        return (float) tSec;
    }
}

