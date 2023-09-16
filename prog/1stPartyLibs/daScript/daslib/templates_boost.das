options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module templates_boost shared public

require ast
require daslib/ast_boost
require daslib/algorithm

struct Template
    //! This structure contains collection of subsitution rules for a template.
    kaboomVar : table<string; tuple<prefix:string;suffix:string> >
    call2name : table<string; string>
    field2name : table<string; string>
    var2name : table<string; string>
    var2expr : table<string; smart_ptr<Expression>>
    var2exprList : table<string; array<smart_ptr<Expression>>>
    type2type : table<string; string>
    type2etype : table<string; TypeDeclPtr>
    blockArgName : table<string; string>
    annArg : table<string; lambda<(var ann: AnnotationDeclaration):void> >
    blkArg : table<string; array<VariablePtr>>
    tag2expr : table<string; smart_ptr<Expression>>

def kaboomVarField ( var self:Template; name,prefix,suffix:string )
    //! Adds a rule to to the template to replace a variable field access with a prefix and suffix.
    //! I.e. foo.bar into prefix + bar + suffix
    self.kaboomVar[name] = [[auto prefix,suffix]]

def replaceVariable ( var self:Template; name:string; var expr : smart_ptr<ast::Expression> )
    //! Adds a rule to the template to replace a variable with an expression.
    move(self.var2expr[name], expr)

def replaceVarTag ( var self:Template; name:string; var expr : smart_ptr<ast::Expression> )
    //! Adds a rule to the template to replace a variable tag with an expression.
    move(self.tag2expr[name], expr)

def replaceArgumentWithList ( var self:Template; name:string; blka:array<VariablePtr> )
    //! Adds a rule to the template to replace a block argument with a list of variables.
    self.blkArg[name] := blka

def replaceVariableWithList ( var self:Template; name:string; expr : array<ExpressionPtr> )
    //! Adds a rule to the template to replace a variable with an expression list.
    self.var2exprList[name] := expr

def replaceVariableWithList ( var self:Template; name:string; expr : dasvector`smart_ptr`Expression )
    //! Adds a rule to the template to replace a variable with an expression list.
    var inscope vec : array<ExpressionPtr>
    for e in expr
        vec |> push_clone(e)
    swap(self.var2exprList[name],vec)

def renameVariable ( var self:Template; name, newName:string )
    //! Adds a rule to the template to rename a variable.
    self.var2name[name] = newName

def renameVariable ( var self:Template; name:string; newName:das_string )
    //! Adds a rule to the template to rename a variable.
    self.var2name[name] = string(newName)

def renameField ( var self:Template; name, newName:string )
    //! Adds a rule to the template to rename any field lookup (., ?., as, is, etc)
    self.field2name[name] = newName

def renameField ( var self:Template; name:string; newName:das_string )
    //! Adds a rule to the template to rename any field lookup (., ?., as, is, etc)
    self.field2name[name] = string(newName)

def replaceType ( var self:Template; name,newName:string )
    //! Adds a rule to the template to replace a type alias with another type alias, specified by name.
    self.type2type[name] = newName

def replaceTypeWithTypeDecl ( var self:Template; name:string; var expr : TypeDeclPtr )
    //! Adds a rule to the template to replace a type alias with another type alias, specified by type declaration.
    move(self.type2etype[name], expr)

def replaceAnnotationArgument ( var self:Template; name:string; var cb:lambda<(var ann: AnnotationDeclaration):void> )
    //! Adds a rule to the template to replace an annotation argument with the result of a callback.
    self.annArg[name] <- cb

def replaceBlockArgument ( var self:Template; name,newName:string )
    //! Adds a rule to the template to rename a block argument.
    self.blockArgName[name] = newName

def renameCall ( var self:Template; name, newName:string )
    //! Adds a rule to the template to rename a call.
    self.call2name[name] = newName

def renameCall ( var self:Template; name:string; newName:das_string )
    //! Adds a rule to the template to rename a call.
    self.call2name[name] = string(newName)

