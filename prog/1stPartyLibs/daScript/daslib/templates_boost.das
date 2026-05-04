options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers
options unsafe_table_lookup = false

module templates_boost shared public

//! AST template substitution engine.
//!
//! Core infrastructure for macro-driven code generation. The `Template` structure
//! stores substitution rules (rename variables, replace types, swap expressions)
//! and `apply_template` walks an AST subtree applying them. Used internally by
//! many other daslib macros.
//! Requires the ``templates`` module.

require daslib/ast
require daslib/ast_boost
require daslib/algorithm
require daslib/strings_boost

struct Template {
    //! This structure contains collection of substitution rules for a template.
    kaboomVar : table<string; tuple<prefix : string; suffix : string> >         //! variable field access replacement rules
    call2name : table<string; string>                                           //! call name replacement rules
    field2name : table<string; string>                                          //! field name replacement rules
    var2name : table<string; string>                                            //! variable name replacement rules
    var2expr : table<string; smart_ptr<Expression>>                             //! variable expression replacement rules
    var2exprList : table<string; array<smart_ptr<Expression>>>                  //! variable expression list replacement rules
    type2type : table<string; string>                                           //! type name replacement rules
    type2etype : table<string; TypeDeclPtr>                                     //! type to type declaration replacement rules
    struct2etype : table<Structure?; TypeDeclPtr>                               //! structure to type declaration replacement rules
    blockArgName : table<string; string>                                        //! block argument name replacement rules
    annArg : table<string; lambda<(var ann : AnnotationDeclaration) : void> >   //! annotation argument replacement rules
    blkArg : table<string; array<VariablePtr>>                                  //! block argument replacement rules
    tag2expr : table<string; smart_ptr<Expression>>                             //! tag expression replacement rules
}

def kaboomVarField(var self : Template; name, prefix, suffix : string) {
    //! Adds a rule to to the template to replace a variable field access with a prefix and suffix.
    //! I.e. foo.bar into prefix + bar + suffix
    self.kaboomVar |> emplace(name,  (prefix, suffix))
}

def replaceVariable(var self : Template; name : string; var expr : smart_ptr<ast::Expression>) {
    //! Adds a rule to the template to replace a variable with an expression.
    self.var2expr |> emplace(name, expr)
}

def replaceVarTag(var self : Template; name : string; var expr : smart_ptr<ast::Expression>) {
    //! Adds a rule to the template to replace a variable tag with an expression.
    self.tag2expr |> emplace(name, expr)
}

def replaceArgumentWithList(var self : Template; name : string; blka : array<VariablePtr>) {
    //! Adds a rule to the template to replace a block argument with a list of variables.
    self.blkArg[name] := blka
}

def replaceVariableWithList(var self : Template; name : string; expr : array<ExpressionPtr>) {
    //! Adds a rule to the template to replace a variable with an expression list.
    self.var2exprList[name] := expr
}

def replaceVariableWithList(var self : Template; name : string; expr : dasvector`smart_ptr`Expression) {
    //! Adds a rule to the template to replace a variable with an expression list.
    var inscope vec : array<ExpressionPtr>
    for (e in expr) {
        vec |> push_clone(e)
    }
    swap(self.var2exprList[name], vec)
}

def renameVariable(var self : Template; name, newName : string) {
    //! Adds a rule to the template to rename a variable.
    self.var2name |> insert(name, newName)
}

def renameVariable(var self : Template; name : string; newName : das_string) {
    //! Adds a rule to the template to rename a variable.
    self.var2name |> insert(name, string(newName))
}

def renameField(var self : Template; name, newName : string) {
    //! Adds a rule to the template to rename any field lookup (., ?., as, is, etc)
    self.field2name |> insert(name, newName)
}

def renameField(var self : Template; name : string; newName : das_string) {
    //! Adds a rule to the template to rename any field lookup (., ?., as, is, etc)
    self.field2name |> insert(name, string(newName))
}

def replaceType(var self : Template; name, newName : string) {
    //! Adds a rule to the template to replace a type alias with another type alias, specified by name.
    self.type2type |> insert(name, newName)
}

def replaceStructWithTypeDecl(var self : Template; pstruct : Structure?; var expr : TypeDeclPtr) {
    //! Adds a rule to the template to replace a type alias with another type alias, specified by type declaration.
    self.struct2etype |> emplace(pstruct, expr)
}

def replaceTypeWithTypeDecl(var self : Template; name : string; var expr : TypeDeclPtr) {
    //! Adds a rule to the template to replace a type alias with another type alias, specified by type declaration.
    self.type2etype |> emplace(name, expr)
}

def replaceAnnotationArgument(var self : Template; name : string; var cb : lambda<(var ann : AnnotationDeclaration) : void>) {
    //! Adds a rule to the template to replace an annotation argument with the result of a callback.
    self.annArg |> emplace(name, cb)
}

def replaceBlockArgument(var self : Template; name, newName : string) {
    //! Adds a rule to the template to rename a block argument.
    self.blockArgName |> insert(name, newName)
}

def renameCall(var self : Template; name, newName : string) {
    //! Adds a rule to the template to rename a call.
    self.call2name |> insert(name, newName)
}

def renameCall(var self : Template; name : string; newName : das_string) {
    //! Adds a rule to the template to rename a call.
    self.call2name |> insert(name, string(newName))
}

