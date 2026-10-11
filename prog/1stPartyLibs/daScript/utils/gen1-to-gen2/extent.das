options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require daslib/ast
require daslib/ast_boost
require math
require strings
require daslib/strings_boost
require offsets
require comments
require scan


struct public Extent {
    from : int
    to : int
    ok : bool
}


class private ExtentVisitor : AstVisitor {
    @do_not_delete src : Source const?
    @do_not_delete lex : Lexical const?
    from : int = 0
    to : int = 0
    seen : bool = false

    def take(at : LineInfo) {
        if (at.fileInfo == null) return
        let a = span_begin((*src), at)
        let b = span_end((*src), at)
        if (b <= a) return
        if (!seen) {
            from = a
            to = b
            seen = true
            return
        }
        from = min(from, a)
        to = max(to, b)
    }

    def override preVisitExpression(expr : ExpressionPtr) {
        take(expr.at)
    }

    def override preVisitExprCall(expr : ExprCall?) {
        take(expr.at)
        take(expr.atEnclosure)
    }

    def override preVisitExprLooksLikeCall(expr : ExprLooksLikeCall?) {
        take(expr.at)
        take(expr.atEnclosure)
    }

    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) {
        take(expr.at)
        let litFrom = span_begin((*src), expr.at)
        for (str in (*lex).strings) {
            if (str.from > litFrom) break
            if (str.to > litFrom && str.to > to) {
                to = str.to
            }
        }
    }

    def override preVisitExprField(expr : ExprField?) {
        take(expr.at)
        take(expr.atField)
    }

    def reach_field(at : LineInfo) {
        let e = ident_after((*src), span_end((*src), at))
        if (e > to) {
            to = e
        }
    }

    def override preVisitExprIsVariant(expr : ExprIsVariant?) {
        take(expr.at)
        reach_field(expr.at)
    }

    def override preVisitExprAsVariant(expr : ExprAsVariant?) {
        take(expr.at)
        reach_field(expr.at)
    }

    def override preVisitExprSafeAsVariant(expr : ExprSafeAsVariant?) {
        take(expr.at)
        reach_field(expr.at)
    }

    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
        take(expr.at)
        let after = span_end((*src), expr.at)
        let lp = find_token((*src), (*lex), after, "(")
        if (lp < 0 || text_of((*src), after, lp) |> find("\n") >= 0) return
        let rp = paren_close((*src), (*lex), lp)
        if (rp >= 0) {
            to = max(to, rp + 1)
        }
    }

    def take_make_closer(at : LineInfo) {
        let open = span_begin((*src), at)
        let two = text_of((*src), open, open + 2)
        let one = text_of((*src), open, open + 1)
        var closer = -1
        if (two == "[[") {
            closer = find_closer((*src), (*lex), open + 2, "]]")
            if (closer >= 0) { closer += 2 }
        } elif (two == "[\{") {
            closer = find_closer((*src), (*lex), open + 2, "\}]")
            if (closer >= 0) { closer += 2 }
        } elif (two == "\{\{") {
            closer = find_closer((*src), (*lex), open + 2, "\}\}")
            if (closer >= 0) { closer += 2 }
        } elif (one == "[") {
            closer = group_close((*src), (*lex), open, "[", "]")
            if (closer >= 0) { closer++ }
        } elif (one == "\{") {
            closer = group_close((*src), (*lex), open, "\{", "\}")
            if (closer >= 0) { closer++ }
        }
        if (closer > to) {
            to = closer
        }
    }

    def override preVisitExprMakeArray(expr : ExprMakeArray?) {
        take(expr.at)
        take_make_closer(expr.at)
    }

    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        take(expr.at)
        take_make_closer(expr.at)
    }

    def override visitExprAt(var expr : ExprAt?) : ExpressionPtr {
        let ie = extent_of((*src), (*lex), expr.index)
        if (!ie.ok) return expr
        let closer = find_token((*src), (*lex), ie.to, "]")
        if (closer >= 0) {
            to = max(to, closer + 1)
        }
        return expr
    }
}


class private BlockFinder : AstVisitor {
    found : bool = false

    def override preVisitExprBlock(expr : ExprBlock?) {
        found = true
    }
}


def public contains_block(expr : ExpressionPtr) : bool {
    //! True when the expression carries a block of its own - a piped `$ [es] (...)` body, a
    //! lambda. Its lines are that block's statements, not continuations of the expression.
    if (expr == null) return false
    var v = new BlockFinder()
    make_visitor(*v) $(adapter) {
        visit(expr, adapter)
    }
    let found = v.found
    unsafe {
        delete v
    }
    return found
}


def private ident_after(src : Source; from : int) : int {
    var i = from
    var e = from
    peek_data(src.text) $(bytes) {
        let n = length(bytes)
        while (i < n) {
            let c = int(bytes[i])
            if (c != ' ' && c != '\t') break
            i++
        }
        var j = i
        while (j < n) {
            let c = int(bytes[j])
            if (!is_alpha(c) && !is_number(c) && c != '_') break
            j++
        }
        e = j > i ? j : from
    }
    return e
}


def public extent_of(src : Source; lex : Lexical; expr : ExpressionPtr) : Extent {
    //! The source range an expression covers, anchors unioned over the whole subtree.
    if (expr == null) return Extent(ok = false)
    var v = new ExtentVisitor()
    unsafe {
        v.src = addr(src)
        v.lex = addr(lex)
    }
    make_visitor(*v) $(adapter) {
        visit(expr, adapter)
    }
    let result = Extent(from = v.from, to = v.to, ok = v.seen)
    unsafe {
        delete v
    }
    return result
}
