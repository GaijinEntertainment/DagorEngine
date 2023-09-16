options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module match shared private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/strings_boost
require daslib/defer

/*
options debugger
require daslib/debug
*/

let LOG_MATCH = false

struct MatchError
    msg : string
    [[do_not_delete]] at : LineInfo const?

struct MatchTo
    conditions   : array<ExpressionPtr>
    declarations : table<string; ExpressionPtr>
    prefix       : array<ExpressionPtr>
    errors       : array<MatchError>
    failed       : bool

[macro_function]
def log_m ( msg:string )
    static_if LOG_MATCH
        to_log(LOG_INFO, msg)

[macro_function]
def match_error ( var errors : array<MatchError>; msg:string; at:LineInfo ) // !!!do not put access.at in there
    errors |> push([[MatchError msg=msg, at=unsafe(addr(at))]])

enum MatchType
    none
    as_is
    copy

[macro_function]
def get_match_type ( typ:TypeDeclPtr )
    if !typ.isStructure || typ.structType==null
        return MatchType none
    for ann in typ.structType.annotations
        if ann.annotation.name=="match_as_is"
            return MatchType as_is
        elif ann.annotation.name=="match_copy"
            return MatchType copy
    return MatchType none

[macro_function]
def match_as_is ( what:TypeDeclPtr; wths:Expression?; makeType:TypeDeclPtr; as_is_name:das_string; access:ExpressionPtr; var to:MatchTo )
    log_m("\tmatch_as_is {describe(what)} {describe(makeType)} {describe(access)}")
    var inscope cond_nz <- new [[ExprIsVariant() at=wths.at,
        value <- clone_expression(access),
        name := as_is_name
    ]]
    cond_nz |> force_at(wths.at)
    to.conditions |> emplace(cond_nz)
    var inscope access_expr <- new [[ExprAsVariant() at=wths.at,
        value <- clone_expression(access),
        name := as_is_name
    ]]
    return match_any(makeType, wths, access_expr, to)

[macro_function]
def match_copy ( what:TypeDeclPtr; wths:Expression?; makeType:TypeDeclPtr; as_is_name:das_string; access:ExpressionPtr; var to:MatchTo )
    log_m("\tmatch_copy {describe(what)} {describe(makeType)} {describe(access)}")
    let tempName = make_unique_private_name("__`match`copy",wths.at)
    var inscope prefixType <- clone_type(makeType)
    var inscope prefix <- qmacro_expr <|
        let $i(tempName) : $t(prefixType)
    prefix |> force_at(wths.at)
    to.prefix |> emplace <| prefix
    var inscope access_new <- qmacro($i(tempName))
    access_new |> force_at(wths.at)
    var inscope cond_nz <- qmacro(match_copy($i(tempName), $e(access))); cond_nz |> force_at(wths.at)
    to.conditions |> emplace(cond_nz)
    return match_any(makeType, wths, access_new, to)