class private TemplateVisitor : AstVisitor {
    //! AST visitor that applies template substitutions.
    @do_not_delete rules : Template?
    def TemplateVisitor(var r : Template?) {
        //! Creates template visitor with given rules.
        rules = r
    }
    def override visitExprTag(var expr : smart_ptr<ExprTag>) : ExpressionPtr {
        //! Visits tag expression and applies tag replacement rules.
        if (expr.name == "v") {
            if (expr.subexpr is ExprVar) {
                let tn = string((expr.subexpr as ExprVar).name)
                if (key_exists(rules.tag2expr, tn)) {
                    var inscope rexpr <- clone_expression(unsafe(rules.tag2expr[tn]))
                    rexpr.at = expr.at
                    return <- rexpr
                }
            }
        }
        return <- expr
    }
    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; var decl : MakeFieldDeclPtr; last : bool) : void {
        //! Visits structure field declaration and applies field renaming rules.
        let fname = string(decl.name)
        if (rules.field2name |> key_exists(fname)) {
            decl.name := rules.field2name |> get_value(fname)
        }
    }
    def override visitExprIsVariant(var expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        //! Visits is-variant expression and applies field renaming rules.
        let fname = string(expr.name)
        if (rules.field2name |> key_exists(fname)) {
            expr.name := rules.field2name |> get_value(fname)
        }
        return <- expr
    }
    def override visitExprAsVariant(var expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        //! Visits as-variant expression and applies field renaming rules.
        let fname = string(expr.name)
        if (rules.field2name |> key_exists(fname)) {
            expr.name := rules.field2name |> get_value(fname)
        }
        return <- expr
    }
    def override visitExprSafeAsVariant(var expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        //! Visits safe-as-variant expression and applies field renaming rules.
        let fname = string(expr.name)
        if (rules.field2name |> key_exists(fname)) {
            expr.name := rules.field2name |> get_value(fname)
        }
        return <- expr
    }
    def override visitExprSafeField(var expr : smart_ptr<ExprSafeField>) : ExpressionPtr {
        //! Visits safe-field expression and applies field renaming rules.
        let fname = string(expr.name)
        if (rules.field2name |> key_exists(fname)) {
            expr.name := rules.field2name |> get_value(fname)
        }
        return <- expr
    }
    def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr {
        //! Visits field expression and applies field renaming rules.
        let fname = string(expr.name)
        if (rules.field2name |> key_exists(fname)) {
            expr.name := rules.field2name |> get_value(fname)
        }
        var ev = expr.value ?as ExprVar
        if (ev == null) {
            ev = (expr.value ?as ExprRef2Value) ?as ExprVar
        }
        if (ev != null) {
            let kv = string(ev.name)
            if (rules.kaboomVar |> key_exists(kv)) {
                let ps = rules.kaboomVar[kv]
                return <- new ExprVar(at = expr.at, name := "{ps.prefix}{expr.name}{ps.suffix}")
            }
        }
        return <- expr
    }
    def override visitExprAddr(var expr : smart_ptr<ExprAddr>) : ExpressionPtr {
        //! Visits address-of expression and applies variable renaming rules.
        let vn = string(expr.target)
        if (key_exists(rules.var2name, vn)) {
            expr.target := rules.var2name |> get_value(vn)
        }
        return <- expr
    }
    def override visitExprLet(var expr : smart_ptr<ExprLet>) : ExpressionPtr {
        //! Visits let expression and applies variable renaming rules.
        for (v in expr.variables) {
            let vn = string(v.name)
            if (key_exists(rules.var2name, vn)) {
                v.name := rules.var2name |> get_value(vn)
            }
        }
        return <- expr
    }
    def override visitExprVar(var expr : smart_ptr<ExprVar>) : ExpressionPtr {
        //! Visits variable expression and applies variable renaming and replacement rules.
        let vn = string(expr.name)
        if (key_exists(rules.var2name, vn)) {
            expr.name := rules.var2name |> get_value(vn)
        }
        if (key_exists(rules.var2expr, vn)) {
            var inscope rexpr <- clone_expression(unsafe(rules.var2expr[vn]))
            rexpr.at = expr.at
            return <- rexpr
        }
        return <- expr
    }
    def replaceAlias(var typ : TypeDeclPtr&) {
        //! Replaces type aliases in the given type declaration according to the template rules.
        if (typ.baseType == Type.alias) {
            let ta = string(typ.alias)
            if (key_exists(rules.type2etype, ta)) {
                move_new(typ) <| clone_type(unsafe(rules.type2etype[ta]))
            } elif (key_exists(rules.type2type, ta)) {
                typ.alias := rules.type2type |> get_value(ta)
            }
        } elif (typ.baseType == Type.tStructure && typ.structType != null) {
            let st = typ.structType
            if (key_exists(rules.struct2etype, st)) {
                let oldFlags = typ.flags
                move_new(typ) <| clone_type(unsafe(rules.struct2etype[st]))
                typ.flags = oldFlags
            }
        }
        if (typ.firstType != null) {
            self->replaceAlias(typ.firstType)
        }
        if (typ.secondType != null) {
            self->replaceAlias(typ.secondType)
        }
        for (arg in typ.argTypes) {
            self->replaceAlias(arg)
        }
    }
    def override visitTypeDecl(var typ : TypeDeclPtr) : TypeDeclPtr {
        //! Visits type declaration and applies type replacement rules.
        var inscope tyyp <- clone_type(typ)
        replaceAlias(tyyp)
        return <- tyyp
    }
    def override preVisitExprFor(var expr : smart_ptr<ExprFor>) : void {
        //! Visits for-loop expression and applies variable renaming rules.
        for (it in expr.iterators) {
            let itn = string(it)
            if (key_exists(rules.var2name, itn)) {
                it := rules.var2name |> get_value(itn)
            }
        }
    }
    def override preVisitExprBlock(var blk : smart_ptr<ExprBlock>) {
        //! Visits block expression and applies block argument renaming and replacement rules, as well as annotation argument replacement rules.
        if (!blk.blockFlags.isClosure) {
            return
        }
        for (arg in blk.arguments) {
            let vn = "{arg.name}"
            if (key_exists(rules.blockArgName, vn)) {
                arg.name := rules.blockArgName |> get_value(vn)
            }
        }
        for (ann in blk.annotations) {
            rules.annArg |> get("{ann.annotation.name}") $(cb) {
                cb |> invoke(*ann)
            }
        }
        let nargs = blk.arguments |> length
        for (ai in range(nargs)) {
            let rai = nargs - ai - 1
            assume arg = blk.arguments[rai]
            let vname = string(arg.name)
            if (key_exists(rules.blkArg, vname)) {
                var inscope new_args : array<VariablePtr>
                for (a in rules.blkArg[vname]) {
                    new_args |> emplace_new <| new Variable(
                            at = arg.at,
                            name := a.name,
                            _type <- clone_type(a._type),
                            init <- clone_expression(a.init)
                        )
                }
                new_args |> reverse
                blk.arguments |> erase(rai)
                for (na in new_args) {
                    blk.arguments |> emplace(na, rai)
                }
            } elif (key_exists(rules.var2name, vname)) {
                arg.name := rules.var2name |> get_value(vname)
            }
        }
    }
    def preVisitAnythingCall(var arguments : dasvector`smart_ptr`Expression) : void {
        //! Visits call arguments and applies variable expression list replacement rules.
        if (length(rules.var2exprList) == 0) {
            return
        }
        let nargs = arguments |> length
        if (nargs == 0) {
            return
        }
        for (ai in range(nargs)) {
            let rai = nargs - ai - 1
            assume arg = arguments[rai]
            if (arg is ExprVar) {
                let vname = string((arg as ExprVar).name)
                if (key_exists(rules.var2exprList, vname)) {
                    var inscope new_args : array<ExpressionPtr>
                    for (a in rules.var2exprList[vname]) {
                        new_args |> emplace_new <| clone_expression(a)
                    }
                    new_args |> reverse
                    arguments |> erase(rai)
                    for (na in new_args) {
                        arguments |> emplace(na, rai)
                    }
                }
            }
        }
    }
    def override preVisitExprLooksLikeCall(var expr : smart_ptr<ExprLooksLikeCall>) : void {
        //! Visits looks-like-call expression and applies call renaming rules.
        let cname = string(expr.name)
        if (rules.call2name |> key_exists(cname)) {
            expr.name := rules.call2name |> get_value(cname)
        }
        preVisitAnythingCall(expr.arguments)
    }
    def override preVisitExprCall(var expr : smart_ptr<ExprCall>) : void {
        //! Visits call expression and applies call renaming rules.
        let cname = string(expr.name)
        if (rules.call2name |> key_exists(cname)) {
            expr.name := rules.call2name |> get_value(cname)
        }
        preVisitAnythingCall(expr.arguments)
    }
    def override preVisitExprAddr(var expr : smart_ptr<ExprAddr>) : void {
        //! Visits address-of expression and applies call renaming rules.
        let cname = string(expr.target)
        if (rules.call2name |> key_exists(cname)) {
            expr.target := rules.call2name |> get_value(cname)
        }
    }
    def override preVisitExprMakeArray(var expr : smart_ptr<ExprMakeArray>) : void {
        //! Visits make-array expression and applies call renaming rules.
        preVisitAnythingCall(expr.values)
    }
}

def visit_expression(var expr : ExpressionPtr&; var adapter : smart_ptr<VisitorAdapter>) {
    //! Visits the expression with the given visitor adapter.
    unsafe {
        // note: visit returns smart_ptr_raw, so we need to be careful with lifetime here
        var newExpr <- expr |> visit(adapter)
        if (expr != newExpr) {
            expr := null
        }
        expr <- newExpr
    }
}