class private TemplateVisitor : AstVisitor
    [[do_not_delete]] rules : Template?
    def TemplateVisitor ( r : Template? )
        rules = r
    def override visitExprTag ( var expr:smart_ptr<ExprTag> ) : ExpressionPtr
        if expr.name=="v"
            if expr.subexpr is ExprVar
                let tn = string((expr.subexpr as ExprVar).name)
                if key_exists(rules.tag2expr, tn)
                    var inscope rexpr <- clone_expression(rules.tag2expr[tn])
                    rexpr.at = expr.at
                    return <- rexpr
        return <- expr
    def override preVisitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;var decl:MakeFieldDeclPtr;last:bool) : void
        let fname = string(decl.name)
        if rules.field2name |> key_exists(fname)
            decl.name := rules.field2name[fname]
    def override visitExprIsVariant(var expr:smart_ptr<ExprIsVariant>) : ExpressionPtr
        let fname = string(expr.name)
        if rules.field2name |> key_exists(fname)
            expr.name := rules.field2name[fname]
        return <- expr
    def override visitExprAsVariant(var expr:smart_ptr<ExprAsVariant>) : ExpressionPtr
        let fname = string(expr.name)
        if rules.field2name |> key_exists(fname)
            expr.name := rules.field2name[fname]
        return <- expr
    def override visitExprSafeAsVariant(var expr:smart_ptr<ExprSafeAsVariant>) : ExpressionPtr
        let fname = string(expr.name)
        if rules.field2name |> key_exists(fname)
            expr.name := rules.field2name[fname]
        return <- expr
    def override visitExprSafeField(var expr:smart_ptr<ExprSafeField>) : ExpressionPtr
        let fname = string(expr.name)
        if rules.field2name |> key_exists(fname)
            expr.name := rules.field2name[fname]
        return <- expr
    def override visitExprField(var expr:smart_ptr<ExprField>) : ExpressionPtr
        let fname = string(expr.name)
        if rules.field2name |> key_exists(fname)
            expr.name := rules.field2name[fname]
        var ev = expr.value ?as ExprVar
        if ev == null
            ev = (expr.value ?as ExprRef2Value) ?as ExprVar
        if ev != null
            let kv = string(ev.name)
            if rules.kaboomVar |> key_exists(kv)
                let ps = rules.kaboomVar[kv]
                return <- new [[ExprVar() at=expr.at, name:="{ps.prefix}{expr.name}{ps.suffix}"]]
        return <- expr
    def override visitExprAddr( var expr:smart_ptr<ExprAddr> ) : ExpressionPtr
        let vn = string(expr.target)
        if key_exists(rules.var2name,vn)
            expr.target := rules.var2name[vn]
        return <- expr
    def override visitExprLet( var expr:smart_ptr<ExprLet> ) : ExpressionPtr
        for v in expr.variables
            let vn = string(v.name)
            if key_exists(rules.var2name,vn)
                v.name := rules.var2name[vn]
        return <- expr
    def override visitExprVar(var expr:smart_ptr<ExprVar>) : ExpressionPtr
        let vn = string(expr.name)
        if key_exists(rules.var2name,vn)
            expr.name := rules.var2name[vn]
        if key_exists(rules.var2expr,vn)
            var inscope rexpr <- clone_expression(rules.var2expr[vn])
            rexpr.at = expr.at
            return <- rexpr
        return <- expr
    def replaceAlias ( var typ:TypeDeclPtr& )
        if typ.baseType==Type alias
            let ta = string(typ.alias)
            if key_exists(rules.type2etype, ta)
                move_new(typ) <| clone_type(rules.type2etype[ta])
            elif key_exists(rules.type2type, ta)
                typ.alias := rules.type2type[ta]
        if typ.firstType != null
            self->replaceAlias(typ.firstType)
        if typ.secondType != null
            self->replaceAlias(typ.secondType)
        for arg in typ.argTypes
            self->replaceAlias(arg)
    def override visitTypeDecl(var typ:TypeDeclPtr) : TypeDeclPtr
        var inscope tyyp <- clone_type(typ)
        replaceAlias(tyyp)
        return <- tyyp
    def override preVisitExprFor ( var expr:smart_ptr<ExprFor> ) : void
        for it in expr.iterators
            let itn = string(it)
            if key_exists(rules.var2name,itn)
                it := rules.var2name[itn]
    def override preVisitExprBlock(var blk:smart_ptr<ExprBlock>)
        if !blk.blockFlags.isClosure
            return
        for arg in blk.arguments
            let vn = "{arg.name}"
            if key_exists(rules.blockArgName, vn)
                arg.name := rules.blockArgName[vn]
        for ann in blk.annotations
            rules.annArg |> get("{ann.annotation.name}") <| $(cb)
                cb |> invoke(*ann)
        let nargs = blk.arguments |> length
        for ai in range(nargs)
            let rai = nargs - ai - 1
            assume arg = blk.arguments[rai]
            let vname = string(arg.name)
            if key_exists(rules.blkArg, vname)
                var inscope new_args : array<VariablePtr>
                for a in rules.blkArg[vname]
                    new_args |> emplace_new <| new [[Variable()
                            at=arg.at,
                            name:=a.name,
                            _type <- clone_type(a._type),
                            init <- clone_expression(a.init)
                        ]]
                new_args |> reverse
                blk.arguments |> erase(rai)
                for na in new_args
                    blk.arguments |> emplace(na, rai)
            elif key_exists(rules.var2name, vname)
                arg.name := rules.var2name[vname]
    def preVisitAnythingCall(var arguments:dasvector`smart_ptr`Expression) : void
        if length(rules.var2exprList)==0
            return
        let nargs = arguments |> length
        if nargs == 0
            return
        for ai in range(nargs)
            let rai = nargs - ai - 1
            assume arg = arguments[rai]
            if arg is ExprVar
                let vname = string((arg as ExprVar).name)
                if key_exists(rules.var2exprList, vname)
                    var inscope new_args : array<ExpressionPtr>
                    for a in rules.var2exprList[vname]
                        new_args |> emplace_new <| clone_expression(a)
                    new_args |> reverse
                    arguments |> erase(rai)
                    for na in new_args
                        arguments |> emplace(na, rai)
    def override preVisitExprLooksLikeCall(var expr:smart_ptr<ExprLooksLikeCall>): void
        let cname = string(expr.name)
        if rules.call2name |> key_exists(cname)
            expr.name := rules.call2name[cname]
        preVisitAnythingCall(expr.arguments)
    def override preVisitExprCall(var expr:smart_ptr<ExprCall>): void
        let cname = string(expr.name)
        if rules.call2name |> key_exists(cname)
            expr.name := rules.call2name[cname]
        preVisitAnythingCall(expr.arguments)
    def override preVisitExprAddr(var expr:smart_ptr<ExprAddr>): void
        let cname = string(expr.target)
        if rules.call2name |> key_exists(cname)
            expr.target := rules.call2name[cname]
    def override preVisitExprMakeArray(var expr:smart_ptr<ExprMakeArray>): void
        preVisitAnythingCall(expr.values)

