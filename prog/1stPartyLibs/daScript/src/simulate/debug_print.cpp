#include "daScript/misc/platform.h"

#include "daScript/simulate/debug_print.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/debug_break.h"

namespace das {

    string human_readable_formatting ( const string & str ) {
        string result;
        string tab;
        auto it = str.begin();
        auto tail = str.end();
        while ( it != tail ) {
            while ( it!=tail && *it==' ') {
                it ++;
            }
            while ( it!=tail && *it!='\n' ) {
                auto Ch = *it;
                result += Ch;
                if ( Ch=='[' ) tab += "  ";
                else if ( Ch==']' ) {
                    if ( tab.size()>=2 ) tab.resize(tab.size()-2); // note: we don't track mismatch
                }
                it ++;
            }
            if ( it == tail ) break;
            while ( it!=tail && *it=='\n' ) {
                it ++;
            }
            result += "\n";
            result += tab;
        }
        return result;
    }

    string debug_value ( void * pX, TypeInfo * info, PrintFlags flags ) {
        TextWriter ss;
        DebugDataWalker<TextWriter> walker(ss,flags);
        walker.walk((char*)pX,info);
        if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
            return human_readable_formatting(ss.str());
        } else {
            return ss.str();
        }
    }

    string debug_value ( vec4f value, TypeInfo * info, PrintFlags flags ) {
        TextWriter ss;
        DebugDataWalker<TextWriter> walker(ss,flags);
        walker.walk(value,info);
        if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
            return human_readable_formatting(ss.str());
        } else {
            return ss.str();
        }
    }
}