def apply_template(var rules : Template; at : LineInfo; var expr : smart_ptr<Expression>; forceAt : bool = true) : ExpressionPtr {
    //! Applies the template to the given expression. If `forceAt` is set, the resulting expression will have the same line info as 'at'.
    var astVisitor = new TemplateVisitor(unsafe(addr(rules)))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit_expression(expr, astVisitorAdapter)
    if (forceAt) {
        expr |> force_at(at)
    }
    unsafe {
        delete astVisitor
    }
    return <- expr
}

def private visit_type(var typ : TypeDeclPtr&; var adapter : smart_ptr<VisitorAdapter>) {
    //! Visits the type declaration with the given visitor adapter.
    unsafe {
        // note: visit returns smart_ptr_raw, so we need to be careful with lifetime here
        var newType <- typ |> visit(adapter)
        if (typ != newType) {
            typ := null
        }
        typ <- newType
    }
}

def apply_template(var rules : Template; at : LineInfo; var typ : smart_ptr<TypeDecl>; forceAt : bool = true) : TypeDeclPtr {
    //! Applies the template to the given expression. If `forceAt` is set, the resulting expression will have the same line info as 'at'.
    var astVisitor = new TemplateVisitor(unsafe(addr(rules)))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit_type(typ, astVisitorAdapter)
    /*
    if forceAt
        expr |> force_at(at)
    */
    unsafe {
        delete astVisitor
    }
    return <- typ
}

def unquote_block(expr : ExpressionPtr) : smart_ptr<ExprBlock> {
    //! Returns the corresponding block subexpression expression from the ExprMakeBlock.
    assert(expr is ExprMakeBlock)
    unsafe {
        let mkb <- reinterpret<smart_ptr<ExprMakeBlock>>(expr)
        let blk := mkb._block
        return reinterpret<smart_ptr<ExprBlock>>(blk)
    }
}

def move_unquote_block(var expr : ExpressionPtr&) : smart_ptr<ExprBlock> {
    //! Moves the corresponding block subexpression expression from the ExprMakeBlock.
    assert(expr is ExprMakeBlock)
    unsafe {
        var mkb <- reinterpret<smart_ptr<ExprMakeBlock>>(expr)
        let blk := mkb._block
        var res <- reinterpret<smart_ptr<ExprBlock>>(blk)
        mkb := null
        return <- res
    }
}

def make_expression_block(var exprs : array<ExpressionPtr>) : smart_ptr<ExprBlock> {
    //! Create ExprBlock and move all expressions from expr to the list of the block.
    var inscope blk <- new ExprBlock(uninitialized)
    for (e in exprs) {
        blk.list |> emplace_new <| clone_to_move(e)
    }
    return <- blk
}

def make_expression_block(var exprs : dasvector`smart_ptr`Expression) : smart_ptr<ExprBlock> {
    //! Create ExprBlock and move all expressions from expr to the list of the block.
    var inscope blk <- new ExprBlock(uninitialized)
    for (e in exprs) {
        blk.list |> emplace_new <| clone_expression(e)
    }
    return <- blk
}

def private add_global_var_any(mod : Module?; vname : string; vat : LineInfo; vflag : TypeDeclFlags; var value : ExpressionPtr; priv : bool) : bool {
    //! Adds global variable to the module, given name, type flags and initial value.
    var inscope vvar <- new Variable(at = vat,
        name := vname,
        _type <- new TypeDecl(at = vat,
            baseType = Type.autoinfer,
            flags = vflag
        ),
        init <- value
    )
    vvar.flags |= VariableFlags.generated
    if (priv) {
        vvar.flags |= VariableFlags.private_variable
    }
    return add_variable(mod, vvar)
}

def add_global_var(mod : Module?; vname : string; vat : LineInfo; var value : ExpressionPtr) {
    //! Adds global variable to the module, given name and initial value.
    //! Global variables type is would be inferred from the initial value.
    //! `priv` specifies if the variable is private to the block.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags.removeConstant, value, false)
}

def add_global_var(mod : Module?; vname : string; var typ : TypeDeclPtr; vat : LineInfo; priv : bool; blk : block<(var v : VariablePtr) : void>) {
    //! Add global variable to the module, given name and type.
    //! `priv` specifies if the variable is private to the block.
    var inscope vvar <- new Variable(at = vat,
        name := vname,
        _type <- typ
    )
    vvar.flags |= VariableFlags.generated
    if (priv) {
        vvar.flags |= VariableFlags.private_variable
    }
    invoke(blk, vvar)
    return add_variable(mod, vvar)
}

def add_global_var(mod : Module?; vname : string; var typ : TypeDeclPtr; vat : LineInfo; priv : bool) {
    //! Add global variable to the module, given name and type.
    return add_global_var(mod, vname, typ, vat, priv) <| $ [unused_argument(v)] (var v) {
        pass
    }
}

def add_global_let(mod : Module?; vname : string; vat : LineInfo; var value : ExpressionPtr) {
    //! Add global variable to the module, given name and initial value.
    //! Variable type will be constant.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags.constant, value, false)
}

def add_global_private_var(mod : Module?; vname : string; vat : LineInfo; var value : ExpressionPtr) {
    //! Add global variable to the module, given name and initial value.
    //! It will be private.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags.removeConstant, value, true)
}

def add_global_private_let(mod : Module?; vname : string; vat : LineInfo; var value : ExpressionPtr) {
    //! Add global variable to the module, given name and initial value.
    //! It will be private, and type will be constant.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags.constant, value, true)
}

def make_unique_private_name(prefix : string; vat : LineInfo) {
    //! Generates unique private name for the variable, given prefix and line info.
    //!
    //! .. warning::
    //!
    //!    The assumption is that line info is unique for the context of the unique name generation.
    //!    If it is not, additional measures must be taken to ensure uniqueness of prefix.
    return "{prefix}_{vat.line}_{vat.column}"
}

def apply_template(at : LineInfo; var typ : smart_ptr<TypeDecl>&; blk : block<(var rules : Template) : void>) : TypeDeclPtr {
    //! Applies the template to the given type declaration. If `forceAt` is set, the resulting type declaration will have the same line info as 'at'.
    unsafe {
        // TODO: figure out why this is needed
        var rules : Template
        invoke(blk, rules)
        var res <- apply_template(rules, at, typ, true)
        delete rules
        return <- res
    }
}

def apply_template(at : LineInfo; var expr : smart_ptr<Expression>&; blk : block<(var rules : Template) : void>) : ExpressionPtr {
    //! Applies the template to the given expression. If `forceAt` is set, the resulting expression will have the same line info as 'at'.
    unsafe {
        // TODO: figure out why this is needed
        var rules : Template
        invoke(blk, rules)
        var res <- apply_template(rules, at, expr, true)
        delete rules
        return <- res
    }
}

def apply_template(var expr : smart_ptr<Expression>&; blk : block<(var rules : Template) : void>) : ExpressionPtr {
    //! Applies the template to the given expression.
    unsafe {
        // TODO: figure out why this is needed
        var rules : Template
        invoke(blk, rules)
        var res <- apply_template(rules, expr.at, expr, false)
        delete rules
        return <- res
    }
}

