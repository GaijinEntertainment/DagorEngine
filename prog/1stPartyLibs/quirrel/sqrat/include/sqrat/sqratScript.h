// Sqrat: altered version by Gaijin Games KFT
// SqratScript: Script Compilation and Execution
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
#if !defined(_SQRAT_SCRIPT_H_)
#define _SQRAT_SCRIPT_H_

#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>

#include "sqratObject.h"


namespace Sqrat {

/// Helper class for managing Squirrel scripts
class Script : public Object {
public:

    Script(HSQUIRRELVM v) : Object(v) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a string containing a Squirrel script
    ///
    /// \param script String containing a file path to a Squirrel script
    /// \param errMsg String that is filled with any errors that may occur
    /// \param name   Optional string containing the script's name (for errors)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(SQRAT_HAS_STRING_VIEW)
    bool CompileString(const string_view &script, string &errMsg,
                       const string_view &name = string_view(),
                       const HSQOBJECT *bindings=nullptr)
#else
    bool CompileString(const string &script, string &errMsg,
                       const string &name = string(),
                       const HSQOBJECT *bindings=nullptr)
#endif
    {
        if(!sq_isnull(obj)) {
            sq_release(vm, &obj);
            sq_resetobject(&obj);
        }

        if(SQ_FAILED(sq_compilebuffer(vm, script.data(), static_cast<SQInteger>(script.size() /** sizeof(SQChar)*/), name.data(), true, bindings))) {
            errMsg = LastErrorString(vm);
            return false;
        }

        sq_getstackobj(vm,-1,&obj);
        sq_addref(vm, &obj);
        sq_pop(vm, 1);
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a file containing a Squirrel script
    ///
    /// \param path   File path containing a Squirrel script
    /// \param errMsg String that is filled with any errors that may occur
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool CompileFile(const string& path, string& errMsg) {
        if(!sq_isnull(obj)) {
            sq_release(vm, &obj);
            sq_resetobject(&obj);
        }

        if(SQ_FAILED(sqstd_loadfile(vm, path.c_str(), true))) {
            errMsg = LastErrorString(vm);
            return false;
        }

        sq_getstackobj(vm,-1,&obj);
        sq_addref(vm, &obj);
        sq_pop(vm, 1);
        return true;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Runs the script
    ///
    /// \param errMsg String that is filled with any errors that may occur
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Run(string& errMsg, Object * context = NULL) {
        if(!sq_isnull(obj)) {
            SQRESULT result;
            SQInteger top = sq_gettop(vm);
            sq_pushobject(vm, obj);

            if (!context)
              sq_pushroottable(vm);
            else
              sq_pushobject(vm, context->GetObject());

            result = sq_call(vm, 1, false, true);
            sq_settop(vm, top);
            if(SQ_FAILED(result)) {
                errMsg = LastErrorString(vm);
                return false;
            }
            return true;
        }
        return false;
    }
};

}

#endif
