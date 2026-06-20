options gen2
module llvm_dll_utils shared private

require jit

require daslib/rtti
require daslib/strings_boost
require daslib/ast

// Wrapped name for dll.
struct public DllName {
    name : string

    def DllName(s : string) {
        name = s |> replace_multiple([(text = "@", replacement = "`At`"), (text = "=", replacement = "`Eq`"), (text = "<", replacement = "`Lt`"), (text = ">", replacement = "`Gt`"), (text = ",", replacement = "`C`")])
    }

    def const impl() {
        return "{name} implementation"
    }

    def const publ() {
        return "{name}"
    }

    def const glob() {
        return "{name} glob"
    }

    def const id() {
        return "{name} hash"
    }
}

// We need unique ids for dll names. Visitor generates this ids.
class UidGen : AstVisitor {
    uid : uint64
    id_map : table<void?, uint64>?
    @do_not_delete thisFunc : Function?

    def private add(ptr : void?) {
        *id_map |> insert(ptr, uid)
        uid++
    }

    [unused_argument(expr)] def override preVisitFunction(expr : FunctionPtr) {
        uid = 0 |> uint64()
    }

    def override preVisitExprAddr(expr : ExprAddr?) {
        add(expr)
    }

    def override preVisitExprNew(expr : ExprNew?) {
        add(expr)
        add(expr.typeexpr)
    }

    def override preVisitExprAscend(expr : ExprAscend?) {
        add(expr)
        add(expr.subexpr._type)
    }

    def override preVisitExprDelete(expr : ExprDelete?) {
        add(expr)
        add(expr.subexpr._type.firstType)
    }

    def override preVisitExprClone(expr : ExprClone?) {
        add(expr.left._type)
    }

    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
        add(expr)
    }

    def override preVisitExprBlock(expr : ExprBlock?) {
        add(expr)
    }

    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        add(expr)
    }

    def override preVisitExprCall(expr : ExprCall?) {
        add(expr)
    }

    def override preVisitExprOp2(var expr : ExprOp2?) {
        add(expr)
    }

    def override preVisitExprOp1(var expr : ExprOp1?) {
        add(expr)
    }

    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) {
        add(expr)
    }

    def override preVisitExprMakeBlock(expr : ExprMakeBlock?) {
        add(expr)
    }

}

class public UidNodes {
    id_map : table<void?, uint64>
    namesp : string
    @do_not_delete thisFunc : Function?

    def UidNodes(prog : Program?; var tab : table<void?, uint64>) {
        namesp = prog.thisNamespace |> string()
        id_map <- tab
    }

    def const get_namesp_prefix() {
        return namesp
    }

    def private get_base(ptr : void?; hint : string) {
        if (id_map |> key_exists(ptr)) {
            let id = unsafe(id_map[ptr])
            return DllName("{get_namesp_prefix()}`{get_mangled_name(thisFunc)}_{id}_{hint}")
        } else {
            panic("Key not found for {hint}")
            return DllName()
        }
    }

    def get_addr(expr : ExprAddr?) : DllName {
        return get_base(expr, "addr_{get_mangled_name(expr.func)}")
    }

    def get_new(expr : TypeDeclPtr) : DllName {
        return get_base(expr, "new`handle`{get_mangled_name(expr)}")
    }

    def get_ascend_new(expr : ExprAscend?) {
        return get_base(expr, "ascend_new`handle`{get_mangled_name(expr.subexpr._type)}")
    }

    def get_delete_type(t : TypeDeclPtr) : DllName {
        return get_base(t, "delete`handle`{get_mangled_name(t)}")
    }

    def get_clone_type(t : TypeDeclPtr) : DllName {
        return get_base(t, "clone`handle`{get_mangled_name(t)}")
    }

    def get_tinfo(expr : ExprTypeInfo?) {
        return get_base(expr, "tinfo_{expr.typeexpr.get_mnh}")
    }

    def get_ctor(expr : ExprMakeStruct?) {
        return get_base(expr, "ctor_{expr.constructor}")
    }

    def get_call(expr : ExprCallFunc?) : DllName {
        return get_base(expr, "call_{get_mangled_name(expr.func)}")
    }

    def public get_block_name_ptr(blk : ExprBlock?) : DllName {
        return get_base(blk, "block_{get_mangled_name(blk)}")
    }

    def public get_block_name(blk : ExprBlock?) : DllName {
        return get_base(blk, "block_{get_mangled_name(blk)}")
    }

    def public get_block_ann(mk_blk : ExprMakeBlock?) : DllName {
        return get_base(mk_blk, "makeblock_ann")
    }

    def reset(var func : Function?) {
        thisFunc = func
    }

    def const get_dll_fn_name_ptr(fn : Function?) : DllName {
        return DllName("{get_namesp_prefix()}`{fn |> get_mangled_name()}")
    }

    def const get_dll_fn_name(fn : FunctionPtr) : DllName {
        return DllName("{get_namesp_prefix()}`{fn |> get_mangled_name()}")
    }

}

def public create_uid_nodes(prog : Program?; funcs : array<FunctionPtr>) {
    var res = new table<void?, uint64>()
    var uids = new UidGen(id_map = res)
    make_visitor(*uids) $(adapter) {
        for (fun in funcs) {
            visit(fun, adapter)
        }
    }
    unsafe {
        delete uids
    }
    return new UidNodes(prog, *res)
}


// All dll operations pass through this class
class public DLLHandle {
    handle : void?
    created_fileinfos : table<FileInfo?; FileInfo?>

    def DLLHandle(handle_ : void?) {
        handle = handle_
    }

    def reset() {
        if (handle != null) {
            handle |> close_dynamic_library()
            handle = null
        }
    }

    def get_function_address(fn : DllName) : void? {
        assert(handle != null, "dynamic_lib_handle is null")
        return handle |> get_function_address(fn.publ());
    }

    def get_function_impl(fn : DllName) {
        assert(handle != null, "dynamic_lib_handle is null")
        return handle |> get_function_address(fn.impl());
    }

    def get_function_hash(fn : DllName) {
        return unsafe(reinterpret<uint64?>(handle |> get_function_address(fn.id())))
    }

    def set_glob_address(fn : DllName, ptr : void?) {
        assert(handle != null, "dynamic_lib_handle is null")
        var fn_addr = unsafe(reinterpret<void ??>(handle |> get_function_address(fn.glob())))
        // assert(fn_addr != null)
        if (fn_addr == null) return false
        *fn_addr = ptr;
        return true
    }


    def create_fileinfo(fi : FileInfo?) {
        if (!created_fileinfos |> key_exists(fi)) {
            let fi_ptr = unsafe(reinterpret<void?>(fi))
            var sym = get_function_address(DllName("fileinfo_at_{fi_ptr}"))
            var sym_name = get_function_address(DllName("fileinfo_at_{fi_ptr}_name"))
            let name = unsafe(reinterpret<string>(sym_name))

            let src_fi = unsafe(reinterpret<FileInfo?>(sym))
            recreate_fileinfo_name(src_fi, name)
            return src_fi
        } else {
            return unsafe(created_fileinfos[fi])
        }
    }

}

def public add_obj_extension(path : string) {
    return "{path}.o"
}
def public add_dll_extension(path : string) {
    return "{path}.dll"
}

def public add_exe_extension(path : string) {
    return "{path}.exe"
}

def public get_dll_by_path(path : string) : DLLHandle? {
    return new DLLHandle(handle = load_dynamic_library(add_dll_extension(path)))
}