def private visit_expression ( var expr:ExpressionPtr&; var adapter:smart_ptr<VisitorAdapter> )
    unsafe
        // note: visit returns smart_ptr_raw, so we need to be careful with lifetime here
        var newExpr <- expr |> visit(adapter)
        if expr != newExpr
            expr := null
        expr <- newExpr

def apply_template ( var rules:Template; at:LineInfo; var expr:smart_ptr<Expression>; forceAt:bool = true ) : ExpressionPtr
    //! Applies the template to the given expression. If `forceAt` is set, the resulting expression will have the same line info as 'at'.
    var astVisitor = new TemplateVisitor(unsafe(addr(rules)))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit_expression(expr, astVisitorAdapter)
    if forceAt
        expr |> force_at(at)
    unsafe
        delete astVisitor
    return <- expr

def unquote_block ( expr:ExpressionPtr ) : smart_ptr<ExprBlock>
    //! Returns the corresponding block subexpression expression form the ExprMakeBlock.
    assert(expr is ExprMakeBlock)
    unsafe
        let mkb <- reinterpret<smart_ptr<ExprMakeBlock>>(expr)
        let blk := mkb._block
        return reinterpret<smart_ptr<ExprBlock>>(blk)

def move_unquote_block ( var expr:ExpressionPtr& ) : smart_ptr<ExprBlock>
    //! Moves the corresponding block subexpression expression form the ExprMakeBlock.
    assert(expr is ExprMakeBlock)
    unsafe
        var mkb <- reinterpret<smart_ptr<ExprMakeBlock>>(expr)
        let blk := mkb._block
        var res <- reinterpret<smart_ptr<ExprBlock>>(blk)
        mkb := null
        return <- res

def make_expression_block ( var exprs:array<ExpressionPtr> ) : smart_ptr<ExprBlock>
    //! Create ExprBlock and move all expressions from expr to the list of the block.
    var inscope blk <- new [[ExprBlock()]]
    for e in exprs
        blk.list |> emplace_new <| clone_to_move(e)
    return <- blk

def make_expression_block ( var exprs:dasvector`smart_ptr`Expression ) : smart_ptr<ExprBlock>
    //! Create ExprBlock and move all expressions from expr to the list of the block.
    var inscope blk <- new [[ExprBlock()]]
    for e in exprs
        blk.list |> emplace_new <| clone_expression(e)
    return <- blk

def private add_global_var_any ( mod:Module?; vname:string; vat:LineInfo; vflag:TypeDeclFlags; var value:ExpressionPtr; priv:bool )
    var inscope vvar <- new [[Variable() at=vat,
        name:=vname,
        _type <- new [[TypeDecl() at=vat,
            baseType = Type autoinfer,
            flags = vflag
        ]],
        init <- value
    ]]
    vvar.flags |= VariableFlags generated
    if priv
        vvar.flags |= VariableFlags private_variable
    return <- add_variable(mod, vvar)

def add_global_var ( mod:Module?; vname:string; vat:LineInfo; var value:ExpressionPtr )
    //! Adds global variable to the module, given name and initial value.
    //! Global variables type is would be inferred from the initial value.
    //! `priv` specifies if the variable is private to the block.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags removeConstant, value, false)