[macro_function]
def match_struct ( what:TypeDeclPtr; wths:ExprMakeStruct?; access:ExpressionPtr; var to:MatchTo )
    // if its a pointer to ast::Expression or such, we need to 'as' cast first
    if (wths.makeType.isHandle && isExpression(wths.makeType)
            && what.isPointer && what.firstType!=null && isExpression(what.firstType))
        return match_as_is(what, wths,wths.makeType,  wths.makeType.annotation.name, access, to)
    // if its pointer to structure, automatically dereference it
    if what.baseType==Type tPointer && what.firstType!=null && (what.firstType.isStructure || what.firstType.isHandle)
        var inscope cond_nz <- qmacro($e(access) != null)
        cond_nz |> force_at(wths.at)
        to.conditions |> emplace(cond_nz)
        var inscope access_nz <- qmacro(*($e(access)))
        access_nz |> force_at(wths.at)
        return match_struct(what.firstType, wths, access_nz, to)
    if !(what.isStructure || what.isHandle)
        to.errors |> match_error("{describe(what)} does not match [[ ... ]]", wths.at)
        return false
    if wths.makeType!=null && !wths.makeType.isAuto && !is_same_type(what, wths.makeType, RefMatters no, ConstMatters no, TemporaryMatters no)
        if wths.makeType.isStructure
            let mtype = get_match_type(wths.makeType)
            if mtype==MatchType as_is
                return match_as_is(what, wths, wths.makeType, wths.makeType.structType.name, access, to)
            elif mtype==MatchType copy
                return match_copy(what, wths, wths.makeType, wths.makeType.structType.name, access, to)
        to.errors |> match_error("{describe(what)} type does not match [[{describe(wths.makeType)} ... ]] structure", wths.at)
        return false
    let wdl = length(what.dim)
    let wthsLength = length(wths.structs)
    let whatDim = wdl>0 ? what.dim[wdl-1] : 1
    if wthsLength != whatDim
        to.errors |> match_error("{describe(what)} number of elements does not match [[describe(wths.makeType)}[{wthsLength}] ... ]]", wths.at)
        return false
    for i in range(wthsLength)
        var fields : table<string>
        for mf in *wths.structs[i]
            let mfname = string(mf.name)
            if fields |> key_exists(mfname)
                to.errors |> match_error("duplicate field {mf.name}", mf.at)
                return false
            fields |> insert(mfname)
        delete fields
    for i in range(wthsLength)
        for mf in *wths.structs[i]
            if true // to create scope
                var inscope new_access <- whatDim==1 ? qmacro($e(access).$f(mf.name)) : qmacro($e(access)[$v(i)].$f(mf.name))
                new_access |> force_at(wths.at)
                var inscope ftype <- get_field_type(what, string(mf.name), what.flags.constant)
                if ftype == null
                    to.errors |> match_error("field {mf.name} not found", mf.at)
                    return false
                log_m("\t[[structure[{i}] {mf.name}]] match passed\n")
                if !match_any(ftype, get_ptr(mf.value), new_access, to)
                    return false
    return true

[macro_function]
def match_tuple ( what:TypeDeclPtr; wths:ExprMakeTuple?; access:ExpressionPtr; var to:MatchTo )
    if what.baseType!=Type tTuple
        to.errors |> match_error("{describe(what)} does not match [[ ... ]] tuple", wths.at)
        return false
    if wths.makeType!=null && !wths.makeType.isAuto && !is_same_type(what, wths.makeType, RefMatters no, ConstMatters no, TemporaryMatters no)
        to.errors |> match_error("{describe(what)} type does not match [[{describe(wths.makeType)} ... ]] tuple", wths.at)
        return false
    // find if there is ..., get its index
    var tagIndex = INT_MAX
    for v,i in wths.values,count()
        if (v is ExprTag) && (v as ExprTag).name=="..."
            if tagIndex!=INT_MAX
                to.errors |> match_error("... can only apper once per tuple match", wths.at)
                return false
            tagIndex = i
    // check lengths
    let wthsLength = length(wths.values)
    let whatDim = length(what.argTypes)
    if tagIndex==INT_MAX
        if whatDim != wthsLength
            to.errors |> match_error("{describe(what)} number of elements does not match [[describe(wths.makeType)} ... ]] tuple", wths.at)
            return false
    else
        if whatDim < wthsLength-1
            to.errors |> match_error("{describe(what)} needs at least {wthsLength-1} tuple elements to match", wths.at)
            return false
    log_m("\tuple match passed\n")
    for i in range(wthsLength)
        if i == tagIndex    // skip ...
            continue
        let ei = i<= tagIndex ? i : whatDim - (wthsLength - i)
        if true // to create scope for defer delete
            var inscope new_access <- qmacro($e(access).$f("_{ei}")); new_access |> force_at(wths.at)
            if !match_any(what.argTypes[ei], get_ptr(wths.values[i]), new_access, to)
                return false
    return true