class private RemoveDerefVisitor : AstVisitor {
    //! Visitor that removes dereferences of a specific variable.
    vname : string //! Name of the variable to remove dereferences for.
    def RemoveDerefVisitor(n : string) {
        //! Creates visitor that removes dereferences of variable `n`.
        vname = n
    }
    def override visitExprRef2Value(var expr : smart_ptr<ExprRef2Value>) : ExpressionPtr {
        //! Visits reference-to-value expression and removes dereference if it matches the variable name.
        if (expr.subexpr is ExprVar) {
            let vvar = expr.subexpr as ExprVar
            if ("{vvar.name}" == vname) {
                return <- clone_expression(expr.subexpr)
            }
        }
        return <- expr
    }
}

def public remove_deref(varname : string; var expr : smart_ptr<Expression>) {
    //! Removes dereferences of the variable `varname` from the expression.
    //! This is typically used when replacing 'workhorse' variable with constant.
    var astVisitor = new RemoveDerefVisitor(varname)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    expr |> visit(astVisitorAdapter)
    unsafe {
        delete astVisitor
    }
}

def public add_array_ptr_ref(var a : array<smart_ptr<auto(TT)>>) : array<smart_ptr<TT>> {
    //! Implementation details for the reification. This adds any array to the rules.
    var q := a
    return <- q
}

def public add_type_ptr_ref(a : TypeDeclPtr; flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Clones a type declaration and applies given flags.
    // var q := a
    var inscope q <- clone_type(a)
    q.flags |= flags & (
            TypeDeclFlags.ref | TypeDeclFlags.removeRef
        |   TypeDeclFlags.temporary | TypeDeclFlags.removeTemporary
        |   TypeDeclFlags._implicit | TypeDeclFlags.explicitConst
        |   TypeDeclFlags.removeConstant | TypeDeclFlags.removeDim
        )
    return <- q
}

def public enum_class_type(st) {
    //! return underlying type for the enumeration
    if (st.baseType == Type.tInt8 || st.baseType == Type.tUInt8) {
        return Type.tEnumeration8
    } elif (st.baseType == Type.tInt16 || st.baseType == Type.tUInt16) {
        return Type.tEnumeration16
    } elif (st.baseType == Type.tInt64 || st.baseType == Type.tUInt64) {
        return Type.tEnumeration64
    } else {
        return Type.tEnumeration
    }
}

def public add_type_ptr_ref(var st : StructurePtr; flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Creates a type declaration from a structure smart pointer.
    return <- new TypeDecl(at = st.at, baseType = Type.tStructure, structType = get_ptr(st), flags = flags)
}

def public add_type_ptr_ref(var st : Structure?; flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Creates a type declaration from a structure raw pointer.
    return <- new TypeDecl(at = st.at, baseType = Type.tStructure, structType = st, flags = flags)
}

def public add_type_ptr_ref(var st : EnumerationPtr; flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Creates a type declaration from an enumeration smart pointer.
    return <- new TypeDecl(at = st.at, baseType = enum_class_type(st), enumType = get_ptr(st), flags = flags)
}

def public add_type_ptr_ref(var st : Enumeration?; flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Creates a type declaration from an enumeration raw pointer.
    return <- new TypeDecl(at = st.at, baseType = enum_class_type(st), enumType = st, flags = flags)
}

[unused_argument(anything), unused_argument(flags)]
def public add_type_ptr_ref(anything : auto(TT); flags : TypeDeclFlags) : TypeDeclPtr {
    //! Implementation details for the reification. Fallback overload that produces a compile-time error for unsupported types.
    concept_assert(false, "$t reification expected type `TypeDeclPtr`, `StructurePtr`, or `Structure?`, but received {typeinfo typename(type<TT>)}")
    var inscope decl : TypeDeclPtr
    return <- decl
}

def private generatedExpr(var expr : ExpressionPtr) : ExpressionPtr {
    //! Marks the expression as generated.
    expr.genFlags |= ExprGenFlags.generated
    return <- expr
}

def private generatedVariable(var svar : VariablePtr) : VariablePtr {
    //! Marks the variable as generated.
    svar.flags |= VariableFlags.generated
    return <- svar
}

