#include "daScript/misc/platform.h"

#include "dasStdDlg.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"

namespace das {

char * GetSaveFileDlg ( const char * initialFileName , const char * initialPath, const char * filter, Context * ctx, das::LineInfoArg * at ) {
    auto sf = GetSaveFileFromUser(
        initialFileName ? initialFileName : "",
        initialPath ? initialPath : "",
        filter ? filter : ""
    );
    return ctx->allocateString(sf, at);
}

char * GetOpenFileDlg ( const char * initialPath, const char * filter, Context * ctx, das::LineInfoArg * at ) {
    auto sf = GetOpenFileFromUser(
        initialPath ? initialPath : "",
        filter ? filter : ""
    );
    return ctx->allocateString(sf, at);
}

char * GetOpenFolderDlg ( const char * initialPath, Context * ctx, das::LineInfoArg * at ) {
    auto sf = GetOpenFolderFromUser(
        initialPath ? initialPath : ""
    );
    return ctx->allocateString(sf, at);
}

class Module_StdDlg : public Module {
public:
    Module_StdDlg() : Module("stddlg") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addExtern<DAS_BIND_FUN(StdDlgInit)> (*this, lib, "dlg_init",
            SideEffects::worstDefault, "StdDlgInit");
        addExtern<DAS_BIND_FUN(GetOkCancelFromUser)> (*this, lib, "get_dlg_ok_cancel_from_user",
            SideEffects::worstDefault, "GetOkCancelFromUser");
        addExtern<DAS_BIND_FUN(GetOkFromUser)> (*this, lib, "get_dlg_ok_from_user",
            SideEffects::worstDefault, "GetOkFromUser");
        addExtern<DAS_BIND_FUN(GetSaveFileDlg)> (*this, lib, "get_dlg_save_file",
            SideEffects::worstDefault, "GetSaveFileDlg");
        addExtern<DAS_BIND_FUN(GetOpenFileDlg)> (*this, lib, "get_dlg_open_file",
            SideEffects::worstDefault, "GetOpenFileDlg");
        addExtern<DAS_BIND_FUN(GetOpenFolderDlg)> (*this, lib, "get_dlg_open_folder",
            SideEffects::worstDefault, "GetOpenFolderDlg");
    }
    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        tw << "#include \"../modules/dasStdDlg/src/dasStdDlg.h\"\n";
        return ModuleAotType::cpp;
    }
};

}

REGISTER_MODULE_IN_NAMESPACE(Module_StdDlg,das);