[macro_function]
def match_array ( what:TypeDeclPtr; wths:ExprMakeArray?; dynamic:bool; access:ExpressionPtr; var to:MatchTo )
    let wdl = length(what.dim)
    var inscope recT : TypeDeclPtr
    // find if there is ..., get its index
    var tagIndex = INT_MAX
    for v,i in wths.values,count()
        if (v is ExprTag) && (v as ExprTag).name=="..."
            if tagIndex!=INT_MAX
                to.errors |> match_error("... can only apper once per array match", wths.at)
                return false
            tagIndex = i
    // check lengths
    let wthsLength = length(wths.values)
    if wdl != 0 // check if static array
        if dynamic
            to.errors |> match_error("{describe(what)} is not a dynamic array", wths.at)
            return false
        let whatDim = what.dim[wdl-1]
        if tagIndex==INT_MAX
            if whatDim!=wthsLength
                to.errors |> match_error("{describe(what)} number of elements does not match [[describe(wths.makeType)}[{wthsLength}] ... ]] array", wths.at)
                return false
        else
            if whatDim < wthsLength-1
                to.errors |> match_error("{describe(what)} needs at least {wthsLength-1} array elements to match", wths.at)
                return false
        move_new(recT) <| clone_type(what)
        recT.dim |> pop()
        if !wths.makeType.isAuto && !is_same_type(recT, wths.makeType, RefMatters no, ConstMatters no, TemporaryMatters no)
            to.errors |> match_error("{describe(what)} type does not match [[{describe(wths.recordType)}[{wthsLength}] ... ]] array", wths.at)
            return false
    elif !what.isGoodArrayType // check if dynamic array
        to.errors |> match_error("{describe(what)} is not an array", wths.at)
        return false
    else
        if !dynamic
            to.errors |> match_error("{describe(what)} is not a static array", wths.at)
            return false
        recT := what.firstType
        var inscope lenCond <- (tagIndex!=INT_MAX ?
            qmacro(length($e(access)) >= $v(wthsLength-1)) :
            qmacro(length($e(access)) == $v(wthsLength)))
        lenCond |> force_at(wths.at)
        to.conditions |> emplace(lenCond)
    log_m("\tarray match passed\n")
    for i in range(wthsLength)
        if i != tagIndex
            let ei = i<=tagIndex ? i : wthsLength-i
            var inscope new_access <- i<tagIndex ? qmacro($e(access)[$v(ei)]) : qmacro($e(access)[length($e(access)) - $v(ei)])
            new_access |> force_at(wths.at)
            if !match_any(recT, get_ptr(wths.values[i]), new_access, to)
                return false
    return true

[macro_function]
def match_variant ( what:TypeDeclPtr; wths:ExprMakeVariant?; access:ExpressionPtr; var to:MatchTo )
    if what.baseType!=Type tVariant
        to.errors |> match_error("{describe(what)} does not match [[ ... ]] variant", wths.at)
        return false
    if wths.makeType==null || !(wths.makeType.baseType==Type autoinfer || wths.makeType.baseType==Type tVariant)
        to.errors |> match_error("variant type has not been resolved", wths.at)
        return false
    if !wths.makeType.isAuto && !is_same_type(what, wths.makeType, RefMatters no, ConstMatters no, TemporaryMatters no)
        to.errors |> match_error("{describe(what)} type does not match [[{describe(wths.makeType)} ... ]] variant", wths.at)
        return false
    let wdl = length(what.dim)
    let wthsLength = length(wths.variants)
    let whatDim = wdl>0 ? what.dim[wdl-1] : 1
    if wthsLength != whatDim
        to.errors |> match_error("{describe(what)} number of variant elements does not match [[describe(wths.makeType)}[{wthsLength}] ... ]]", wths.at)
        return false
    for mf,i in wths.variants,count()
        if true // to create scope for defer delete
            var inscope new_access <- whatDim==1 ? qmacro($e(access) as $f(mf.name)) : qmacro($e(access)[$v(i)] as $f(mf.name))
            new_access |> force_at(wths.at)
            var inscope ftype <- get_field_type(what, string(mf.name), what.flags.constant)
            if ftype == null
                to.errors |> match_error("variant {mf.name} not found", mf.at)
                return false
            to.conditions |> emplace_new <| new [[ExprIsVariant() at=wths.at,
                value <- clone_expression(access),
                name := mf.name
            ]]
            log_m("\t[[variant {mf.name}]] match passed\n")
            if !match_any(ftype, get_ptr(mf.value), new_access, to)
                return false
    return true