class private QRulesVisitor : AstVisitor {
    //! Visitor that processes type tags and variable tags in a block, generating replacement rules.
    tag_index : int = 0                 //! Index for generating unique tag names.
    failed : bool = false               //! Flag indicating if processing failed.
    @do_not_delete rblock : ExprBlock?  //! Reference to the expression block where replacement rules are added.
    def QRulesVisitor(var rb : ExprBlock?) {
        //! Creates visitor with given expression block for replacement rules.
        rblock = rb
    }
    def visitTD(var typ : TypeDeclPtr&) : void {
        //! Visits type declaration and processes type tags.
        unsafe {
            var newTyp <- visitTypeDecl(typ)
            if (newTyp != typ) {
                typ := null
            }
            typ <- newTyp
        }
    }
    def override visitTypeDecl(var typ : TypeDeclPtr) : TypeDeclPtr {
        //! Visits type declaration and processes type tags.
        if ((typ.baseType == Type.alias) && (typ.flags.isTag)) {
            if (typ.firstType == null || typ.firstType.dimExpr |> length != 1) {
                compiling_program() |> macro_error(typ.at, "unsupported type tag")
                failed = true
                return <- typ
            }
            let new_tag_name = "temp`type`{tag_index++}`{typ.at.line}`{typ.at.column}"
            var inscope ntyp <- new TypeDecl(at = typ.at,
                baseType = Type.alias,
                alias := new_tag_name
            )
            ntyp.flags |= typ.flags & ~TypeDeclFlags.isTag
            rblock.list |> emplace_new <| new ExprCall(at = typ.at,
                name := "replaceTypeWithTypeDecl",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = typ.at, name := "rules"),
                    new ExprConstString(at = typ.at, value := new_tag_name) |> generatedExpr(),
                    new ExprCall(at = typ.at, name := "add_type_ptr_ref",
                        arguments := array<ExpressionPtr>(
                            clone_expression(typ.firstType.dimExpr[0]),
                            new ExprConstBitfield(at = typ.at, value := ntyp.flags)
                        )
                    )
                )
            )
            return <- ntyp
        }
        if (typ.firstType != null) {
            visitTD(typ.firstType)
        }
        if (typ.secondType != null) {
            visitTD(typ.secondType)
        }
        for (arg in typ.argTypes) {
            visitTD(arg)
        }
        return <- typ
    }
    def override preVisitExprBlockArgument(blk : smart_ptr<ExprBlock>; var arg : VariablePtr; lastArg : bool) : void {
        //! Visits block argument and processes variable tags.
        if (arg.flags.tag) {
            if (arg.source == null) {
                compiling_program() |> macro_error(arg.at, "argument '{arg.name}' of block must have tag source")
                failed = true
                return
            }
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new ExprCall(at = arg.at,
                name := "renameVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = arg.at, name := "rules"),
                    new ExprConstString(at = arg.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(arg.source) |> generatedExpr()
                ))
            arg.name := new_tag_name
            arg.flags ^= VariableFlags.tag
            arg.source := null
        } elif (arg._type.flags.isTag && arg._type.baseType == Type.none) {
            if (arg.init == null) {
                compiling_program() |> macro_error(arg.at, "argument '{arg.name}' of block must have initializer")
                failed = true
                return
            }
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new ExprCall(at = arg.at,
                name := "replaceArgumentWithList",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = arg.at, name := "rules"),
                    new ExprConstString(at = arg.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(arg.init) |> generatedExpr()
                ))
            arg.name := new_tag_name
        }
    }
    def override preVisitFunctionArgument(fun : FunctionPtr; var arg : VariablePtr; lastArg : bool) : void {
        //! Visits function argument and processes variable/argument-list tags (mirrors preVisitExprBlockArgument).
        if (arg.flags.tag) {
            if (arg.source == null) {
                compiling_program() |> macro_error(arg.at, "argument '{arg.name}' of function must have tag source")
                failed = true
                return
            }
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new ExprCall(at = arg.at,
                name := "renameVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = arg.at, name := "rules"),
                    new ExprConstString(at = arg.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(arg.source) |> generatedExpr()
                ))
            arg.name := new_tag_name
            arg.flags ^= VariableFlags.tag
            arg.source := null
        } elif (arg._type.flags.isTag && arg._type.baseType == Type.none) {
            if (arg.init == null) {
                compiling_program() |> macro_error(arg.at, "argument '{arg.name}' of function must have initializer")
                failed = true
                return
            }
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new ExprCall(at = arg.at,
                name := "replaceArgumentWithList",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = arg.at, name := "rules"),
                    new ExprConstString(at = arg.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(arg.init) |> generatedExpr()
                ))
            arg.name := new_tag_name
        }
    }
    def override preVisitExprFor(var expr : smart_ptr<ExprFor>) : void {
        //! Visits for-loop expression and processes iterator tags.
        for (itn, tag in expr.iterators, expr.iteratorsTags) {
            if (tag != null) {
                let new_tag_name = "temp`var`{tag_index++}`{expr.at.line}`{expr.at.column}"
                rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                    name := "renameVariable",
                    arguments := array<ExpressionPtr>(
                        new ExprVar(at = expr.at, name := "rules"),
                        new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                        clone_expression(tag) |> generatedExpr()
                    ))
                // rblock.list[length(rblock.list) - 1].flags |= VariableFlags generated
                itn := new_tag_name
                tag := null
            }
        }
    }
    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; var decl : MakeFieldDeclPtr; last : bool) : void {
        //! Visits structure field declaration and processes field tags.
        if (decl.tag != null) {
            let new_tag_name = "temp`make`field`{tag_index++}`{expr.at.line}`{expr.at.column}"
            decl.name := new_tag_name
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "renameField",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(decl.tag) |> generatedExpr()
                ))
        }
    }
    def override visitExprTag(var expr : smart_ptr<ExprTag>) : ExpressionPtr {
        //! Visits tag expression and processes variable/expression/argument/call tags.
        var new_tag_name = ""
        if (expr.name == "c") {
            new_tag_name = "temp`call`{tag_index++}`{expr.at.line}`{expr.at.column}"
        } elif (expr.name == "f") {
            new_tag_name = "temp`field`{tag_index++}`{expr.at.line}`{expr.at.column}"
        } else {
            new_tag_name = "temp`var`{tag_index++}`{expr.at.line}`{expr.at.column}"
        }
        // print("replacing {new_tag_name} <- {describe(expr.subexpr)}\n")
        // deppending on a tag now?
        if (expr.name == "v") {// value
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "replaceVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    new ExprCall(at = expr.at, name := "convert_to_expression",
                        arguments := array<ExpressionPtr>(clone_expression(expr.subexpr))
                    )
                ))
        } elif (expr.name == "" || expr.name == "e") {// expression
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "replaceVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    new ExprCall(at = expr.at, name := "add_ptr_ref",
                        arguments := array<ExpressionPtr>(clone_expression(expr.subexpr))
                    )
                ))
        } elif (expr.name == "a") {// arguments
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "replaceVariableWithList",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(expr.subexpr) |> generatedExpr()
                ))
        } elif (expr.name == "b") {// expression block
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "replaceVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    new ExprCall(at = expr.at, name := "make_expression_block", arguments := array<ExpressionPtr>(clone_expression(expr.subexpr)))
                ))
        } elif (expr.name == "i") {// identificator
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "renameVariable",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(expr.subexpr) |> generatedExpr()
                ))
            if (expr.value != null) {
                if (expr.value is ExprLet) {
                    (expr.value as ExprLet).variables[0].name := new_tag_name
                    return <- clone_expression(expr.value)
                } elif (expr.value is ExprAddr) {
                    (expr.value as ExprAddr).target := new_tag_name
                    return <- clone_expression(expr.value)
                } else {
                    compiling_program() |> macro_error(expr.at, "unexpected $i(name)({expr.value.__rtti})")
                    failed = true
                    return <- expr
                }
            }
        } elif (expr.name == "f") {// field
            rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                name := "renameField",
                arguments := array<ExpressionPtr>(
                    new ExprVar(at = expr.at, name := "rules"),
                    new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                    clone_expression(expr.subexpr) |> generatedExpr()
                ))
            var inscope fexpr <- clone_expression(expr.value)
            if (fexpr is ExprField) {
                (fexpr as ExprField).name := new_tag_name
            } elif (fexpr is ExprSafeField) {
                (fexpr as ExprSafeField).name := new_tag_name
            } elif (fexpr is ExprAsVariant) {
                (fexpr as ExprAsVariant).name := new_tag_name
            } elif (fexpr is ExprSafeAsVariant) {
                (fexpr as ExprSafeAsVariant).name := new_tag_name
            } elif (fexpr is ExprIsVariant) {
                (fexpr as ExprIsVariant).name := new_tag_name
            } else {
                compiling_program() |> macro_error(expr.at, "unexpected $f(name)({fexpr.__rtti})")
                failed = true
                return <- expr
            }
            return <- fexpr
        } elif (expr.name == "c") {// call
            if (expr.value == null) {
                compiling_program() |> macro_error(expr.at, "expecting $c(name)(...), got null")
                failed = true
            } elif (expr.value is ExprCall) {
                rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                    name := "renameCall",
                    arguments := array<ExpressionPtr>(
                        new ExprVar(at = expr.at, name := "rules"),
                        new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                        clone_expression(expr.subexpr) |> generatedExpr()
                    ))
                var inscope ccall <- clone_expression(expr.value)
                (ccall as ExprCall).name := new_tag_name
                return <- ccall
            } elif (expr.value is ExprAddr) {
                rblock.list |> emplace_new <| new ExprCall(at = expr.at,
                    name := "renameCall",
                    arguments := array<ExpressionPtr>(
                        new ExprVar(at = expr.at, name := "rules"),
                        new ExprConstString(at = expr.at, value := new_tag_name) |> generatedExpr(),
                        clone_expression(expr.subexpr) |> generatedExpr()
                    ))
                var inscope ccall <- clone_expression(expr.value)
                (ccall as ExprAddr).target := new_tag_name
                return <- ccall
            } else {
                compiling_program() |> macro_error(expr.at, "expecting $c(name)(...), got {expr.value.__rtti}")
                failed = true
            }
        } else {
            compiling_program() |> macro_error(expr.at, "unsupported ExprTag {expr.name}")
            failed = true
        }
        return <- new ExprVar(at = expr.at, name := new_tag_name)
    }
}

[macro_function]
def private apply_qrules(var expr : smart_ptr<Expression>&) : smart_ptr<ExprBlock> {
    //! Implementation details for the expression reification.
    var inscope eblk <- new ExprBlock(at = expr.at,
        returnType <- new TypeDecl(at = expr.at, baseType = Type.tVoid),
        arguments := array<VariablePtr>(
            new Variable(at = expr.at, name := "rules", _type <- typeinfo ast_typedecl(type<Template>)) |> generatedVariable()
        ),
        blockFlags = ExprBlockFlags.isClosure
    )
    var astVisitor = new QRulesVisitor(get_ptr(eblk))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit_expression(expr, astVisitorAdapter)
    if (astVisitor.failed) {
        eblk := null
    }
    unsafe {
        delete astVisitor
    }
    let annot_rules =  ("rules", RttiValue(tBool = true))
    eblk |> append_annotation("$", "unused_argument", [ annot_rules ])
    return <- eblk
}

