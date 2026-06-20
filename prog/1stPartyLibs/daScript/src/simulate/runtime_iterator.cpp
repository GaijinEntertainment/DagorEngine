#include "daScript/simulate/simulate.h"
#include "daScript/simulate/runtime_iterator.h"

namespace das {
    bool PointerDimIterator::first ( Context &, char * _value ) {
        char ** value = (char **) _value;
        if ( data != data_end ) {
            *value = *data;
            return true;
        } else {
            return false;
        }
    }

    bool PointerDimIterator::next  ( Context &, char * _value ) {
        char ** value = (char **) _value;
        if ( ++data != data_end ) {
            *value = *data;
            return true;
        } else {
            return false;
        }
    }

    void PointerDimIterator::close ( Context & context, char * _value ) {
        if ( _value ) {
            char ** value = (char **) _value;
            *value = nullptr;
        }
        context.freeIterator((char *)this, debugInfo);
    }

}