[macro_function]
def match_as_variant ( what:TypeDeclPtr; wthAs:ExprAsVariant?; access:ExpressionPtr; var to:MatchTo )
    if what.baseType!=Type tVariant
        to.errors |> match_error("{describe(what)} is not a variant", wthAs.at)
        return false
    var inscope ftype <- get_field_type(what, string(wthAs.name), what.flags.constant)
    if ftype == null
        to.errors |> match_error("variant {wthAs.name} not found", wthAs.at)
        return false
    to.conditions |> emplace_new <| new [[ExprIsVariant() at=wthAs.at,
        value <- clone_expression(access),
        name := wthAs.name
    ]]
    var inscope new_access <- new [[ExprAsVariant() at=wthAs.at,
        value <- clone_expression(access),
        name := wthAs.name
    ]]
    log_m("\tas {wthAs.name} match passed\n")
    return match_any(ftype, get_ptr(wthAs.value), access, to)

[macro_function]
def match_type ( what:TypeDeclPtr; wthmt:ExprCall?; access:ExpressionPtr; var to:MatchTo )
    if length(wthmt.arguments)!=2   // match_type(expr, type)
        to.errors |> match_error("match_type expects 2 arguments", wthmt.at)
        return false
    assume value = wthmt.arguments[1]
    assume mtype = wthmt.arguments[0]._type
    if mtype==null
        to.errors |> match_error("unresolved type", wthmt.arguments[0].at)
        to.failed = true
        return false
    if is_same_type(what, mtype, RefMatters no, ConstMatters no, TemporaryMatters no)
        log_m("\tmatch_type {describe(what)} passed\n")
        return match_any(what, get_ptr(value), access, to)
    else
        let mt = get_match_type(mtype)
        if mt==MatchType as_is
            return match_as_is(mtype, wthmt, mtype, mtype.structType.name, access, to)
        elif mt==MatchType copy
            return match_copy(mtype, wthmt, mtype, mtype.structType.name, access, to)
        to.errors |> match_error("mismatching type {describe(what)} vs {describe(mtype)}", mtype.at)
        return false

[macro_function]
def match_expr ( what:TypeDeclPtr; wthmt:ExprCall?; access:ExpressionPtr; var to:MatchTo )
    if length(wthmt.arguments)!=1   // match_expr(expr)
        to.errors |> match_error("match_expr expects 1 argument", wthmt.at)
        return false
    assume value = wthmt.arguments[0]
    if value._type!=null && !is_same_type(what, value._type, RefMatters no, ConstMatters no, TemporaryMatters no)
        to.errors |> match_error("mismatching expression type {describe(what)} vs {describe(value._type)}", value.at)
        return false
    var inscope expr <- clone_expression(value)
    apply_template(expr) <| $ ( rules )
        for dv,dn in keys(to.declarations),values(to.declarations)
            rules |> replaceVariable(dv) <| clone_expression(dn)
        rules |> replaceVarTag("_") <| clone_expression(access)
    var inscope cond <- qmacro($e(access) == $e(expr))
    cond |> force_at(value.at)
    to.conditions |> emplace(cond)
    return true

[macro_function]
def match_guards ( what:TypeDeclPtr; wth:ExprOp2?; access:ExpressionPtr; var to:MatchTo )
    if match_any(what, get_ptr(wth.left), access, to)
        log_m("\tguards {describe(wth.right)} at {wth.right.at}\n")
        var inscope guards <- clone_expression(wth.right)
        apply_template(guards) <| $ ( rules )
            for dv,dn in keys(to.declarations),values(to.declarations)
                rules |> replaceVariable(dv) <| clone_expression(dn)
            rules |> replaceVarTag("_") <| clone_expression(access)
        to.conditions |> emplace(guards)
        return true
    else
        return false

