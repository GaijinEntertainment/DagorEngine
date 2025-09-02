// Sqrat: altered version by Gaijin Games KFT
// SqratConst: Constant and Enumeration Binding
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#pragma once
#if !defined(_SQRAT_CONST_H_)
#define _SQRAT_CONST_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"

namespace Sqrat {

class Enumeration : public Object {
public:
    Enumeration(HSQUIRRELVM v, bool createTable = true) : Object(v) {
        if(createTable) {
            sq_newtable(vm);
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&obj)));
            sq_addref(vm, &obj);
            sq_pop(vm,1);
        }
    }

    template <typename T,
              SQRAT_STD::enable_if_t<SQRAT_STD::is_integral_v<T> ||
                                     SQRAT_STD::is_floating_point_v<T> ||
                                     SQRAT_STD::is_enum_v<T>, int> = 0>
    Enumeration& Const(const SQChar* name, T val) {
        BindValue<T>(name, strlen(name), val, false);
        return *this;
    }

    Enumeration& Const(const SQChar* name, const SQChar* val) {
        BindValue<const SQChar*>(name, strlen(name), val, false);
        return *this;
    }
};


class ConstTable : public Enumeration {
public:
    ConstTable(HSQUIRRELVM v) : Enumeration(v, false) {
        sq_pushconsttable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1, &obj)));
        sq_addref(v, &obj);
        sq_pop(v, 1);
    }

    template <typename T>
    ConstTable& Const(const SQChar* name, T val) {
        Enumeration::Const(name, val);
        return *this;
    }

    ConstTable& Enum(const SQChar* name, Enumeration& en) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        sq_pushobject(vm, en.GetObject());
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm,1); // pop table
        return *this;
    }
};

}

#endif
