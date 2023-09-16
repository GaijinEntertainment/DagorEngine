options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module ast_used shared private

require daslib/ast_boost

struct public OnlyUsedTypes
    //! Collection of all structure and enumeration types that are used in the AST.
    st : table<Structure?;bool>
    en : table<Enumeration?;bool>

class TypeVisitor : AstVisitor
    usedTypes : OnlyUsedTypes
    def TypeVisitor
        pass
    def collect ( typ:TypeDeclPtr )
        unsafe
            if typ.baseType==Type tStructure
                if usedTypes.st |> key_exists(reinterpret<Structure?> typ.structType)
                    return
            if typ.structType != null
                usedTypes.st[reinterpret<Structure?> typ.structType] = true
                for fld in typ.structType.fields
                    self->collect ( fld._type )
            if typ.enumType != null
                usedTypes.en[reinterpret<Enumeration?> typ.enumType] = true
        if typ.firstType != null
            self->collect ( typ.firstType )
        if typ.secondType != null
            self->collect ( typ.secondType )
        for arg in typ.argTypes
            self->collect ( arg )
    def override preVisitTypeDecl(typ:TypeDeclPtr) : void
        self->collect(typ)

def public collect_used_types(vfun:array<Function?>; vvar:array<Variable?>; blk:block<(usedTypes:OnlyUsedTypes):void>)
    //! Goes through list of functions `vfun` and variables `vvar` and collects list of which enumeration and structure types are used in them.
    //! Calls `blk` with said list.
    var astVisitor = new TypeVisitor()
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    for f in vfun
        unsafe
            visit(reinterpret<FunctionPtr> f, astVisitorAdapter)
    for v in vvar
        astVisitor->collect(v._type)
    invoke(blk, astVisitor.usedTypes)
    unsafe
        delete astVisitor