[macro_function]
def match_or ( what:TypeDeclPtr; wth:ExprOp2?; access:ExpressionPtr; var to:MatchTo )
    var inscope toL : MatchTo
    toL.declarations := to.declarations
    if !match_any(what, get_ptr(wth.left), access, toL)
        return false
    var inscope toR : MatchTo
    if !match_any(what, get_ptr(wth.right), access, toR)
        return false
    for k in keys(to.declarations)
        toL.declarations |> erase(k)
        toR.declarations |> erase(k)
    if length(toL.declarations) != length(toR.declarations)
        to.errors |> match_error("mismatching variable declarations in ||", wth.at)
        return false
    for k,vL in keys(toL.declarations),values(toL.declarations)
        if describe(vL) != describe(toR.declarations[k])
            to.errors |> match_error("mismatching variable {k} declaration in ||", wth.at)
            return false
    delete to.declarations
    unsafe
        to.declarations <- toL.declarations // because its a superset
    var inscope lcond <- join_conditions(toL.conditions, wth.left.at)
    var inscope rcond <- join_conditions(toR.conditions, wth.right.at)
    var inscope cond <- qmacro($e(lcond) || $e(rcond))
    cond |> force_at(wth.at)
    to.conditions |> emplace(cond)
    log_m("\tmatch_or {describe(what)} passed\n")
    return true

[macro_function]
def match_tag ( what:TypeDeclPtr; wth:Expression?; access:ExpressionPtr; var to:MatchTo )
    var tag = wth as ExprTag
    if tag.name!="v"
        to.errors |> match_error("unsupported match tag ${tag.name}", tag.at)
        return true
    if tag.subexpr is ExprVar
        let tname  = string((tag.subexpr as ExprVar).name)
        if to.declarations |> key_exists(tname)
            to.errors |> match_error("duplicate variable {tname}", tag.at)
            return false
        move_new(to.declarations[tname]) <| clone_expression(access)
        log_m("\tadd variable {tname} as {describe(access)}\n")
        return true
    else
        to.errors |> match_error("invalid $v syntax", tag.at)
        return false

[macro_function]
def is_match_call ( wth:Expression?; name:string )
    return (wth is ExprCall) && ((wth as ExprCall).name=="match_{name}" || (wth as ExprCall).name=="__::match`match_{name}")

[macro_function]
def match_any ( what:TypeDeclPtr; wth:Expression?; access:ExpressionPtr; var to:MatchTo )
    if LOG_MATCH
        log_m("match {describe(what)} {debug_expression(wth)}:\n")
    if wth is ExprTag
        return match_tag(what, wth, access, to)
    elif (wth is ExprVar) && (wth as ExprVar).name=="_"
        log_m("\tmatch anything via _n\n")
        return true
    elif wth is ExprMakeStruct
        return match_struct(what, wth as ExprMakeStruct, access, to)
    elif wth is ExprMakeArray
        var mka = wth as ExprMakeArray
        if length(mka.values)==1 && (mka.values[0] is ExprMakeTuple)
            return match_tuple(what, (mka.values[0] as ExprMakeTuple), access, to)
        else
            return match_array(what, wth as ExprMakeArray, false, access, to)
    elif wth is ExprMakeVariant
        return match_variant(what, wth as ExprMakeVariant, access, to)
    elif wth is ExprAsVariant
        return match_as_variant(what, wth as ExprAsVariant, access, to)
    elif wth |> is_match_call("type")
        return match_type(what, wth as ExprCall, access, to)
    elif wth |> is_match_call("expr")
        return match_expr(what, wth as ExprCall, access, to)
    elif (wth is ExprCall) && ((wth as ExprCall).name=="to_array_move" || (wth as ExprCall).name=="__::builtin`to_array_move")
        var cll = wth as ExprCall
        if length(cll.arguments)!=1
            to.errors |> match_error("to_array_move expects 1 argument", cll.at)
            return false
        if cll.arguments[0] is ExprMakeArray
            return match_array(what, cll.arguments[0] as ExprMakeArray, true, access, to)
        else
            to.errors |> match_error("to_array_move expects array", cll.at)
            return false
    elif (wth is ExprOp2) && (wth as ExprOp2).op=="&&"
        return match_guards(what, wth as ExprOp2, access, to)
    elif (wth is ExprOp2) && (wth as ExprOp2).op=="||"
        return match_or(what, wth as ExprOp2, access, to)
    elif string(wth.__rtti) |> starts_with("ExprConst")
        if wth._type == null
            to.errors |> match_error("unresolved constant type", wth.at)
            return false
        if wth.__rtti=="ExprConstPtr" && what.isPointer
            pass
        elif !is_same_type(what, wth._type, RefMatters no, ConstMatters no, TemporaryMatters no)
            to.errors |> match_error("condition type mismatch {describe(what)} vs {describe(wth._type)}", wth.at)
            return false
        log_m("\tadd condition {describe(access)} == {describe(wth)}\n")
        var inscope ee := unsafe(reinterpret<ExpressionPtr> wth)
        var inscope cond <- qmacro($e(access) == $e(ee))
        cond |> force_at(wth.at)
        to.conditions |> emplace(cond)
        return true
    elif wth._type!=null && is_same_type(what,wth._type, RefMatters no, ConstMatters no, TemporaryMatters no)
        log_m("\tadd condition {describe(access)} == {describe(wth)}\n")
        var inscope ee := unsafe(reinterpret<ExpressionPtr> copy_to_local(wth))
        var inscope cond <- qmacro($e(access) == $e(ee))
        cond |> force_at(wth.at)
        to.conditions |> emplace(cond)
        return true
    else
        to.errors |> match_error("usupported match syntax {wth.__rtti}", wth.at)
        to.failed = true
        return false