def add_global_var ( mod:Module?; vname:string; var typ:TypeDeclPtr; vat:LineInfo; priv:bool; blk:block<(var v : VariablePtr):void> )
    var inscope vvar <- new [[Variable() at=vat,
        name:=vname,
        _type <- typ
    ]]
    vvar.flags |= VariableFlags generated
    if priv
        vvar.flags |= VariableFlags private_variable
    invoke(blk, vvar)
    return add_variable(mod, vvar)

def add_global_var ( mod:Module?; vname:string; var typ:TypeDeclPtr; vat:LineInfo; priv:bool )
    return add_global_var(mod,vname,typ,vat,priv) <| $ [unused_argument(v)]( var v )
        pass

def add_global_let ( mod:Module?; vname:string; vat:LineInfo; var value:ExpressionPtr )
    //! Add global variable to the module, given name and initial value.
    //! Variable type will be constant.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags constant, value, false)

def add_global_private_var ( mod:Module?; vname:string; vat:LineInfo; var value:ExpressionPtr )
    //! Add global variable to the module, given name and initial value.
    //! It will be private.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags removeConstant, value, true)

def add_global_private_let ( mod:Module?; vname:string; vat:LineInfo; var value:ExpressionPtr )
    //! Add global variable to the module, given name and initial value.
    //! It will be private, and type will be constant.
    return add_global_var_any(mod, vname, vat, TypeDeclFlags constant, value, true)

def make_unique_private_name ( prefix:string; vat:LineInfo )
    //! Generates unique private name for the variable, given prefix and line info.
    //!
    //! .. warning::
    //! The assumption is that line info is unique for the context of the unique name generation.
    //! If it is not, additional measures must be taken to ensure uniqueness of prefix.
    return "{prefix}_{vat.line}_{vat.column}"

def apply_template ( at:LineInfo; var expr:smart_ptr<Expression>&; blk:block<(var rules:Template):void> ) : ExpressionPtr
    unsafe
        // TODO: figure out why this is needed
        var rules : Template
        invoke(blk, rules)
        var res <- apply_template(rules, at, expr, true)
        delete rules
        return <- res

def apply_template ( var expr:smart_ptr<Expression>&; blk:block<(var rules:Template):void> ) : ExpressionPtr
    unsafe
        // TODO: figure out why this is needed
        var rules : Template
        invoke(blk, rules)
        var res <- apply_template(rules,expr.at, expr, false)
        delete rules
        return <- res

class private RemoveDerefVisitor : AstVisitor
    vname : string
    def RemoveDerefVisitor ( n : string )
        vname = n
    def override visitExprRef2Value(var expr:smart_ptr<ExprRef2Value>) : ExpressionPtr
        if expr.subexpr is ExprVar
            let vvar = expr.subexpr as ExprVar
            if "{vvar.name}"==vname
                return <- clone_expression(expr.subexpr)
        return <- expr

def public remove_deref ( varname : string; var expr:smart_ptr<Expression> )
    //! Removes dereferences of the variable `varname` from the expression.
    //! This is typically used when replacing 'workhorse' variable with constant.
    var astVisitor = new RemoveDerefVisitor(varname)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    expr |> visit(astVisitorAdapter)
    unsafe
        delete astVisitor

def public add_array_ptr_ref ( var a : array<smart_ptr<auto(TT)>> ) : array<smart_ptr<TT>>
    //! Implementation details for the reification. This adds any array to the rules.
    var q := a
    return <- q

def public add_type_ptr_ref ( var a : TypeDeclPtr; flags:TypeDeclFlags ) : TypeDeclPtr
    //! Implementation details for the reification. This adds any array to the rules.
    // var q := a
    var inscope q <- clone_type(a)
    q.flags |= flags & (
            TypeDeclFlags ref | TypeDeclFlags removeRef
        |   TypeDeclFlags temporary | TypeDeclFlags removeTemporary
        |   TypeDeclFlags _implicit | TypeDeclFlags explicitConst
        |   TypeDeclFlags removeConstant | TypeDeclFlags removeDim
        )
    return <- q

def public add_type_ptr_ref ( var st : StructurePtr; flags:TypeDeclFlags ) : TypeDeclPtr
    //! Implementation details for the reification. This adds any array to the rules.
    return <- new [[TypeDecl() at=st.at, baseType=Type tStructure, structType = get_ptr(st), flags=flags]]

def private generatedExpr( var expr : ExpressionPtr ) : ExpressionPtr
    expr.genFlags |= ExprGenFlags generated
    return <- expr

def private generatedVariable( var svar : VariablePtr ) : VariablePtr
    svar.flags |= VariableFlags generated
    return <- svar

