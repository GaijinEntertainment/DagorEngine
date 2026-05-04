options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module ast_used shared private

//! Used-type collection visitor.
//!
//! Walks the AST and collects all structure and enumeration types
//! that are actually referenced, producing an ``OnlyUsedTypes`` set.
//! Used by code generators to emit only the types that are needed.

require daslib/ast_boost

struct public OnlyUsedTypes {
    //! Collection of all structure and enumeration types that are used in the AST.
    st : table<Structure?>   //! all structure types used
    en : table<Enumeration?>   //! all enumeration types used
}

class TypeVisitor : AstVisitor {
    //! AST visitor that collects referenced types and structures.
    usedTypes : OnlyUsedTypes //! collected used types
    def TypeVisitor {
        pass
    }
    def collect(typ : TypeDeclPtr) {
        if (typ.baseType == Type.tStructure) {
            if (usedTypes.st |> key_exists(typ.structType)) {
                return
            }
        }
        if (typ.structType != null) {
            insert(usedTypes.st, typ.structType)
            for (fld in typ.structType.fields) {
                collect(fld._type)
            }
        }
        if (typ.enumType != null) {
            insert(usedTypes.en, typ.enumType)
        }
        if (typ.firstType != null) {
            collect(typ.firstType)
        }
        if (typ.secondType != null) {
            collect(typ.secondType)
        }
        for (arg in typ.argTypes) {
            collect(arg)
        }
    }
    def override preVisitTypeDecl(typ : TypeDeclPtr) : void {
        collect(typ)
    }
}

def public collect_used_types(vfun : array<Function?>; vvar : array<Variable?>; blk : block<(usedTypes : OnlyUsedTypes) : void>) {
    //! Goes through list of functions `vfun` and variables `vvar` and collects list of which enumeration and structure types are used in them.
    //! Calls `blk` with said list.
    var astVisitor = new TypeVisitor()
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    for (f in vfun) {
        unsafe {
            visit(reinterpret<FunctionPtr> f, astVisitorAdapter)
        }
    }
    for (v in vvar) {
        astVisitor->collect(v._type)
    }
    invoke(blk, astVisitor.usedTypes)
    unsafe {
        delete astVisitor
    }
}