[macro_function]
def join_conditions ( var conditions:array<ExpressionPtr>; at:LineInfo ) : ExpressionPtr
    let clen = length(conditions)
    if clen == 0
        return <- new [[ExprConstBool() value=true, at=at]]
    var inscope tail <- conditions |> back()
    conditions |> pop()
    while length(conditions) > 0
        if true
            var inscope last <- conditions |> back()
            conditions |> pop()
            var inscope new_tail <- new [[ExprOp2() at=last.at, op:="&&", left<-last, right<-tail]]
            move(tail) <| new_tail
    return <- tail

[call_macro(name="match")]
class MatchMacro : AstCallMacro
    //! Implements `match` macro.
    report_match_errors = true
    multi_match = false
    def override canVisitArgument ( expr:smart_ptr<ExprCallMacro>; argIndex:int ) : bool
        // if we are reporting errors, and macro did not fold - no point showing errors inside match - there will be too many, and they are meaningless
        return !is_reporting_compilation_errors()
    def override canFoldReturnResult ( expr:smart_ptr<ExprCallMacro> ) : bool
        // function can't decide if its void or not, until match is folded
        return false
    def override preVisit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : void
        return  if length(expr.arguments) != 2
        assume blk = expr.arguments[1]
        return  if blk._type==null
        return  if !blk._type.isGoodBlockType
        return  if !(blk is ExprMakeBlock)
        // we mark all if's inside the match to not fold, so that we can fold the match later
        var eblk = ((blk as ExprMakeBlock)._block as ExprBlock)
        for ee in eblk.list
            if ee is ExprIfThenElse
                (ee as ExprIfThenElse).if_flags |= IfFlags doNotFold
    def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify(length(expr.arguments) == 2, prog, expr.at, "expecting match(what) <| block")
        assume what = expr.arguments[0]
        assume blk = expr.arguments[1]
        macro_verify(what._type!=null && !what._type.isAutoOrAlias, prog, expr.at, "match `what` argument did not resolve")
        macro_verify(blk._type.isGoodBlockType, prog, expr.at, "match `block` argument did not resolve")
        macro_verify(blk is ExprMakeBlock, prog, expr.at, "match `block` argument must be immediate block declaration")
        var eblk = ((blk as ExprMakeBlock)._block as ExprBlock)
        macro_verify(length(eblk.finalList) == 0, prog, expr.at, "match `block` argument can't have finally section")
        for ee in eblk.list
            if ee is ExprIfThenElse
                let eite = ee as ExprIfThenElse
                macro_verify(eite.if_false==null, prog, expr.at, "match to `if` can't have `else` section")
            else
                macro_error(prog, ee.at, "match `block` argument can only contain match-to calls, not {ee.__rtti}")
        var inscope access <- clone_expression(what)
        var inscope iff : smart_ptr<ExprIfThenElse>
        var inscope iffb : array<smart_ptr<ExprIfThenElse>>
        let eli = length(eblk.list)
        var any_errors = false
        var inscope prefix : array<ExpressionPtr>
        for ei in range(eli)
            if true
                var eto = (eblk.list[multi_match ? ei : eli-ei-1]) as ExprIfThenElse
                var inscope to : MatchTo
                if match_any(what._type, get_ptr(eto.cond), access, to)
                    log_m("match_any ok\n\n")
                    var inscope new_iff <- new [[ExprIfThenElse() at=eto.at, cond<-join_conditions(to.conditions, eto.at)]]
                    var inscope nblk <- new [[ExprBlock() at=eto.at]]
                    for l in (eto.if_true as ExprBlock).list
                        nblk.list |> emplace_new <| clone_expression(l)
                    for l in (eto.if_true as ExprBlock).finalList
                        nblk.finalList |> emplace_new <| clone_expression(l)
                    apply_template(nblk) <| $ ( rules )
                        for dv,dn in keys(to.declarations),values(to.declarations)
                            rules |> replaceVariable(dv) <| clone_expression(dn)
                        rules |> replaceVarTag("_") <| clone_expression(access)
                    move(new_iff.if_true) <| nblk
                    if multi_match
                        iffb |> emplace(new_iff)
                    else
                        move(new_iff.if_false) <| iff
                        move(iff) <| new_iff
                elif report_match_errors || to.failed
                    log_m("match_any failed\n\n")
                    any_errors = true
                    if length(to.errors) != 0
                        for e in to.errors
                            macro_error(prog, *e.at, e.msg)
                    else
                        macro_error(prog, eto.at, "unspecified match error")
                    break
                static_if LOG_MATCH
                    for e in to.errors
                        log_m("\tERROR:: {e.msg} at {describe(*e.at)}\n")
                if length(to.prefix) != 0
                    let p0 = length(prefix)
                    prefix |> resize(p0+length(to.prefix))
                    for p,i in to.prefix,count(p0)
                        move(prefix[i]) <| p
        if any_errors
            return <- [[ExpressionPtr]]
        if multi_match
            for ib in iffb
                prefix |> emplace(ib)
            iffb |> clear()
            return <- qmacro($b(prefix))
        else
            prefix |> emplace(iff)
            return <- qmacro($b(prefix))

