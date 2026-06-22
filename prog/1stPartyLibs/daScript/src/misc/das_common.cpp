#include "daScript/misc/das_common.h"

namespace das {

    bool starts_with ( const string & name, const char * template_name ) {
        auto len = strlen(template_name);
        if ( name.size() < len ) return false;
        return name.compare(0,len,template_name) == 0;
    }
}