[macro_function]
def private apply_function_qrules(var func : smart_ptr<Function>&) : smart_ptr<ExprBlock> {
    //! Implementation details for the expression reification.
    var inscope eblk <- new ExprBlock(at = func.at,
        returnType <- new TypeDecl(at = func.at, baseType = Type.tVoid),
        arguments := array<VariablePtr>(
            new Variable(at = func.at, name := "rules", _type <- typeinfo ast_typedecl(type<Template>)) |> generatedVariable()
        ),
        blockFlags = ExprBlockFlags.isClosure
    )
    var astVisitor = new QRulesVisitor(get_ptr(eblk))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(func, astVisitorAdapter)
    if (astVisitor.failed) {
        eblk := null
    }
    unsafe {
        delete astVisitor
    }
    let annot_rules =  ("rules", RttiValue(tBool = true))
    eblk |> append_annotation("$", "unused_argument", [ annot_rules ])
    return <- eblk
}

[macro_function]
def private apply_template_qrules(prog : ProgramPtr; var template_structure : Structure?) : smart_ptr<ExprBlock> {
    //! Implementation details for the expression reification.
    var inscope eblk <- new ExprBlock(at = template_structure.at,
        returnType <- new TypeDecl(at = template_structure.at, baseType = Type.tVoid),
        arguments := array<VariablePtr>(
            new Variable(at = template_structure.at, name := "rules", _type <- typeinfo ast_typedecl(type<Template>)) |> generatedVariable()
        ),
        blockFlags = ExprBlockFlags.isClosure
    )
    var astVisitor = new QRulesVisitor(get_ptr(eblk))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)

    var mod = template_structure._module
    unsafe {
        var ts = template_structure
        var vis_structure <- reinterpret<smart_ptr<Structure>>(ts)
        prog.visit_structure(vis_structure, astVisitorAdapter)
    }

    if (!astVisitor.failed) {
        for_each_function(mod, "", $(func){
            return if (astVisitor.failed)
            if (func.flags.isClassMethod) {
                if (func.classParent == template_structure) {
                    visit(func, astVisitorAdapter)
                }
            }
        })
    }

    if (astVisitor.failed) {
        eblk := null
    }
    unsafe {
        delete astVisitor
    }
    let annot_rules =  ("rules", RttiValue(tBool = true))
    eblk |> append_annotation("$", "unused_argument", [ annot_rules ])
    return <- eblk
}

