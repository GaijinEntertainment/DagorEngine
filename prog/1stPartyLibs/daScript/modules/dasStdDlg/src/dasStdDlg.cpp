#include "daScript/misc/platform.h"

#include "dasStdDlg.h"
#include "dasStdDlgTray.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/bind_enum.h"

DAS_BASE_BIND_ENUM(das::TrayEventKind, TrayEventKind, click, double_click, right_click, menu, notification)

MAKE_TYPE_FACTORY(TrayEvent, das::TrayEvent)

namespace das {

struct TrayEventAnnotation : ManagedStructureAnnotation<TrayEvent, false, false> {
    TrayEventAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation("TrayEvent", ml, "das::TrayEvent") {
        addField<DAS_BIND_MANAGED_FIELD(kind)>("kind", "kind");
        addField<DAS_BIND_MANAGED_FIELD(id)>("id", "id");
        addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
        addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
    }
};

char * GetSaveFileDlg ( const char * initialFileName , const char * initialPath, const char * filter, Context * ctx, LineInfoArg * at ) {
    auto sf = GetSaveFileFromUser(
        initialFileName ? initialFileName : "",
        initialPath ? initialPath : "",
        filter ? filter : ""
    );
    return ctx->allocateString(sf, at);
}

char * GetOpenFileDlg ( const char * initialPath, const char * filter, Context * ctx, LineInfoArg * at ) {
    auto sf = GetOpenFileFromUser(
        initialPath ? initialPath : "",
        filter ? filter : ""
    );
    return ctx->allocateString(sf, at);
}

class Module_StdDlg : public Module {
public:
    Module_StdDlg() : Module("stddlg") {
        ModuleLibrary lib;
        lib.addModule(this);
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
        addEnumeration(new EnumerationTrayEventKind());
        addAnnotation(new TrayEventAnnotation(lib));
        addExtern<DAS_BIND_FUN(TrayAvailable)> (*this, lib, "tray_available",
            SideEffects::accessExternal, "TrayAvailable");
        addExtern<DAS_BIND_FUN(TrayCreate)> (*this, lib, "tray_create",
            SideEffects::modifyExternal, "TrayCreate")
                ->args({"tooltip"});
        addExtern<DAS_BIND_FUN(TraySetIcon)> (*this, lib, "tray_set_icon",
            SideEffects::modifyExternal, "TraySetIcon")
                ->args({"rgba8", "width", "height", "context", "at"});
        addExtern<DAS_BIND_FUN(TraySetTooltip)> (*this, lib, "tray_set_tooltip",
            SideEffects::modifyExternal, "TraySetTooltip")
                ->args({"text"});
        addExtern<DAS_BIND_FUN(TrayMenuClear)> (*this, lib, "tray_menu_clear",
            SideEffects::modifyExternal, "TrayMenuClear");
        addExtern<DAS_BIND_FUN(TrayMenuAdd)> (*this, lib, "tray_menu_add",
            SideEffects::modifyExternal, "TrayMenuAdd")
                ->args({"id", "label", "enabled", "checked", "context", "at"});
        addExtern<DAS_BIND_FUN(TrayMenuAddSeparator)> (*this, lib, "tray_menu_add_separator",
            SideEffects::modifyExternal, "TrayMenuAddSeparator");
        addExtern<DAS_BIND_FUN(TrayMenuCommit)> (*this, lib, "tray_menu_commit",
            SideEffects::modifyExternal, "TrayMenuCommit");
        addExtern<DAS_BIND_FUN(TrayPoll)> (*this, lib, "tray_poll",
            SideEffects::invokeAndAccessExternal, "TrayPoll")
                ->args({"blk", "context", "at"});
        addExtern<DAS_BIND_FUN(TrayNotify)> (*this, lib, "tray_notify",
            SideEffects::modifyExternal, "TrayNotify")
                ->args({"title", "body"});
        addExtern<DAS_BIND_FUN(TrayDestroy)> (*this, lib, "tray_destroy",
            SideEffects::modifyExternal, "TrayDestroy");
    }
    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        tw << "#include \"../modules/dasStdDlg/src/dasStdDlg.h\"\n";
        tw << "#include \"../modules/dasStdDlg/src/dasStdDlgTray.h\"\n";
        return ModuleAotType::cpp;
    }
};

REGISTER_DYN_MODULE(Module_StdDlg,Module_StdDlg);

}

REGISTER_MODULE_IN_NAMESPACE(Module_StdDlg,das);

