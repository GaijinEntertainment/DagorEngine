#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"

#if !defined(DAS_NO_FILEIO)
#include <sys/stat.h>
#endif

das::Context * get_context ( int stackSize=0 );

namespace das {

    ModuleFileAccess::ModuleFileAccess() {
    }

    ModuleFileAccess::~ModuleFileAccess() {
        if (context) delete context;
    }

    ModuleFileAccess::ModuleFileAccess ( const string & pak, const FileAccessPtr & access ) {
        ModuleGroup dummyLibGroup;
        TextWriter tout;
        if ( auto program = compileDaScript(pak, access, tout, dummyLibGroup) ) {
            if ( program->failed() ) {
                for ( auto & err : program->errors ) {
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                }
                DAS_FATAL_ERROR("failed to compile: %s\n%s\n", pak.c_str(), tout.str().c_str());
            } else {
                context = get_context(program->getContextStackSize());
                if ( !program->simulate(*context, tout) ) {
                    tout << "failed to simulate\n";
                    for ( auto & err : program->errors ) {
                        tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                    }
                    delete context;
                    context = nullptr;
                    DAS_FATAL_ERROR("failed to simulate: %s\n%s", pak.c_str(), tout.str().c_str());
                    return;
                }
                modGet = context->findFunction("module_get");
                DAS_ASSERTF(modGet!=nullptr, "can't find module_get function");
                includeGet = context->findFunction("include_get");          // note, this one CAN be null
                moduleAllowed = context->findFunction("module_allowed");    // note, this one CAN be null
                moduleUnsafe = context->findFunction("module_allowed_unsafe");    // note, this one CAN be null
                // get it ready
                context->restart();
                context->runInitScript();   // note: we assume sane init stack size here
                // setup pak root
                int ipr = context->findVariable("DAS_PAK_ROOT");
                if ( ipr != -1 ) {
                    // TODO: verify if its string
                    string pakRoot = ".";
                    auto np = pak.find_last_of("\\/");
                    if ( np != string::npos ) {
                        pakRoot = pak.substr(0,np+1);
                    }
                    // set it
                    char ** gpr = (char **) context->getVariable(ipr);
                    *gpr = context->stringHeap->allocateString(pakRoot);
                }
                // logs
                auto tostr = tout.str();
                if ( !tostr.empty() ) {
                    printf("%s\n", tostr.c_str());
                }
            }
        } else {
            DAS_FATAL_ERROR("failed to compile: %s\n%s", pak.c_str(), tout.str().c_str());
        }
    }

    int64_t FileAccess::getFileMtime ( const string & fileName) const {
#if !defined(DAS_NO_FILEIO)
        struct stat st;
        stat(fileName.c_str(), &st);
        return st.st_mtime;
#else
        return -1;
#endif
    }

    bool ModuleFileAccess::canModuleBeUnsafe ( const string & mod, const string & fileName ) const {
        if(failed() || !moduleUnsafe) return FileAccess::canModuleBeUnsafe(mod,fileName);
        vec4f args[2];
        args[0] = cast<const char *>::from(mod.c_str());
        args[1] = cast<const char *>::from(fileName.c_str());
        auto res = context->evalWithCatch(moduleUnsafe, args, nullptr);
        auto exc = context->getException(); exc;
        DAS_ASSERTF(!exc, "exception failed in `module_unsafe`: %s", exc);
        return cast<bool>::to(res);
    }

    bool ModuleFileAccess::isModuleAllowed ( const string & mod, const string & fileName ) const {
        if(failed() || !moduleAllowed) return FileAccess::isModuleAllowed(mod,fileName);
        vec4f args[2];
        args[0] = cast<const char *>::from(mod.c_str());
        args[1] = cast<const char *>::from(fileName.c_str());
        auto res = context->evalWithCatch(moduleAllowed, args, nullptr);
        auto exc = context->getException(); exc;
        DAS_ASSERTF(!exc, "exception failed in `module_allowed`: %s", exc);
        return cast<bool>::to(res);
    }

    ModuleInfo ModuleFileAccess::getModuleInfo ( const string & req, const string & from ) const {
        if (failed()) return FileAccess::getModuleInfo(req, from);
        vec4f args[2];
        args[0] = cast<const char *>::from(req.c_str());
        args[1] = cast<const char *>::from(from.c_str());
        struct {
            char * modName;
            char * modFileName;
            char * modImportName;
        } res;
        memset(&res, 0, sizeof(res));
        context->evalWithCatch(modGet, args, &res);
        auto exc = context->getException(); exc;
        DAS_ASSERTF(!exc, "exception failed in `module_get`: %s", exc);
        ModuleInfo info;
        info.moduleName = res.modName ? res.modName : "";
        info.fileName = res.modFileName ? res.modFileName : "";
        info.importName = res.modImportName ? res.modImportName : "";
        return info;
    }

    string ModuleFileAccess::getIncludeFileName ( const string & fileName, const string & incFileName ) const {
        if (failed() || !includeGet) return FileAccess::getIncludeFileName(fileName,incFileName);
        vec4f args[2];
        args[0] = cast<const char *>::from(incFileName.c_str());
        args[1] = cast<const char *>::from(fileName.c_str());
        vec4f res = context->evalWithCatch(includeGet, args, nullptr);
        auto exc = context->getException(); exc;
        DAS_ASSERTF(!exc, "exception failed in `include_get`: %s", exc);
        auto fname = cast<const char *>::to(res);
        return fname ? fname : "";
    }
}