class private QRulesVisitor : AstVisitor
    tag_index : int = 0
    failed : bool = false
    [[do_not_delete]] rblock : ExprBlock?
    def QRulesVisitor ( rb:ExprBlock? )
        rblock = rb
    def visitTD ( var typ:TypeDeclPtr& ) : void
        unsafe
            var newTyp <- visitTypeDecl(typ)
            if newTyp != typ
                typ := null
            typ <- newTyp
    def override visitTypeDecl(var typ:TypeDeclPtr) : TypeDeclPtr
        if (typ.baseType == Type alias) && (typ.flags.isTag)
            if typ.firstType==null || typ.firstType.dimExpr |> length!=1
                compiling_program() |> macro_error(typ.at,"unsupported type tag")
                failed = true
                return <- typ
            let new_tag_name = "temp`type`{tag_index++}`{typ.at.line}`{typ.at.column}"
            var inscope ntyp <- new [[TypeDecl() at=typ.at,
                baseType = Type alias,
                alias := new_tag_name
            ]]
            ntyp.flags |= typ.flags & ~TypeDeclFlags isTag
            rblock.list |> emplace_new <| new [[ExprCall() at=typ.at,
                name:="replaceTypeWithTypeDecl",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=typ.at, name:="rules"]];
                    new [[ExprConstString() at=typ.at, value:=new_tag_name]] |> generatedExpr();
                    new [[ExprCall() at=typ.at, name:="add_type_ptr_ref",
                        arguments := [{ExpressionPtr
                            clone_expression(typ.firstType.dimExpr[0]);
                            new [[ExprConstBitfield() at=typ.at, value:=ntyp.flags]]
                        }]
                    ]]
                }]
            ]]
            return <- ntyp
        if typ.firstType != null
            visitTD(typ.firstType)
        if typ.secondType != null
            visitTD(typ.secondType)
        for arg in typ.argTypes
            visitTD(arg)
        return <- typ
    def override preVisitExprBlockArgument(blk:smart_ptr<ExprBlock>;var arg:VariablePtr;lastArg:bool): void
        if arg.flags.tag
            if arg.source==null
                compiling_program() |> macro_error(arg.at,"argument '{arg.name}' of block must have tag source")
                failed = true
                return
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new [[ExprCall() at=arg.at,
                name:="renameVariable",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=arg.at, name:="rules"]];
                    new [[ExprConstString() at=arg.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(arg.source) |> generatedExpr()
                }]]]
            arg.name := new_tag_name
            arg.flags ^= VariableFlags tag
            arg.source := null
        elif arg._type.flags.isTag && arg._type.baseType==Type none
            if arg.init==null
                compiling_program() |> macro_error(arg.at,"argument '{arg.name}' of block must have initializer")
                failed = true
                return
            let new_tag_name = "temp`var`{tag_index++}`{arg.at.line}`{arg.at.column}"
            rblock.list |> emplace_new <| new [[ExprCall() at=arg.at,
                name:="replaceArgumentWithList",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=arg.at, name:="rules"]];
                    new [[ExprConstString() at=arg.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(arg.init) |> generatedExpr()
                }]]]
            arg.name := new_tag_name
    def override preVisitExprFor ( var expr : smart_ptr<ExprFor> ) : void
        for itn,tag in expr.iterators,expr.iteratorsTags
            if tag != null
                let new_tag_name = "temp`var`{tag_index++}`{expr.at.line}`{expr.at.column}"
                rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                    name:="renameVariable",
                    arguments:=[{ExpressionPtr
                        new [[ExprVar() at=expr.at, name:="rules"]];
                        new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                        clone_expression(tag) |> generatedExpr()
                    }]]]
                // rblock.list[length(rblock.list) - 1].flags |= VariableFlags generated
                itn := new_tag_name
                tag := null
    def override preVisitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;var decl:MakeFieldDeclPtr;last:bool) : void
        if decl.tag != null
            let new_tag_name = "temp`make`field`{tag_index++}`{expr.at.line}`{expr.at.column}"
            decl.name := new_tag_name
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="renameField",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(decl.tag) |> generatedExpr()
                }]]]
    def override visitExprTag ( var expr:smart_ptr<ExprTag> ) : ExpressionPtr
        var new_tag_name = ""
        if expr.name=="c"
            new_tag_name = "temp`call`{tag_index++}`{expr.at.line}`{expr.at.column}"
        elif expr.name=="f"
            new_tag_name = "temp`field`{tag_index++}`{expr.at.line}`{expr.at.column}"
        else
            new_tag_name = "temp`var`{tag_index++}`{expr.at.line}`{expr.at.column}"
        // print("replacing {new_tag_name} <- {describe(expr.subexpr)}\n")
        // deppending on a tag now?
        if expr.name=="v"   // value
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="replaceVariable",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    new [[ExprCall() at=expr.at, name:="convert_to_expression",
                        arguments := [{ExpressionPtr[1] clone_expression(expr.subexpr) }]
                    ]]
                }]]]
        elif expr.name=="" || expr.name=="e"    // expression
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="replaceVariable",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    new [[ExprCall() at=expr.at, name:="add_ptr_ref",
                        arguments := [{ExpressionPtr[1] clone_expression(expr.subexpr) }]
                    ]]
                }]]]
        elif expr.name=="a"    // arguments
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="replaceVariableWithList",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(expr.subexpr) |> generatedExpr()
                }]]]
        elif expr.name=="b" // expression block
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="replaceVariable",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    new [[ExprCall() at=expr.at,name:="make_expression_block", arguments := [{ExpressionPtr[1] clone_expression(expr.subexpr) }] ]]
                }]]]
        elif expr.name=="i" // identificator
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="renameVariable",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(expr.subexpr) |> generatedExpr()
                }]]]
            if expr.value != null
                if expr.value is ExprLet
                    (expr.value as ExprLet).variables[0].name := new_tag_name
                    return <- clone_expression(expr.value)
                elif expr.value is ExprAddr
                    (expr.value as ExprAddr).target := new_tag_name
                    return <- clone_expression(expr.value)
                else
                    compiling_program() |> macro_error(expr.at,"unexpected $i(name)({expr.value.__rtti})")
                    failed = true
                    return <- expr
        elif expr.name=="f" // field
            rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                name:="renameField",
                arguments:=[{ExpressionPtr
                    new [[ExprVar() at=expr.at, name:="rules"]];
                    new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                    clone_expression(expr.subexpr) |> generatedExpr()
                }]]]
            var inscope fexpr <- clone_expression(expr.value)
            if fexpr is ExprField
                (fexpr as ExprField).name := new_tag_name
            elif fexpr is ExprSafeField
                (fexpr as ExprSafeField).name := new_tag_name
            elif fexpr is ExprAsVariant
                (fexpr as ExprAsVariant).name := new_tag_name
            elif fexpr is ExprSafeAsVariant
                (fexpr as ExprSafeAsVariant).name := new_tag_name
            elif fexpr is ExprIsVariant
                (fexpr as ExprIsVariant).name := new_tag_name
            else
                compiling_program() |> macro_error(expr.at,"unexpected $f(name)({fexpr.__rtti})")
                failed = true
                return <- expr
            return <- fexpr
        elif expr.name=="c" // call
            if expr.value == null
                compiling_program() |> macro_error(expr.at,"expecting $c(name)(...), got null")
                failed = true
            elif expr.value is ExprCall
                rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                    name:="renameCall",
                    arguments:=[{ExpressionPtr
                        new [[ExprVar() at=expr.at, name:="rules"]];
                        new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                        clone_expression(expr.subexpr) |> generatedExpr()
                    }]]]
                var inscope ccall <- clone_expression(expr.value)
                (ccall as ExprCall).name := new_tag_name
                return <- ccall
            elif expr.value is ExprAddr
                rblock.list |> emplace_new <| new [[ExprCall() at=expr.at,
                    name:="renameCall",
                    arguments:=[{ExpressionPtr
                        new [[ExprVar() at=expr.at, name:="rules"]];
                        new [[ExprConstString() at=expr.at, value:=new_tag_name]] |> generatedExpr();
                        clone_expression(expr.subexpr) |> generatedExpr()
                    }]]]
                var inscope ccall <- clone_expression(expr.value)
                (ccall as ExprAddr).target := new_tag_name
                return <- ccall
            else
                compiling_program() |> macro_error(expr.at,"expecting $c(name)(...), got {expr.value.__rtti}")
                failed = true
        else
            compiling_program() |> macro_error(expr.at,"unsupported ExprTag {expr.name}")
            failed = true
        return <- new [[ExprVar() at=expr.at, name:=new_tag_name]]