def public apply_qmacro(var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : ExpressionPtr {
    //! Implementation details for the expression reification.
    return <- apply_template(expr, blk)
}

def public apply_qblock(var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : ExpressionPtr {
    //! Implementation details for the expression reification. This is a block reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    qblk.blockFlags ^= ExprBlockFlags.isClosure
    return <- qblk
}

def public apply_qblock_to_array(var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : array<ExpressionPtr> {
    //! Implementation details for the expression reification. This is a block reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    var inscope arr : array<ExpressionPtr>
    for (e in qblk.list) {
        arr |> emplace_new <| clone_expression(e)
    }
    return <- arr
}

def public apply_qblock_expr(var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : ExpressionPtr {
    //! Implementation details for the expression reification. This is a first line of the block as expression reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    assert(qblk.list |> length == 1)
    return <- clone_to_move(qblk.list[0])
}

def public apply_qtype(var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : TypeDeclPtr {
    //! Implementation details for the expression reification. This is a type declaration reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprTypeDecl)
    return <- clone_type((res as ExprTypeDecl).typeexpr)
}

def public expression_at(var expr : ExpressionPtr; at : LineInfo) : ExpressionPtr {
    //! Force expression location, then return it.
    expr |> force_at(at)
    return <- expr
}

class private AstQCallMacro : AstCallMacro {
    //! This macro implements expression reification
    apply_call = "apply_qmacro" //! Default apply call, can be overridden in subclasses
    macro_call = "qmacro"       //! Default macro call, can be overridden in subclasses
    def override canVisitArgument(call : smart_ptr<ExprCallMacro>; argIndex : int) : bool {
        //! Only first argument is visited
        return false    // nop, this one is just like quote
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visit the macro call and generate the reified expression.
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting 1 argument, {macro_call}(expression)")
            return <- default<ExpressionPtr>
        }
        var inscope qclone <- clone_expression(call.arguments[0])
        var inscope qblk <- apply_qrules(qclone)
        if (qblk == null) {
            return <- default<ExpressionPtr>
        }
        var inscope acall <- new ExprCall(at = call.at,
            name := apply_call,
            arguments := array<ExpressionPtr>(
                new ExprQuote(
                    at = call.at,
                    arguments := array<ExpressionPtr>(qclone)
                ),
                new ExprMakeBlock(_block <- qblk, at = call.at)
            )
        )
        return <- acall
    }
}

[call_macro(name="qmacro")]
class private QMacro : AstQCallMacro {
    //! This macro implements expression reification 'qmacro'
}

[call_macro(name="qmacro_block")]
class private QBlockMacro : AstQCallMacro {
    //! This macro implements expression block reification 'qmacro_block'
    override apply_call = "apply_qblock"
    override macro_call = "qmacro_block"
}

[call_macro(name="qmacro_block_to_array")]
class private QBlockToArrayMacro : AstQCallMacro {
    //! This macro implements expression block to array reification 'qmacro_block_to_array'
    override apply_call = "apply_qblock_to_array"
    override macro_call = "qmacro_block_to_array"
}

[call_macro(name="qmacro_expr")]
class private QBlockExprMacro : AstQCallMacro {
    //! This macro implements first line of the expression block reification 'qmacro_expr'
    override apply_call = "apply_qblock_expr"
    override macro_call = "qmacro_expr"
}

[call_macro(name="qmacro_type")]
class private QTypeMacro : AstQCallMacro {
    //! This macro implements type declaration reification 'qmacro_type'
    override apply_call = "apply_qtype"
    override macro_call = "qmacro_type"
}

def public apply_qmacro_function(fname : string; var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : FunctionPtr {
    //! Implementation details for reification. This is a function generation reification.
    var inscope func <- new Function(at = expr.at, atDecl = expr.at, name := fname, flags = FunctionFlags.generated)
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    for (arg in qblk.arguments) {
        func.arguments |> emplace_new <| clone_variable(arg)
    }
    func.result := qblk.returnType
    qblk.arguments |> clear
    qblk.blockFlags ^= ExprBlockFlags.isClosure
    move(func.body) <| qblk
    return <- func
}

def public apply_qmacro_method(fname : string; var parent : StructurePtr; var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : FunctionPtr {
    //! Implementation details for reification. This is a class method function generation reification.
    var inscope func <- apply_qmacro_function(fname, expr, blk)
    func.flags |= FunctionFlags.isClassMethod
    func.classParent = get_ptr(parent)
    return <- func
}

def public apply_qmacro_variable(vname : string; var expr : smart_ptr<Expression>; blk : block<(var rules : Template) : void>) : VariablePtr {
    //! Implementation details for reification. This is a variable generation reification.
    var inscope vvar <- new Variable(at = expr.at, name := vname, flags = VariableFlags.generated)
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprTypeDecl)
    move_new(vvar._type) <| clone_type((res as ExprTypeDecl).typeexpr)
    return <- vvar
}

class private AstQNamedMacro : AstCallMacro {
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    apply_call = "apply_qmacro_function"
    macro_call = "qmacro_function"
    def override canVisitArgument(call : smart_ptr<ExprCallMacro>; argIndex : int) : bool {
        //! Only visits the first argument (the name).
        return argIndex == 0
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visits the macro call and generates the reified named expression.
        if (call.arguments |> length != 2) {
            macro_error(prog, call.at, "expecting 2 arguments, {macro_call}(name,expression)")
            return <- default<ExpressionPtr>
        }
        if (call.arguments[0]._type == null || !(call.arguments[0]._type.isString)) {
            macro_error(prog, call.at, "expecting function name, {macro_call}(NAME,expression)")
            return <- default<ExpressionPtr>
        }
        var inscope qclone <- clone_expression(call.arguments[1])
        var inscope qblk <- apply_qrules(qclone)
        if (qblk == null) {
            return <- default<ExpressionPtr>
        }
        return <- new ExprCall(at = call.at,
            name := apply_call,
            arguments := array<ExpressionPtr>(
                clone_expression(call.arguments[0]),
                new ExprQuote(
                    at = call.at,
                    arguments := array<ExpressionPtr>(qclone)
                ),
                new ExprMakeBlock(_block <- qblk, at = call.at)
            )
        )
    }
}

[call_macro(name="qmacro_function")]
class private AstQFunctionMacro : AstQNamedMacro {
    //! This macro implements expression reification for functions.
}

[call_macro(name="qmacro_variable")]
class private AstQVariableMacro : AstQNamedMacro {
    //! This macro implements expression reification for variables.
    override apply_call = "apply_qmacro_variable"
    override macro_call = "qmacro_variable"
}

class private AstQNamedClassMacro : AstCallMacro {
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    apply_call = "apply_qmacro_method"
    macro_call = "qmacro_method"
    def override canVisitArgument(call : smart_ptr<ExprCallMacro>; argIndex : int) : bool {
        //! Only visits the first two arguments (name and class).
        return argIndex == 0 || argIndex == 1
    }
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visits the macro call and generates the reified named class expression.
        if (call.arguments |> length != 3) {
            macro_error(prog, call.at, "expecting 3 arguments, {macro_call}(name,class,expression)")
            return <- default<ExpressionPtr>
        }
        if (!(call.arguments[0]._type.isString)) {
            macro_error(prog, call.at, "expecting function name, {macro_call}(NAME,class,expression)")
            return <- default<ExpressionPtr>
        }
        var inscope qclone <- clone_expression(call.arguments[2])
        var inscope qblk <- apply_qrules(qclone)
        if (qblk == null) {
            return <- default<ExpressionPtr>
        }
        return <- new ExprCall(at = call.at,
            name := apply_call,
            arguments := array<ExpressionPtr>(
                clone_expression(call.arguments[0]),
                clone_expression(call.arguments[1]),
                new ExprQuote(
                    at = call.at,
                    arguments := array<ExpressionPtr>(qclone)
                ),
                new ExprMakeBlock(_block <- qblk, at = call.at)
            )
        )
    }
}

[call_macro(name="qmacro_method")]
class private AstQMethodMacro : AstQNamedClassMacro {
    //! This macro implements expression reification for class methods.
    override apply_call = "apply_qmacro_method"
}

def public apply_qmacro_template_class(instance_name : string; var template_type : smart_ptr<TypeDecl>; blk : block<(var rules : Template) : void>) : TypeDeclPtr {
    //! Implementation details for the expression reification. This is a template class instantiation reification.
    var mod = compiling_module()
    var template_structure = template_type.structType
    let template_name = string(template_structure.name)
    var inscope instance_structure <- clone_structure(template_structure)
    instance_structure.parent = template_structure.parent
    instance_structure.name := instance_name
    instance_structure.flags &= ~(StructureFlags.isTemplate)
    var inscope instance_type <- new TypeDecl(at = template_structure.at,
        baseType = Type.tStructure,
        structType = get_ptr(instance_structure)
    )
    // lets build renames for all methods in the module
    var method_renames : table<string, string>
    var template_module = template_structure._module
    for_each_function(template_module, "", $(func){
        if (func.classParent == template_structure) {
            // we rename methods
            let func_name = string(func.name)
            var method_name_parts = split(func_name, "`")
            if (method_name_parts[0] == template_name) {
                method_name_parts[0] = instance_name
            }
            if (length(method_name_parts) == 2) {
                if (method_name_parts[1] == template_name) { // ctor
                    method_name_parts[1] = instance_name
                } elif (method_name_parts[1] == "clone" || method_name_parts[1] == "finalize" || method_name_parts[1] == "each") {
                    method_name_parts.erase(0)
                }
            }
            let func_clone_name = join(method_name_parts, "`")
            method_renames.insert(func_name, func_clone_name)
        }
    })
    // lets build rule block
    var inscope rules : Template
    invoke(blk, rules)
    // Template -> Instance TypeDecl
    rules |> replaceTypeWithTypeDecl(template_name) <| clone_type(instance_type)
    rules |> replaceStructWithTypeDecl(template_structure) <| clone_type(instance_type)
    // Template`Template class ctor rename
    rules |> renameCall("{template_name}`{template_name}") <| "{instance_name}`{instance_name}"
    // method renames
    for (t_method, i_method in keys(method_renames), values(method_renames)) {
        rules |> renameVariable(t_method, i_method)
        rules |> renameVariable("_::{t_method}", "_::{i_method}")
    }
    // now, lets clone all methods in the module
    for_each_function(template_module, "", $(func){
        if (func.classParent == template_structure) {
            let func_clone_name = method_renames[string(func.name)]
            var inscope func_clone <- clone_function(func)
            func_clone.name := func_clone_name
            func_clone.moreFlags &= ~(MoreFunctionFlags.isTemplate)
            func_clone.classParent = get_ptr(instance_structure)
            func_clone.result |> move_new <| apply_template(rules, func.at, clone_type(func.result))
            // expand $a() argument lists in function parameters (blkArg rules)
            // preVisitFunctionArgument collects $a tags from function arguments.
            if (length(rules.blkArg) > 0) {
                let nfargs = func_clone.arguments |> length
                for (ai in range(nfargs)) {
                    let rai = nfargs - ai - 1
                    assume farg = func_clone.arguments[rai]
                    let vname = string(farg.name)
                    if (key_exists(rules.blkArg, vname)) {
                        var inscope new_args : array<VariablePtr>
                        for (a in rules.blkArg[vname]) {
                            new_args |> emplace_new <| new Variable(
                                at = farg.at,
                                name := a.name,
                                _type <- clone_type(a._type),
                                init <- clone_expression(a.init)
                            )
                        }
                        new_args |> reverse
                        func_clone.arguments |> erase(rai)
                        for (na in new_args) {
                            func_clone.arguments |> emplace(na, rai)
                        }
                    }
                }
                // sync body block arguments from expanded function arguments
                var body_block = func_clone.body as ExprBlock
                if (body_block != null) {
                    body_block.arguments |> clear
                    for (farg in func_clone.arguments) {
                        body_block.arguments |> emplace_new <| new Variable(
                            at = farg.at,
                            name := farg.name,
                            _type <- clone_type(farg._type),
                            init <- clone_expression(farg.init)
                        )
                    }
                }
            }
            for (arg in func_clone.arguments) {
                arg._type |> move_new <| apply_template(rules, arg.at, clone_type(arg._type))
            }
            func_clone.body |> move_new <| apply_template(rules, func.at, clone_expression(func.body), false)
            mod |> add_function(func_clone)
        }
    })
    // now we fix up fields
    for (fld in instance_structure.fields) {
        fld._type |> move_new <| apply_template(rules, fld.at, clone_type(fld._type))
        if (fld.init != null) {
            fld.init |> move_new <| apply_template(rules, fld.at, clone_expression(fld.init))
        }
    }
    mod |> add_structure(instance_structure)
    delete rules
    delete template_type
    return <- instance_type
}

def public apply_qmacro_template_function(func : FunctionPtr; blk : block<(var rules : Template) : void>) : FunctionPtr {
    //! Applies template rules to a function, cloning it with substituted types.
    var inscope rules : Template
    invoke(blk, rules)
    var inscope func_clone <- clone_function(func)
    func_clone.moreFlags &= ~(MoreFunctionFlags.isTemplate)
    func_clone.result |> move_new <| apply_template(rules, func.at, clone_type(func.result))
    for (arg in func_clone.arguments) {
        arg._type |> move_new <| apply_template(rules, arg.at, clone_type(arg._type))
    }
    return <- func_clone
}

[call_macro(name="qmacro_template_function")]
class private AstQNamedTemplateFunctionMacro : AstCallMacro {
    //! Call macro for quoting named template functions.
    apply_call = "apply_qmacro_template_function"
    macro_call = "qmacro_template_function"
   //! This macro implements expression reification for the named expressions (function, variable, etc.)
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visit the macro call and generate the reified expression.
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting {macro_call}(@@TEMPLATE_FUNCTION_NAME)")
            return <- default<ExpressionPtr>
        }
        if (!(call.arguments[0] is ExprAddr)) {
            macro_error(prog, call.at, "expecting @@TEMPLATE_FUNCTION_NAME")
            return <- default<ExpressionPtr>
        }
        var expr_func = call.arguments[0] as ExprAddr
        if (expr_func.func == null) {
            macro_error(prog, call.at, "expecting @@TEMPLATE_FUNCTION_NAME to refer to a function")
            return <- default<ExpressionPtr>
        }
        var inscope qfunc := unsafe(reinterpret<smart_ptr<Function>>(expr_func.func))
        var inscope qblk <- apply_function_qrules(qfunc)
        var mod_name = string(expr_func.func._module.name)
        if (mod_name.empty()) {
            mod_name = "_"
        }
        return <- new ExprCall(at = call.at,
            name := apply_call,
            arguments := array<ExpressionPtr>(
                new ExprCall(at = call.at,
                    name := "find_compiling_function_by_mangled_name_hash",
                    arguments := array<ExpressionPtr>(
                        new ExprConstString(at = call.at, value := mod_name)
                    ,   new ExprConstUInt64(at = call.at, value := expr_func.func.getMangledNameHash)
                    )
                ),
                new ExprMakeBlock(_block <- qblk, at = call.at)
            )
        )
    }
}

[call_macro(name="qmacro_template_class")]
class private AstQNamedTemplateClassMacro : AstCallMacro {
    //! Call macro for quoting named template class methods.
    apply_call = "apply_qmacro_template_class"
    macro_call = "qmacro_template_class"
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visit the macro call and generate the reified expression.
        if (call.arguments |> length != 2) {
            macro_error(prog, call.at, "expecting {macro_call}(NAME,type<TemplateClassOrStructType>)")
            return <- default<ExpressionPtr>
        }
        if (!(call.arguments[0]._type.isString)) {
            macro_error(prog, call.at, "expecting function name, {macro_call}(NAME,type<TemplateClassOrStructType>)")
            return <- default<ExpressionPtr>
        }
        if (call.arguments[1]._type == null) {
            macro_error(prog, call.at, "expecting type")
            return <- default<ExpressionPtr>
        }
        if (!(call.arguments[1]._type.baseType == Type.tStructure
            && call.arguments[1]._type.structType != null
            && call.arguments[1]._type.structType.flags.isTemplate)) {
            macro_error(prog, call.at, "expecting template structure or class type, {macro_call}(NAME,type<TemplateClassOrStructType>), got {describe(call.arguments[1]._type)} which is {call.arguments[1]._type.baseType}")
            return <- default<ExpressionPtr>
        }
        var inscope qclone <- clone_expression(call.arguments[1])
        var inscope qblk <- apply_template_qrules(prog, (qclone as ExprTypeDecl).typeexpr.structType)
        if (qblk == null) {
            return <- default<ExpressionPtr>
        }
        return <- new ExprCall(at = call.at,
            name := apply_call,
            arguments := array<ExpressionPtr>(
                clone_expression(call.arguments[0]),
                new ExprTypeInfo(
                    at = call.at,
                    trait := "ast_typedecl",
                    typeexpr <- clone_type(call.arguments[1]._type)
                ),
                new ExprMakeBlock(_block <- qblk, at = call.at)
            )
        )
    }
}

def public add_structure_field(var cls : StructurePtr; name : string; var t : TypeDeclPtr; var init : ExpressionPtr) {
    //! Adds a field to the structure.
    let fi = length(cls.fields)
    cls.fields |> resize(fi + 1)
    cls.fields[fi].name := name
    move(cls.fields[fi]._type) <| t
    move(cls.fields[fi].init) <| init
    return fi
}

def public make_class(name : string; mod : Module?) {
    //! Creates a class structure. Adds __rtti, __finalize fields.
    var inscope st <- new Structure(name := name, flags = bitfield(1))
    unsafe(builtin_ast_make_class_rtti(get_ptr(st)))
    var inscope virt_fin <- unsafe(builtin_ast_make_class_finalize(get_ptr(st)))
    add_function(mod, virt_fin)
    return <- st
}

def public make_class(name : string; var baseClass : StructurePtr; mod : Module?) {
    //! Creates a class structure derived from baseClass. Adds __rtti, __finalize fields.
    return <- make_class(name, get_ptr(baseClass), mod)
}

def public make_class(name : string; var baseClass : Structure?; mod : Module?) {
    //! Creates a class structure derived from baseClass. Adds __rtti, __finalize fields.
    if (mod == null) {
        panic("expecting module")
    }
    var inscope st <- clone_structure(baseClass)
    st.name := name
    st.parent = baseClass
    unsafe(builtin_ast_make_class_rtti(get_ptr(st)))
    var inscope virt_fin <- unsafe(builtin_ast_make_class_finalize(get_ptr(st)))
    add_function(mod, virt_fin)
    return <- st
}

def public make_class_constructor(cls : StructurePtr; ctor : FunctionPtr) {
    //! Adds a class constructor from a constructor function.
    if (cls == null) {
        panic("expecting class")
    }
    if (ctor == null) {
        panic("expecting constructor function")
    }
    return <- unsafe(builtin_ast_make_class_constructor(get_ptr(cls), get_ptr(ctor)))
}

def public modify_to_class_member(cls : StructurePtr; fun : FunctionPtr; isExplicit : bool; Constant : bool) {
    //! Modifies function to be a member of a particular class.
    if (cls == null) {
        panic("expecting class")
    }
    if (fun == null) {
        panic("expecting function")
    }
    unsafe(builtin_ast_modify_to_class_member(get_ptr(fun), get_ptr(cls), isExplicit, Constant))
}
