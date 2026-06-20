#pragma once

#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/debug_info.h"

namespace das {

    struct DAS_API Iterator {
        Iterator(LineInfo * at) : debugInfo(at) {}
        virtual ~Iterator() {}
        virtual bool first ( Context & context, char * value ) = 0;
        virtual bool next  ( Context & context, char * value ) = 0;
        virtual void close ( Context & context, char * value ) = 0;    // can't throw
        virtual void walk ( DataWalker & ) { }
       bool isOpen = false;
       LineInfo * debugInfo;
    };

    struct PointerDimIterator : Iterator {
        PointerDimIterator  ( char ** d, uint32_t cnt, uint32_t sz, LineInfo * at )
            : Iterator(at), data(d), data_end(d+cnt), size(sz) {}
        virtual bool first(Context &, char * _value) override;
        virtual bool next(Context &, char * _value) override;
        virtual void close(Context & context, char * _value) override;
        char ** data;
        char ** data_end;
        uint32_t size;
    };

    struct Sequence {
        mutable Iterator * iter;
    };

}
