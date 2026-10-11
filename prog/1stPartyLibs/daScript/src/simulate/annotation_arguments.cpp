#include "daScript/misc/platform.h"

#include "daScript/simulate/annotation_arguments.h"

#include <algorithm>

namespace das {

    const AnnotationArgument * AnnotationArgumentList::find ( const string & name, Type type ) const {
        auto it = find_if(begin(), end(), [&](const AnnotationArgument & arg){
            return (arg.name==name) && (type==Type::tVoid || type==arg.type);
        });
        return it==end() ? nullptr : &*it;
    }

    bool AnnotationArgumentList::getBoolOption(const string & name, bool def) const {
        auto arg = find(name, Type::tBool);
        return arg ? arg->bValue : def;
    }

    int32_t AnnotationArgumentList::getIntOption(const string & name, int32_t def) const {
        auto arg = find(name, Type::tInt);
        return arg ? arg->iValue : def;
    }

    uint64_t AnnotationArgumentList::getUInt64Option(const string & name, uint64_t def) const {
        auto arg = find(name, Type::tInt);
        return arg ? uint64_t(arg->iValue) : def;
    }

    uint64_t AnnotationArgumentList::getUInt64OptionEx(const string & name, const string & name2, uint64_t def) const {
        auto arg = find(name, Type::tInt);
        if (arg) return uint64_t(arg->iValue);
        arg = find(name2, Type::tInt);
        return arg ? uint64_t(arg->iValue) : def;
    }
}