[macro_function]
def private apply_qrules ( var expr:smart_ptr<Expression>& ) : smart_ptr<ExprBlock>
    var inscope eblk <- new [[ExprBlock() at=expr.at,
        returnType <- new [[TypeDecl() at=expr.at, baseType=Type tVoid]],
        arguments := [{VariablePtr[1]
            new [[Variable() at=expr.at, name := "rules", _type <- typeinfo(ast_typedecl type<Template>) ]] |> generatedVariable()
        }],
        blockFlags = ExprBlockFlags isClosure
    ]]
    var astVisitor = new QRulesVisitor(get_ptr(eblk))
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit_expression(expr, astVisitorAdapter)
    if astVisitor.failed
        eblk := null
    unsafe
        delete astVisitor
    return <- eblk

def public apply_qmacro ( var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : ExpressionPtr
    //! Implementation details for the expression reificaiton. This is a generat expression reification.
    return <- apply_template(expr, blk)

def public apply_qblock ( var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : ExpressionPtr
    //! Implementation details for the expression reificaiton. This is a block reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    qblk.blockFlags ^= ExprBlockFlags isClosure
    return <- qblk

def public apply_qblock_to_array ( var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : array<ExpressionPtr>
    //! Implementation details for the expression reificaiton. This is a block reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    var inscope arr : array<ExpressionPtr>
    for e in qblk.list
        arr |> emplace_new <| clone_expression(e)
    return <- arr

def public apply_qblock_expr ( var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : ExpressionPtr
    //! Implementation details for the expression reificaiton. This is a frist line of the block as expression reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    assert(qblk.list |> length == 1)
    return <- clone_to_move(qblk.list[0])

def public apply_qtype ( var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : TypeDeclPtr
    //! Implementation details for the expression reificaiton. This is a type declaration reification.
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprTypeDecl)
    return <- clone_type((res as ExprTypeDecl).typeexpr)

def public expression_at ( var expr:ExpressionPtr; at:LineInfo ) : ExpressionPtr
    //! Force expression location, than return it.
    expr |> force_at(at)
    return <- expr

class private  AstQCallMacro : AstCallMacro
    //! This macro implements expression reification
    apply_call = "apply_qmacro"
    macro_call = "qmacro"
    def override canVisitArgument ( call:smart_ptr<ExprCallMacro>; argIndex:int ) : bool
        return false    // nop, this one is just like quote
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        if call.arguments |> length != 1
            macro_error(prog,call.at,"expecting 1 argument, {macro_call}(expression)")
            return <- [[ExpressionPtr]]
        var inscope qclone <- clone_expression(call.arguments[0])
        var inscope qblk <- apply_qrules(qclone)
        if qblk==null
            return <- [[ExpressionPtr]]
        var inscope acall <- new [[ExprCall() at=call.at,
            name:=apply_call,
            arguments := [{ExpressionPtr
                new [[ExprQuote()
                    at=call.at,
                    arguments := [{ExpressionPtr[1] qclone}]
                ]];
                new [[ExprMakeBlock() _block<-qblk, at=call.at]]
            }]
        ]]
        return <- acall

[call_macro(name="qmacro")]
class private QMacro : AstQCallMacro {}

[call_macro(name="qmacro_block")]
class private QBlockMacro : AstQCallMacro
    override apply_call = "apply_qblock"
    override macro_call = "qmacro_block"

[call_macro(name="qmacro_block_to_array")]
class private QBlockToArrayMacro : AstQCallMacro
    override apply_call = "apply_qblock_to_array"
    override macro_call = "qmacro_block_to_array"

[call_macro(name="qmacro_expr")]
class private QBlockExprMacro : AstQCallMacro
    override apply_call = "apply_qblock_expr"
    override macro_call = "qmacro_expr"

[call_macro(name="qmacro_type")]
class private QTypeMacro : AstQCallMacro
    override apply_call = "apply_qtype"
    override macro_call = "qmacro_type"

def public emplace_new ( var arr:array<ExpressionPtr>; var expr:ExpressionPtr )
    //! Unifies emplace and emplace_new for the array<ExpressionPtr>
    arr |> emplace(expr)

def public emplace_new ( var arr:array<VariablePtr>; var expr:VariablePtr )
    //! Unifies emplace and emplace_new for the array<VariablePtr>
    arr |> emplace(expr)

def public apply_qmacro_function ( fname:string; var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : FunctionPtr
    //! Implementation details for reification. This is a function generation reification.
    var inscope func <- new [[Function() at=expr.at, atDecl=expr.at, name:=fname, flags=FunctionFlags generated]]
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprMakeBlock)
    var inscope qblk <- move_unquote_block(res)
    for arg in qblk.arguments
        func.arguments |> emplace_new <| clone_variable(arg)
    func.result := qblk.returnType
    qblk.arguments |> clear
    qblk.blockFlags ^= ExprBlockFlags isClosure
    move(func.body) <| qblk
    return <- func

def public apply_qmacro_method ( fname:string; parent:StructurePtr; var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : FunctionPtr
    //! Implementation details for reification. This is a class method function generation reification.
    var inscope func <- apply_qmacro_function(fname,expr,blk)
    func.flags |= FunctionFlags isClassMethod
    func.classParent = get_ptr(parent)
    return <- func

def public apply_qmacro_variable ( vname:string; var expr:smart_ptr<Expression>; blk:block<(var rules:Template):void> ) : VariablePtr
    //! Implementation details for reification. This is a variable generation reification.
    var inscope vvar <- new [[Variable() at=expr.at, name:=vname, flags=VariableFlags generated]]
    var inscope res <- apply_template(expr, blk)
    assert(res is ExprTypeDecl)
    move_new(vvar._type) <| clone_type((res as ExprTypeDecl).typeexpr)
    return <- vvar

class private AstQNamedMacro : AstCallMacro
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    apply_call = "apply_qmacro_function"
    macro_call = "qmacro_function"
    def override canVisitArgument ( call:smart_ptr<ExprCallMacro>; argIndex:int ) : bool
        return argIndex==0
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        if call.arguments |> length != 2
            macro_error(prog,call.at,"expecting 2 arguments, {macro_call}(name,expression)")
            return <- [[ExpressionPtr]]
        if !(call.arguments[0]._type.isString)
            macro_error(prog,call.at,"expecting function name, {macro_call}(NAME,expression)")
            return <- [[ExpressionPtr]]
        var inscope qclone <- clone_expression(call.arguments[1])
        var inscope qblk <- apply_qrules(qclone)
        if qblk==null
            return <- [[ExpressionPtr]]
        return <- new [[ExprCall() at=call.at,
            name:=apply_call,
            arguments := [{ExpressionPtr
                clone_expression(call.arguments[0]);
                new [[ExprQuote()
                    at=call.at,
                    arguments := [{ExpressionPtr[1] qclone}]
                ]];
                new [[ExprMakeBlock() _block<-qblk, at=call.at]]
            }]
        ]]

[call_macro(name="qmacro_function")]
class private AstQFunctionMacro : AstQNamedMacro {}

[call_macro(name="qmacro_variable")]
class private AstQVariableMacro : AstQNamedMacro
    override apply_call = "apply_qmacro_variable"
    override macro_call = "qmacro_variable"

class private AstQNamedClassMacro : AstCallMacro
    apply_call = "apply_qmacro_method"
    macro_call = "qmacro_method"
    def override canVisitArgument ( call:smart_ptr<ExprCallMacro>; argIndex:int ) : bool
        return argIndex==0 || argIndex==1
    //! This macro implements expression reification for the named expressions (function, variable, etc.)
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        if call.arguments |> length != 3
            macro_error(prog,call.at,"expecting 3 arguments, {macro_call}(name,class,expression)")
            return <- [[ExpressionPtr]]
        if !(call.arguments[0]._type.isString)
            macro_error(prog,call.at,"expecting function name, {macro_call}(NAME,class,expression)")
            return <- [[ExpressionPtr]]
        var inscope qclone <- clone_expression(call.arguments[2])
        var inscope qblk <- apply_qrules(qclone)
        if qblk==null
            return <- [[ExpressionPtr]]
        return <- new [[ExprCall() at=call.at,
            name:=apply_call,
            arguments := [{ExpressionPtr
                clone_expression(call.arguments[0]);
                clone_expression(call.arguments[1]);
                new [[ExprQuote()
                    at=call.at,
                    arguments := [{ExpressionPtr[1] qclone}]
                ]];
                new [[ExprMakeBlock() _block<-qblk, at=call.at]]
            }]
        ]]

[call_macro(name="qmacro_method")]
class private AstQMethodMacro : AstQNamedClassMacro
    //! This macro implements expression reification for class methods.
    override apply_call = "apply_qmacro_method"

def public add_structure_field ( var cls:StructurePtr; name:string; var t:TypeDeclPtr; var init:ExpressionPtr )
    //! Adds a field to the structure.
    let fi = length(cls.fields)
    cls.fields |> resize(fi+1)
    cls.fields[fi].name := name
    move(cls.fields[fi]._type) <| t
    move(cls.fields[fi].init) <| init
    return fi

def public make_class ( name:string; mod:Module? )
    //! Creates a class structure. Adds __rtti, __finalize fields.
    var inscope st <- new [[Structure() name:=name, flags = bitfield(1)]]
    unsafe(builtin_ast_make_class_rtti(get_ptr(st)))
    var inscope virt_fin <- unsafe(builtin_ast_make_class_finalize(get_ptr(st)))
    add_function(mod, virt_fin)
    return <- st

def public make_class ( name:string; baseClass:StructurePtr; mod:Module? )
    return <- make_class(name, get_ptr(baseClass), mod)

def public make_class ( name:string; baseClass:Structure?; mod:Module? )
    if mod == null
        panic("expecting module")
    var inscope st <- clone_structure(baseClass)
    st.name := name
    st.parent = baseClass
    unsafe(builtin_ast_make_class_rtti(get_ptr(st)))
    var inscope virt_fin <- unsafe(builtin_ast_make_class_finalize(get_ptr(st)))
    add_function(mod, virt_fin)
    return <- st

def public make_class_constructor ( cls:StructurePtr; ctor:FunctionPtr )
    //! Adds a class constructor from a constructor function.
    if cls==null
        panic("expecting class")
    if ctor==null
        panic("expecting constructor function")
    return <- unsafe(builtin_ast_make_class_constructor(get_ptr(cls), get_ptr(ctor)))

def public modify_to_class_member ( cls:StructurePtr; fun:FunctionPtr; isExplicit:bool; Constant:bool )
    //! Modifies function to be a member of a particular class.
    if cls==null
        panic("expecting class")
    if fun==null
        panic("expecting function")
    unsafe(builtin_ast_modify_to_class_member(get_ptr(fun), get_ptr(cls), isExplicit, Constant))