[call_macro(name="static_match")]
class StaticMatchMacro : MatchMacro
    //! Implements `static_match` macro.
    override report_match_errors = false

[call_macro(name="multi_match")]
class MultiMatchMacro : MatchMacro
    //! Implements `multi_match` macro.
    override multi_match = true

[call_macro(name="static_multi_match")]
class StaticMultiMatchMacro : MatchMacro
    //! Implements `static_multi_match` macro.
    override multi_match = true
    override report_match_errors = false

[structure_macro(name="match_as_is")]
class MatchAsIs : AstStructureAnnotation {
    //! Implements `match_as_is` annotation.
    //! This annotation is used to mark that structure can be matched with different type via is and as machinery.
}

[structure_macro(name="match_copy")]
class MatchCopy : AstStructureAnnotation {
    //! Implements `match_copy` annotation.
    //! This annotation is used to mark that structure can be matched with different type via match_copy machinery.
}


def public match_type(anytype,anything) : void
    //! this is a macro, it will be replaced with a match expression of specified type
    pass

def public match_expr(anything) : void
    //! this is a macro, it will be replaced with a match expression
    pass

[_macro]
def private setup
    if is_compiling_macros_in_module("match")
        compiling_module() |> add_keyword("match", false)
        compiling_module() |> add_keyword("static_match", false)
        compiling_module() |> add_keyword("multi_match", false)
        compiling_module() |> add_keyword("static_multi_match", false)
        compiling_module() |> add_keyword("match_type", true)
