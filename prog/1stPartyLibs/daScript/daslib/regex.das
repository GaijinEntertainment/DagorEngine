options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module regex shared public

//! Regular expression engine.
//!
//! Pure-daslang regex compiler and NFA matcher. Supports character classes,
//! capturing and non-capturing groups, named groups ``(?P<name>...)``,
//! quantifiers (greedy and lazy), counted repetitions ``{n,m}``,
//! alternation, anchors, word boundaries, character sets,
//! lookahead assertions ``(?=...)`` and ``(?!...)``,
//! case-insensitive matching, and dot-all mode.
//!
//! Key functions: ``regex_compile``, ``regex_match``, ``regex_search``,
//! ``regex_foreach``, ``regex_replace`` (block and template-string forms),
//! ``regex_split``, ``regex_match_all``, ``regex_group``,
//! ``regex_group_by_name``.
//!
//! See also ``regex_boost`` for compile-time regex construction via the
//! ``%regex~`` reader macro with optional flags (``~i`` for case-insensitive,
//! ``~s`` for dot-all).

require daslib/strings_boost
require strings public

enum ReOp {
    //! Type of regular expression operation.
    Char
    Set
    Any
    Eos
    Bos
    Group
    Plus
    Star
    Question
    Concat
    Union
    Repeat
    WordBoundary
    NonWordBoundary
    Lookahead
    NegativeLookahead
}

//! Bit-set representing a set of characters for regex matching.
typedef CharSet = uint[8]
//! Iterator of random uints used for regex-based string generation.
typedef ReGenRandom = iterator<uint>


struct ReNode {
    //! Single node in regular expression parsing tree.
    op      : ReOp
    id      : int
    @do_not_convert fun2 : function< (var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? >
    @do_not_convert gen2 : function< (var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void >
    at      : range
    text    : string
    textLen : int
    all     : array<ReNode?>
    left    : ReNode?
    right   : ReNode?
    subexpr : ReNode?
    @do_not_delete@do_not_convert next : ReNode?
    cset    : CharSet
    index   : int
    min_rep : int   // minimum repetitions for Repeat
    max_rep : int   // maximum repetitions for Repeat (-1 = unlimited)
    lazy    : bool  // true for lazy quantifiers (*?, +?, ??, {n,m}?)
    @do_not_convert tail : uint8 const?
}

struct Regex {
    //! Regular expression.
    root            : ReNode?
    @do_not_convert match : uint8 const?
    groups          : array<tuple<range; string>>
    earlyOut        : CharSet
    canEarlyOut     : bool
    caseInsensitive : bool   //! when true, matching is case-insensitive (ASCII)
    dotAll          : bool   //! when true, ``.`` matches ``\n`` as well
}

variant MaybeReNode {
    //! Single regular expression node or nothing.
    value   : ReNode?
    nothing : void?
}

/*
    Parser
*/

let private log_parse_enabled = false
let private log_match_enabled = false
let private range_check_enabled = false
let private log_gen_enabled = false

var private trace_tab = 0
var private re_gen_rep_limit = 8u

def public re_gen_get_rep_limit {
    //! Returns the current repetition limit for regex string generation.
    return re_gen_rep_limit
}

def private trace(msg : string; offset : int) {
    static_if (log_parse_enabled) {
        print("{repeat(" ",trace_tab)}{offset}: {msg}\n")
        trace_tab ++
    }
}

def private note(msg : string; offset : int) {
    static_if (log_parse_enabled) {
        print("{repeat(" ",trace_tab)}{offset}: {msg}\n")
    }
}

def private trace_backtrack() {
    static_if (log_parse_enabled) {
        trace_tab --
        print("{repeat(" ",trace_tab)}backtrack\n")
    }
}

def private trace_value(value : ReNode?) {
    static_if (log_parse_enabled) {
        if (value == null) {
            panic("trace_value null?")
        }
        trace_tab --
        print("{repeat(" ",trace_tab)}matched {value.op}, next {value.at.y}\n")
    }
}

def private nada() {
    trace_backtrack()
    return MaybeReNode(nothing = null)
}

def private maybe(var value : ReNode?) {
    trace_value(value)
    return MaybeReNode(value = value)
}

def private eos(expr : string; offset : int) {
    return offset >= length(expr)
}

def private at(expr : string; offset : int) : int {
    static_if (range_check_enabled) {
        let len = length(expr)
        if (offset < len) {
            return character_at(expr, offset)
        } elif (offset == len) {
            return 0
        } else {
            stackwalk()
            panic("out of range {offset} of {len}")
            return 0
        }
    } else {
        unsafe {
            return character_uat(expr, offset)
        }
    }
}

def private next(re : MaybeReNode) {
    if (!(re is value)) {
        stackwalk()
        panic("expecting value")
    }
    return (re as value).at.y
}

def private is_set_empty(cset : CharSet) {
    for (x in cset) {
        if (x != 0u) {
            return false
        }
    }
    return true
}

def private set_or_char(var cset : CharSet; ch : int) {
    cset[ch >> 5] |= uint(1 << (ch & 31))
}

def private set_or_range(var cset : CharSet; bits : range) {
    for (ch in range(bits.x, bits.y + 1)) {
        cset[ch >> 5] |= uint(1 << (ch & 31))
    }
}

def private set_invert(var cset : CharSet) {
    for (x in cset) {
        x ^= 0xffffffff
    }
}

def private set_or_set(var cset : CharSet; eset : CharSet) {
    for (x, y in cset, eset) {
        x |= y
    }
}

def private set_negative(var cset : CharSet) {
    for (x in cset) {
        x = ~x
    }
}

def private set_meta(var cset : CharSet; che : int) {
    if (che == 'w') {
        set_or_range(cset, range('a', 'z'))
        set_or_range(cset, range('A', 'Z'))
        set_or_range(cset, range('0', '9'))
        set_or_char(cset, '_')
    } elif (che == 'W') {
        var eset : CharSet
        set_meta(eset, 'w')
        set_negative(eset)
        set_or_set(cset, eset)
    } elif (che == 's') {
        set_or_char(cset, ' ')
        set_or_char(cset, '\t')
        set_or_char(cset, '\n')
        set_or_char(cset, '\r')
        set_or_char(cset, int(0x0C))  // \f
        set_or_char(cset, int(0x0B))  // \v
    } elif (che == 'S') {
        var eset : CharSet
        set_meta(eset, 's')
        set_negative(eset)
        set_or_set(cset, eset)
    } elif (che == 'd') {
        set_or_range(cset, range('0', '9'))
    } elif (che == 'D') {
        var eset : CharSet
        set_meta(eset, 'd')
        set_negative(eset)
        set_or_set(cset, eset)
    } elif (che == 'n') {
        set_or_char(cset, '\n')
    } elif (che == 'r') {
        set_or_char(cset, '\r')
    } elif (che == 't') {
        set_or_char(cset, '\t')
    } elif (che == 'f') {
        set_or_char(cset, int(0x0C))
    } elif (che == 'v') {
        set_or_char(cset, int(0x0B))
    } else {
        set_or_char(cset, che)
    }
}

let private meta = "\\+*.()[]|^\{}"
let private meta_set = "wWsSdDnrtfvbB"

def private is_meta_character(ch : int) : bool {
    return find(meta, ch) != -1
}

def private is_set_character(ch : int) : bool {
    return find(meta_set, ch) != -1
}

def private is_digit(ch : int) : bool {
    return ch >= '0' && ch <= '9'
}

def private is_word_character(ch : int) : bool {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'
}

def private from_hex(ch : int) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0'
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10
    }
    panic("from_hex")
    return 0
}

def private from_hex(ch1, ch2) {
    return from_hex(ch1) * 16 + from_hex(ch2)
}

def private string_from_hex(ch1 : int) {
    return to_char(from_hex(ch1))
}

def private string_from_hex(ch1, ch2 : int) {
    return to_char(from_hex(ch1, ch2))
}

// <char> ::= any non metacharacter | "\" metacharacter
def private re_char(expr : string; offset : int) : MaybeReNode {
    trace("re_char", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    let ch = at(expr, offset)
    if (ch == '\\') {
        let ech = at(expr, offset + 1)
        if (ech == 0) {
            note("syntax error, expecting meta", offset + 1)
            return nada()
        }
        if (ech == 'x') {
            let len = length(expr)
            if (offset + 2 >= len) {
                note("syntax error, expecting hex on hex1, too long {offset+1} vs {len}", offset + 2)
                return nada()
            }
            let hex1 = at(expr, offset + 2)
            if (!is_hex(hex1)) {
                note("syntax error, expecting hex on hex1, {to_char(hex1)} is not a hex", offset + 2)
                return nada()
            }
            if (offset + 2 < len) {
                let hex2 = at(expr, offset + 3)
                if (is_hex(hex2)) {
                    note("hex \\{to_char(hex1)}{to_char(hex2)} {uint(from_hex(hex1,hex2))}", offset)
                    return maybe(new ReNode(op = ReOp.Char, text = string_from_hex(hex1, hex2), textLen = 1, at = range(offset, offset + 4)))
                }
            }
            note("hex \\{to_char(hex1)} {uint(from_hex(hex1))}", offset)
            return maybe(new ReNode(op = ReOp.Char, text = string_from_hex(hex1), textLen = 1, at = range(offset, offset + 3)))
        }
        if (is_set_character(ech)) {
            if (ech == 'b') {
                return maybe(new ReNode(op = ReOp.WordBoundary, at = range(offset, offset + 2)))
            } elif (ech == 'B') {
                return maybe(new ReNode(op = ReOp.NonWordBoundary, at = range(offset, offset + 2)))
            }
            var cset : CharSet
            set_meta(cset, ech)
            return maybe(new ReNode(op = ReOp.Set, cset = cset, at = range(offset, offset + 2)))
        }
        return maybe(new ReNode(op = ReOp.Char, text = to_char(ech), textLen = 1, at = range(offset, offset + 2)))
    } elif (is_meta_character(ch)) {
        return nada()
    }
    return maybe(new ReNode(op = ReOp.Char, text = to_char(ch), textLen = 1, at = range(offset, offset + 1)))
}

// <set-items> ::= <set-item> | <set-item> <set-items>
// <set-item> ::= <range> | <char>
// <range> ::= <char> "-" <char>
// <char> ::= any non metacharacter | "\" metacharacter
def private re_set_items(expr : string; offset : int) : MaybeReNode {
    trace("re_set_items", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    var cset : CharSet
    var iofs = offset
    var prev_char = -1
    var next_range = false
    var next_meta = false
    while (!eos(expr, iofs)) {
        let ch = at(expr, iofs)
        var next_char = -1
        if (ch == ']') {
            note("set - break - ]", iofs);
            break;
        } elif (ch == '\\') {
            let che = at(expr, iofs + 1);
            if (che == 0) {
                note("syntax error, expecting meta", iofs);
                return nada();
            }
            if (che == 'x') {
                let len = length(expr);
                if (iofs + 2 >= len) {
                    note("syntax error, expecting hex on hex1, too long {iofs+1} vs {len}", iofs + 2);
                    return nada();
                }
                let hex1 = at(expr, iofs + 2);
                if (!is_hex(hex1)) {
                    note("syntax error, expecting hex on hex1, {to_char(hex1)} is not a hex", iofs + 2);
                    return nada();
                }
                if (iofs + 2 < len) {
                    let hex2 = at(expr, iofs + 3);
                    if (is_hex(hex2)) {
                        next_char = from_hex(hex1, hex2);
                        note("set hex \\{to_char(hex1)}{to_char(hex2)} {uint(from_hex(hex1,hex2))}", iofs);
                        iofs += 4;
                    } else {
                        next_char = from_hex(hex1);
                        note("set hex \\{to_char(hex1)} {uint(from_hex(hex1))}", iofs);
                        iofs += 3;
                    }
                } else {
                    next_char = from_hex(hex1);
                    note("set hex \\{to_char(hex1)} {uint(from_hex(hex1))}", offset);
                    iofs += 3;
                }
            } else {
                if (next_range) {
                    note("syntax error, expecting range, got meta", iofs);
                    return nada();
                }
                if (is_set_character(che)) {
                    next_char = -1;
                    next_meta = true;
                    set_meta(cset, che);
                    note("set - meta {to_char(che)}", iofs);
                } else {
                    next_char = che;
                    note("set - {to_char(che)}", iofs);
                }
                iofs += 2;
            }
        } elif (ch == '-') {
            if (prev_char == -1) {
                note("syntax error, expecting range, got -", iofs);
                return nada();
            }
            next_range = true;
            iofs += 1;
        } else {
            next_char = ch;
            note("set - character {uint(ch)} {ch} {uint(ch)}", iofs);
            iofs += 1;
        }
        note("next_char {uint(next_char)} prev_char {uint(prev_char)} next_range {next_range}", iofs);
        if (next_char != -1) {
            if (next_range) {
                note("set - range {uint(prev_char)} {uint(next_char)}", iofs);
                set_or_range(cset, range(prev_char, next_char));
                next_range = false;
                prev_char = -1;
            } else {
                note("set - char {uint(next_char)}", iofs);
                set_or_char(cset, next_char);
                prev_char = next_char;
            }
        } elif (next_meta) {
            next_meta = false;
        } elif (!next_range) {
            note("syntax error???", iofs);
            return nada();
        }
    }
    return maybe(new ReNode(op = ReOp.Set, cset = cset, at = range(offset, iofs)));
}

// <negative-set> ::= "[^" <set-items> "]"
// <positive-set> ::= "[" <set-items> "]"
// <set> ::= <positive-set> | <negative-set>
def private re_set(expr : string; offset : int) : MaybeReNode {
    trace("re_set", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    if (at(expr, offset) != '[') {
        return nada();
    }
    var negative = false;
    if (at(expr, offset + 1) == '^') {
        negative = true;
    }
    var oset = re_set_items(expr, negative ? offset + 2 : offset + 1);
    if (oset is nothing) {
        return nada();
    }
    if (at(expr, next(oset)) != ']') {
        unsafe {
            delete oset
        }
        return nada()
    }
    var oval = oset as value
    oval.at.x = offset
    oval.at.y ++
    oval.op = ReOp.Set
    if (negative) {
        set_negative(oval.cset)
    }
    trace_value(oval)
    return oset
}

// <any> ::= "."
def private re_any(expr : string; offset : int) : MaybeReNode {
    trace("re_any", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    if (at(expr, offset) == '.') {
        return maybe(new ReNode(op = ReOp.Any, at = range(offset, offset + 1)))
    }
    return nada()
}

// <eos> ::= "$"
def private re_eos(expr : string; offset : int) : MaybeReNode {
    trace("re_eos", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    if (at(expr, offset) == '$') {
        return maybe(new ReNode(op = ReOp.Eos, at = range(offset, offset + 1)))
    }
    return nada()
}

// <bos> ::= "^"
def private re_bos(expr : string; offset : int) : MaybeReNode {
    trace("re_bos", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    if (at(expr, offset) == '^') {
        return maybe(new ReNode(op = ReOp.Bos, at = range(offset, offset + 1)))
    }
    return nada()
}

// <group> ::= "(" <RE> ")" | "(?:" <RE> ")" | "(?P<name>" <RE> ")"
def private re_group(expr : string; offset : int) : MaybeReNode {
    trace("re_group", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    if (at(expr, offset) != '(') {
        return nada();
    }
    var group_offset = offset + 1
    var non_capturing = false
    var group_name = ""
    var lookahead_op = ReOp.Group  // default — regular group
    // check for (?:...) or (?P<name>...) or (?=...) or (?!...)
    if (!eos(expr, group_offset) && at(expr, group_offset) == '?') {
        if (!eos(expr, group_offset + 1) && at(expr, group_offset + 1) == ':') {
            non_capturing = true
            group_offset += 2
        } elif (!eos(expr, group_offset + 1) && at(expr, group_offset + 1) == '=') {
            lookahead_op = ReOp.Lookahead
            group_offset += 2
        } elif (!eos(expr, group_offset + 1) && at(expr, group_offset + 1) == '!') {
            lookahead_op = ReOp.NegativeLookahead
            group_offset += 2
        } elif (!eos(expr, group_offset + 1) && at(expr, group_offset + 1) == 'P') {
            if (!eos(expr, group_offset + 2) && at(expr, group_offset + 2) == '<') {
                var name_start = group_offset + 3
                var name_end = name_start
                while (!eos(expr, name_end) && at(expr, name_end) != '>') {
                    name_end++
                }
                if (eos(expr, name_end)) {
                    note("syntax error, expecting >", name_end)
                    return nada()
                }
                group_name = slice(expr, name_start, name_end)
                group_offset = name_end + 1
            }
        }
    }
    var ore = re_re(expr, group_offset);
    if (ore is nothing) {
        return nada();
    }
    if (at(expr, next(ore)) != ')') {
        delete ore
        return nada()
    }
    if (lookahead_op != ReOp.Group) {
        var node = new ReNode(op = lookahead_op, subexpr = ore as value, at = range(offset, next(ore) + 1))
        node.index = -1
        return maybe(node)
    }
    var node = new ReNode(op = ReOp.Group, subexpr = ore as value, at = range(offset, next(ore) + 1))
    if (non_capturing) {
        node.index = -1
    }
    if (!empty(group_name)) {
        node.text = group_name
    }
    return maybe(node)
}

// <elementary-RE> ::= <group> | <any> | <eos> | <bos> | <set> | <char>
def private re_elementary(expr : string; offset : int) : MaybeReNode {
    trace("re_elementary", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    var ogr = re_group(expr, offset)
    if (ogr is value) {
        return ogr
    }
    var oany = re_any(expr, offset)
    if (oany is value) {
        return oany
    }
    var oeos = re_eos(expr, offset)
    if (oeos is value) {
        return oeos
    }
    var obos = re_bos(expr, offset)
    if (obos is value) {
        return obos
    }
    var oset = re_set(expr, offset)
    if (oset is value) {
        return oset
    }
    var ochr = re_char(expr, offset)
    if (ochr is value) {
        return ochr
    }
    return nada()
}

def private mk_concat(var left, right : ReNode?) {
    if (left == null) {
        return right
    } elif (left.op == ReOp.Char && right.op == ReOp.Char) {
        // mk_concat(char(x),char(y)) = char(xy)
        left.at.y = right.at.y
        left.text += right.text
        left.textLen += right.textLen
        unsafe {
            delete right
        }
        return left
    } elif (left.op == ReOp.Concat && left.right.op == ReOp.Char && right.op == ReOp.Char) {
        // mk_concat(concat(z,char(x)),char(y)) = concat(z,char(xy))
        var lor = left.right
        left.at.y = right.at.y
        lor.at.y = right.at.y
        lor.text += right.text
        lor.textLen += right.textLen
        unsafe {
            delete right
        }
        return left
    } else {
        return new ReNode(op = ReOp.Concat, left = left, right = right, at = range(left.at.x, right.at.y))
    }
}

def private mk_union(var left, right : ReNode?) : ReNode? {
    if (left.op == ReOp.Union) {
        left.at.y = right.at.y
        if (right.op == ReOp.Union) {// union(union(x),union(y)) = union(xy)
            left.all |> reserve(length(left.all) + length(right.all))
            for (x in right.all) {
                push(left.all, x)
            }
            unsafe {
                delete right
            }
        } else {// union(union(x),y) = union(xy)
            push(left.all, right)
        }
        return left
    } elif (right.op == ReOp.Union) {
        right.at.x = left.at.x
        push(right.all, left)                     // union(x,union(y)) = union(xy)
        return right
    } else {
        return new ReNode(op = ReOp.Union, all <- array<ReNode?>(left, right), at = range(left.at.x, right.at.y))
    }
}

// <RE> ::= <union> | <simple-RE>
// <union> ::= <RE> "|" <simple-RE>
// <simple-RE> ::= <concatenation> | <basic-RE>
// <concatenation> ::= <simple-RE> <basic-RE>
def private  re_re(expr : string; offset : int) : MaybeReNode {
    trace("re_re", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    var cofs = offset
    var last : ReNode?
    while (!eos(expr, cofs)) {
        if (at(expr, cofs) == '|') {
            if (last == null) {
                note("syntax error, nada | union", cofs)
                return nada()
            }
            var oright = re_re(expr, cofs + 1)
            if (oright is nothing) {
                note("syntax error, re | nada", cofs + 1)
                return nada()
            }
            last = mk_union(last, oright as value)
            cofs = last.at.y
        } else {
            var onext = re_basic(expr, cofs)
            if (onext is nothing) {
                return last == null ? nada() : maybe(last)
            }
            last = mk_concat(last, onext as value)
            cofs = last.at.y
        }
    }
    return maybe(last)
}

// <basic-RE> ::= <star> | <plus> | <question> | <repeat> | <elementary-RE>
// <star> ::= <elementary-RE> "*"
// <plus> ::= <elementary-RE> "+"
// <question> ::= <elementary-RE> "?"
// <repeat> ::= <elementary-RE> "{" <n> "}" | <elementary-RE> "{" <n> "," "}" | <elementary-RE> "{" <n> "," <m> "}"
def private re_basic(expr : string; offset : int) : MaybeReNode {
    trace("re_basic", offset)
    if (eos(expr, offset)) {
        return nada()
    }
    var oelem = re_elementary(expr, offset)
    if (oelem is nothing) {
        return nada()
    }
    let ch = at(expr, next(oelem))
    // prevent quantifiers on lookahead assertions
    if (ch == '*' || ch == '+' || ch == '?' || ch == '{') {
        let eop = (oelem as value).op
        if (eop == ReOp.Lookahead || eop == ReOp.NegativeLookahead) {
            note("quantifier on lookahead is not allowed", next(oelem))
            return nada()
        }
    }
    if (ch == '*') {
        var end_ofs = next(oelem) + 1
        var is_lazy = false
        if (!eos(expr, end_ofs) && at(expr, end_ofs) == '?') {
            is_lazy = true
            end_ofs++
        }
        return maybe(new ReNode(op = ReOp.Star, subexpr = oelem as value, lazy = is_lazy, at = range(offset, end_ofs)))
    } elif (ch == '+') {
        var end_ofs = next(oelem) + 1
        var is_lazy = false
        if (!eos(expr, end_ofs) && at(expr, end_ofs) == '?') {
            is_lazy = true
            end_ofs++
        }
        return maybe(new ReNode(op = ReOp.Plus, subexpr = oelem as value, lazy = is_lazy, at = range(offset, end_ofs)))
    } elif (ch == '?') {
        var end_ofs = next(oelem) + 1
        var is_lazy = false
        if (!eos(expr, end_ofs) && at(expr, end_ofs) == '?') {
            is_lazy = true
            end_ofs++
        }
        return maybe(new ReNode(op = ReOp.Question, subexpr = oelem as value, lazy = is_lazy, at = range(offset, end_ofs)))
    } elif (ch == '{') {
        // parse {n}, {n,}, {n,m}
        var rofs = next(oelem) + 1
        var min_val = 0
        var max_val = 0
        var has_comma = false
        var has_max = false
        // parse min
        if (eos(expr, rofs) || !is_digit(at(expr, rofs))) {
            note("syntax error, expecting digit after \{", rofs)
            return nada()
        }
        while (!eos(expr, rofs) && is_digit(at(expr, rofs))) {
            min_val = min_val * 10 + (at(expr, rofs) - '0')
            rofs++
        }
        if (eos(expr, rofs)) {
            note("syntax error, expecting \} or ,", rofs)
            return nada()
        }
        let nch = at(expr, rofs)
        if (nch == ',') {
            has_comma = true
            rofs++
            if (eos(expr, rofs)) {
                note("syntax error, expecting \} or digit", rofs)
                return nada()
            }
            if (is_digit(at(expr, rofs))) {
                has_max = true
                while (!eos(expr, rofs) && is_digit(at(expr, rofs))) {
                    max_val = max_val * 10 + (at(expr, rofs) - '0')
                    rofs++
                }
            }
        }
        if (eos(expr, rofs) || at(expr, rofs) != '}') {
            note("syntax error, expecting \}", rofs)
            return nada()
        }
        rofs++
        if (!has_comma) {
            // {n} - exact
            max_val = min_val
        } elif (!has_max) {
            // {n,} - at least n
            max_val = -1
        }
        // else {n,m} - min_val and max_val already set
        var is_lazy = false
        if (!eos(expr, rofs) && at(expr, rofs) == '?') {
            is_lazy = true
            rofs++
        }
        return maybe(new ReNode(op = ReOp.Repeat, subexpr = oelem as value, min_rep = min_val, max_rep = max_val, lazy = is_lazy, at = range(offset, rofs)))
    }
    return oelem
}

def private re_parse(expr : string) : ReNode? {
    var ore = re_re(expr, 0)
    if (ore is nothing) {
        return null
    }
    let at = next(ore)
    if (at != length(expr)) {
        note("syntax error", at)
        return null
    }
    return ore as value
}

def public visit_top_down(var node : ReNode?; blk : block<(var n : ReNode?) : void>) {
    //! Recursively visits all nodes in the regex tree in top-down order.
    invoke(blk, node)
    for (x in node.all) {
        visit_top_down(x, blk)
    }
    if (node.subexpr != null) {
        visit_top_down(node.subexpr, blk)
    }
    if (node.left != null) {
        visit_top_down(node.left, blk)
    }
    if (node.right != null) {
        visit_top_down(node.right, blk)
    }
}

def private re_assign_next(var re : Regex) {
    var id = 0
    visit_top_down(re.root) $(var node) {
        node.id = id ++
        if (node.op == ReOp.Concat) {
            node.left.next = node.right
            node.right.next = node.next
        } elif (node.op == ReOp.Group) {
            node.subexpr.next = node.next
        } elif (node.op == ReOp.Lookahead || node.op == ReOp.NegativeLookahead) {
            node.subexpr.next = null  // isolated — subexpr must not chain to outer next
        } elif (node.op == ReOp.Union) {
            for (sub in node.all) {
                sub.next = node.next
            }
        }
    }
}

def private re_assign_groups(var re : Regex) {
    push(re.groups, (range(0), ""))
    visit_top_down(re.root) $(var node) {
        if (node.op == ReOp.Group) {
            if (node.index == -1) {
                return  // non-capturing group, skip
            }
            let index = length(re.groups)
            node.index = index
            if (!empty(node.text)) {
                push(re.groups, (range(0), node.text))  // named group
            } else {
                push(re.groups, (range(0), "{index}"))
            }
        }
    }
}

/*
    matching
*/

def private re_assign_match_functions(var re : Regex) {
    visit_top_down(re.root) $(var node) {
        if (node.op == ReOp.Char) {
            if (re.caseInsensitive) {
                if (node.textLen == 1) {
                    node.fun2 = @@re_match2_single_char_ci
                } else {
                    node.fun2 = @@re_match2_char_ci
                }
            } else {
                if (node.textLen == 1) {
                    node.fun2 = @@re_match2_single_char
                } else {
                    node.fun2 = @@re_match2_char
                }
            }
            node.gen2 = @@re_gen2_char
        } elif (node.op == ReOp.Union) {
            node.fun2 = @@re_match2_union
            node.gen2 = @@re_gen2_union
        } elif (node.op == ReOp.Set) {
            if (re.caseInsensitive) {
                case_fold_set(node.cset)
            }
            node.fun2 = @@re_match2_set
            node.gen2 = @@re_gen2_set
        } elif (node.op == ReOp.Any) {
            node.fun2 = @@re_match2_any
            node.gen2 = @@re_gen2_any
        } elif (node.op == ReOp.Eos) {
            node.fun2 = @@re_match2_eos
            node.gen2 = @@re_gen2_eos
        } elif (node.op == ReOp.Concat) {
            node.fun2 = @@re_match2_concat
            node.gen2 = @@re_gen2_concat
        } elif (node.op == ReOp.Plus) {
            if (node.lazy) {
                node.fun2 = @@re_match2_plus_lazy
            } elif (node.subexpr.op == ReOp.Set) {
                node.cset = node.subexpr.cset
                if (re.caseInsensitive) {
                    case_fold_set(node.cset)
                }
                node.fun2 = @@re_match2_plus_set
            } else {
                node.fun2 = @@re_match2_plus
            }
            node.gen2 = @@re_gen2_plus
        } elif (node.op == ReOp.Star) {
            if (node.lazy) {
                node.fun2 = @@re_match2_star_lazy
            } else {
                node.fun2 = @@re_match2_star
            }
            node.gen2 = @@re_gen2_star
        } elif (node.op == ReOp.Question) {
            if (node.lazy) {
                node.fun2 = @@re_match2_question_lazy
            } else {
                node.fun2 = @@re_match2_question
            }
            node.gen2 = @@re_gen2_question
        } elif (node.op == ReOp.Group) {
            node.fun2 = @@re_match2_group
            node.gen2 = @@re_gen2_group
        } elif (node.op == ReOp.Bos) {
            node.fun2 = @@re_match2_bos
            node.gen2 = @@re_gen2_bos
        } elif (node.op == ReOp.Repeat) {
            if (node.lazy) {
                node.fun2 = @@re_match2_repeat_lazy
            } else {
                node.fun2 = @@re_match2_repeat
            }
            node.gen2 = @@re_gen2_repeat
        } elif (node.op == ReOp.WordBoundary) {
            node.fun2 = @@re_match2_word_boundary
            node.gen2 = @@re_gen2_word_boundary
        } elif (node.op == ReOp.NonWordBoundary) {
            node.fun2 = @@re_match2_non_word_boundary
            node.gen2 = @@re_gen2_non_word_boundary
        } elif (node.op == ReOp.Lookahead) {
            node.fun2 = @@re_match2_lookahead
            node.gen2 = @@re_gen2_lookahead
        } elif (node.op == ReOp.NegativeLookahead) {
            node.fun2 = @@re_match2_negative_lookahead
            node.gen2 = @@re_gen2_negative_lookahead
        } else {
            panic("unsupported {node.op}")
        }
    }
}

def private ci_lower(ch : int) : int {
    //! Converts ASCII uppercase to lowercase for case-insensitive matching.
    if (ch >= 'A' && ch <= 'Z') {
        return ch + 32
    }
    return ch
}

def private case_fold_set(var cset : CharSet) {
    //! Expands a charset to include both upper and lower case variants for case-insensitive matching.
    for (ch in range(0, 256)) {
        if (is_char_in_set(ch, cset)) {
            let lo = ci_lower(ch)
            let up = (lo >= 'a' && lo <= 'z') ? lo - 32 : lo
            set_or_char(cset, lo)
            set_or_char(cset, up)
        }
    }
}

[unsafe_deref]
def private re_match2_single_char(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match single char `{node.text}` {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    unsafe {
        if (character_uat(node.text, 0) != int(*str)) {
            return null
        }
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_char(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match char `{node.text}` {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    unsafe {
        if (memcmp(reinterpret<uint8?> node.text, str, node.textLen) != 0) {
            return null
        }
        var tail = str + node.textLen
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            if (log_match_enabled) {
                print("NOP invoke\n")
            }
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_single_char_ci(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match single char ci `{node.text}` {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    unsafe {
        if (ci_lower(character_uat(node.text, 0)) != ci_lower(int(*str))) {
            return null
        }
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_char_ci(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match char ci `{node.text}` {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    unsafe {
        for (i in range(node.textLen)) {
            if (ci_lower(character_uat(node.text, i)) != ci_lower(int(*(str + i)))) {
                return null
            }
        }
        var tail = str + node.textLen
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_union(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match union {str}\n")
    }
    for (sub in node.all) {
        let osub = invoke(sub.fun2, regex, sub, str)
        if (osub != null) {
            node.tail = sub.tail
            return osub
        }
    }
    return null
}

[unsafe_deref]
def private re_match2_set(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 set {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    if (!is_char_in_set(int(*str), node.cset)) {
        return null
    }
    unsafe {
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_any(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 any {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    if (!regex.dotAll && int(*str) == '\n') {
        return null
    }
    unsafe {
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, tail)
        } else {
            return tail
        }
    }
}

[unsafe_deref]
def private re_match2_concat(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 concat {str}\n")
    }
    var left = node.left
    let oleft = invoke(left.fun2, regex, left, str)
    if (oleft == null) {
        return null
    }
    node.tail = node.right.tail
    return oleft
}

[unsafe_deref]
def private re_match2_eos(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match eos {str}\n")
    }
    if (*str != 0u8) {
        return null
    }
    node.tail = str
    var node2 = node.next
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, str)
    } else {
        return str
    }
}

[unsafe_deref]
def private re_match2_question(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match question {str}\n")
    }
    var nsub = node.subexpr
    var tail = invoke(nsub.fun2, regex, nsub, str)
    if (tail == null) {
        tail = str
    }
    node.tail = tail
    var node2 = node.next
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, tail)
    } else {
        return tail
    }
}

[unsafe_deref]
def private re_match2_plus(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 plus {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    var nsub = node.subexpr
    var osym = invoke(nsub.fun2, regex, nsub, str)
    if (osym == null) {
        return null
    }
    var ofs = osym
    var node2 = node.next
    if (node2 != null) {
        while (osym != null) {
            node.tail = osym
            var otail = invoke(node2.fun2, regex, node2, osym)
            if (otail != null) {
                return otail
            }
            osym = invoke(nsub.fun2, regex, nsub, osym)
        }
        return null
    } else {
        while (osym != null) {
            ofs = osym
            osym = invoke(nsub.fun2, regex, nsub, ofs)
        }
        node.tail = ofs
        return ofs
    }
}

[unsafe_deref]
def private re_match2_plus_set(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 plus {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    var nsub = node.subexpr
    if (!is_char_in_set(int(*str), node.cset)) {
        return null
    }
    unsafe {
        var ofs = str + 1
        var osym = ofs
        var node2 = node.next
        if (node2 != null) {
            while (osym != null) {
                node.tail = osym
                var otail = invoke(node2.fun2, regex, node2, osym)
                if (otail != null) {
                    return otail
                }
                if (*osym == 0u8 || !is_char_in_set(int(*osym), node.cset)) {
                    return null
                }
                osym++
            }
            return null
        } else {
            while (osym != null) {
                ofs = osym
                if (*osym == 0u8 || !is_char_in_set(int(*osym), node.cset)) {
                    break
                }
                osym++
            }
            node.tail = ofs
            return ofs
        }
    }
}

[unsafe_deref]
def private re_match2_star(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 plus {str}\n")
    }
    var ofs = str
    var node2 = node.next
    var nsub = node.subexpr
    if (node2 != null) {
        while (ofs != null) {
            node.tail = ofs
            let oany = invoke(node2.fun2, regex, node2, ofs)
            if (oany != null) {
                return oany
            }
            ofs = invoke(nsub.fun2, regex, nsub, ofs)
        }
        return null
    } else {
        var osym = ofs
        while (osym != null) {
            ofs = osym
            osym = invoke(nsub.fun2, regex, nsub, ofs)
        }
        node.tail = ofs
        return ofs
    }
}

[unsafe_deref]
def private re_match2_group(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 group {str}\n")
    }
    var nsub = node.subexpr
    let osub = invoke(nsub.fun2, regex, nsub, str)
    if (osub == null) {
        return null
    }
    if (node.index >= 0) {
        regex.groups[node.index]._0 = range(int(str - regex.match), int(node.subexpr.tail - regex.match))
    }
    return osub
}

[unsafe_deref]
def private re_match2_bos(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match bos {str}\n")
    }
    if (str != regex.match) {
        return null
    }
    node.tail = str
    var node2 = node.next
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, str)
    } else {
        return str
    }
}

[unsafe_deref]
def private re_match2_word_boundary(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match word_boundary {str}\n")
    }
    unsafe {
        var prev_is_word = false
        if (str != regex.match) {
            prev_is_word = is_word_character(int(*(str + (-1))))
        }
        let curr_is_word = (*str != 0u8) && is_word_character(int(*str))
        if (prev_is_word == curr_is_word) {
            return null
        }
        node.tail = str
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, str)
        } else {
            return str
        }
    }
}

[unsafe_deref]
def private re_match2_non_word_boundary(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match non_word_boundary {str}\n")
    }
    unsafe {
        var prev_is_word = false
        if (str != regex.match) {
            prev_is_word = is_word_character(int(*(str + (-1))))
        }
        let curr_is_word = (*str != 0u8) && is_word_character(int(*str))
        if (prev_is_word != curr_is_word) {
            return null
        }
        node.tail = str
        var node2 = node.next
        if (node2 != null) {
            return invoke(node2.fun2, regex, node2, str)
        } else {
            return str
        }
    }
}

[unsafe_deref]
def private re_match2_lookahead(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match lookahead {str}\n")
    }
    var nsub = node.subexpr
    let osub = invoke(nsub.fun2, regex, nsub, str)
    if (osub == null) {
        return null
    }
    // zero-width: don't advance position
    node.tail = str
    var node2 = node.next
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, str)
    } else {
        return str
    }
}

[unsafe_deref]
def private re_match2_negative_lookahead(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match negative_lookahead {str}\n")
    }
    var nsub = node.subexpr
    let osub = invoke(nsub.fun2, regex, nsub, str)
    if (osub != null) {
        return null   // subexpr matched → negative lookahead fails
    }
    // zero-width: don't advance position
    node.tail = str
    var node2 = node.next
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, str)
    } else {
        return str
    }
}

[unsafe_deref]
def private re_match2_repeat(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 repeat \{{node.min_rep},{node.max_rep}\} {str}\n")
    }
    var nsub = node.subexpr
    var ofs = str
    // match minimum required repetitions
    for (i in range(node.min_rep)) {
        if (*ofs == 0u8 && nsub.op != ReOp.Eos && nsub.op != ReOp.Bos && nsub.op != ReOp.WordBoundary && nsub.op != ReOp.NonWordBoundary) {
            return null
        }
        var osym = invoke(nsub.fun2, regex, nsub, ofs)
        if (osym == null) {
            return null
        }
        ofs = osym
    }
    // now try greedy match up to max_rep
    var node2 = node.next
    if (node.max_rep == -1) {
        // unlimited: match as many as possible, then backtrack
        if (node2 != null) {
            // collect all possible extension positions
            var positions : array<uint8 const?>
            push(positions, ofs)
            while (true) {
                var osym = invoke(nsub.fun2, regex, nsub, ofs)
                if (osym == null || osym == ofs) {
                    break
                }
                push(positions, osym)
                ofs = osym
            }
            // try from longest to shortest (greedy)
            var pidx = length(positions) - 1
            while (pidx >= 0) {
                var pos = positions[pidx]
                node.tail = pos
                var otail = invoke(node2.fun2, regex, node2, pos)
                if (otail != null) {
                    return otail
                }
                pidx--
            }
            return null
        } else {
            // no next: consume as many as possible
            while (true) {
                var osym = invoke(nsub.fun2, regex, nsub, ofs)
                if (osym == null || osym == ofs) {
                    break
                }
                ofs = osym
            }
            node.tail = ofs
            return ofs
        }
    } else {
        // bounded: try up to (max_rep - min_rep) more
        let remaining = node.max_rep - node.min_rep
        if (node2 != null) {
            var positions : array<uint8 const?>
            push(positions, ofs)
            for (i in range(remaining)) {
                var osym = invoke(nsub.fun2, regex, nsub, ofs)
                if (osym == null || osym == ofs) {
                    break
                }
                push(positions, osym)
                ofs = osym
            }
            // try from longest to shortest (greedy)
            var pidx = length(positions) - 1
            while (pidx >= 0) {
                var pos = positions[pidx]
                node.tail = pos
                var otail = invoke(node2.fun2, regex, node2, pos)
                if (otail != null) {
                    return otail
                }
                pidx--
            }
            return null
        } else {
            for (i in range(remaining)) {
                var osym = invoke(nsub.fun2, regex, nsub, ofs)
                if (osym == null || osym == ofs) {
                    break
                }
                ofs = osym
            }
            node.tail = ofs
            return ofs
        }
    }
}

/*
    lazy match functions
*/

[unsafe_deref]
def private re_match2_question_lazy(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match question lazy {str}\n")
    }
    // lazy: try zero matches first
    var node2 = node.next
    if (node2 != null) {
        node.tail = str
        var otail = invoke(node2.fun2, regex, node2, str)
        if (otail != null) {
            return otail
        }
    }
    // zero failed (or no next), try one match
    var nsub = node.subexpr
    var tail = invoke(nsub.fun2, regex, nsub, str)
    if (tail == null) {
        if (node2 == null) {
            return str  // zero matches, no next
        }
        return null
    }
    node.tail = tail
    if (node2 != null) {
        return invoke(node2.fun2, regex, node2, tail)
    } else {
        return str  // lazy without next: prefer zero
    }
}

[unsafe_deref]
def private re_match2_star_lazy(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 star lazy {str}\n")
    }
    var ofs = str
    var node2 = node.next
    var nsub = node.subexpr
    if (node2 != null) {
        // lazy: try fewest matches first, then extend
        while (ofs != null) {
            node.tail = ofs
            let oany = invoke(node2.fun2, regex, node2, ofs)
            if (oany != null) {
                return oany
            }
            ofs = invoke(nsub.fun2, regex, nsub, ofs)
        }
        return null
    } else {
        // lazy without next: match zero
        node.tail = str
        return str
    }
}

[unsafe_deref]
def private re_match2_plus_lazy(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 plus lazy {str}\n")
    }
    if (*str == 0u8) {
        return null
    }
    var nsub = node.subexpr
    var osym = invoke(nsub.fun2, regex, nsub, str)
    if (osym == null) {
        return null
    }
    var node2 = node.next
    if (node2 != null) {
        // lazy: try after each match, starting from 1
        while (osym != null) {
            node.tail = osym
            var otail = invoke(node2.fun2, regex, node2, osym)
            if (otail != null) {
                return otail
            }
            osym = invoke(nsub.fun2, regex, nsub, osym)
        }
        return null
    } else {
        // lazy without next: match exactly 1
        node.tail = osym
        return osym
    }
}

[unsafe_deref]
def private re_match2_repeat_lazy(var regex : Regex; var node : ReNode?; str : uint8 const?) : uint8 const? {
    if (log_match_enabled) {
        print("match2 repeat lazy \{{node.min_rep},{node.max_rep}\} {str}\n")
    }
    var nsub = node.subexpr
    var ofs = str
    // match minimum required repetitions
    for (i in range(node.min_rep)) {
        if (*ofs == 0u8 && nsub.op != ReOp.Eos && nsub.op != ReOp.Bos && nsub.op != ReOp.WordBoundary && nsub.op != ReOp.NonWordBoundary) {
            return null
        }
        var osym = invoke(nsub.fun2, regex, nsub, ofs)
        if (osym == null) {
            return null
        }
        ofs = osym
    }
    // now try lazy match: fewest additional matches first
    var node2 = node.next
    if (node.max_rep == -1) {
        // unlimited: try from shortest
        if (node2 != null) {
            while (true) {
                node.tail = ofs
                var otail = invoke(node2.fun2, regex, node2, ofs)
                if (otail != null) {
                    return otail
                }
                var osym = invoke(nsub.fun2, regex, nsub, ofs)
                if (osym == null || osym == ofs) {
                    break
                }
                ofs = osym
            }
            return null
        } else {
            // lazy without next: match minimum
            node.tail = ofs
            return ofs
        }
    } else {
        // bounded: try from shortest to longest
        let remaining = node.max_rep - node.min_rep
        if (node2 != null) {
            for (i in range(remaining + 1)) {
                node.tail = ofs
                var otail = invoke(node2.fun2, regex, node2, ofs)
                if (otail != null) {
                    return otail
                }
                if (i < remaining) {
                    var osym = invoke(nsub.fun2, regex, nsub, ofs)
                    if (osym == null || osym == ofs) {
                        break
                    }
                    ofs = osym
                }
            }
            return null
        } else {
            // lazy without next: match minimum
            node.tail = ofs
            return ofs
        }
    }
}

/*
    early out
*/

def private re_early_out(var cset : CharSet; node : ReNode?) : bool {
    if (node.op == ReOp.Char) {
        let fc = first_character(node.text)
        set_or_char(cset, fc)
        // if case-insensitive, also add the opposite-case first char
        if (fc >= 'A' && fc <= 'Z') {
            set_or_char(cset, fc + 32)
        } elif (fc >= 'a' && fc <= 'z') {
            set_or_char(cset, fc - 32)
        }
        return false
    } elif (node.op == ReOp.Set) {
        set_or_set(cset, node.cset)
        return false
    } elif (node.op == ReOp.Any) {
        for (x in cset) {
            x = 0xffffffffu
        }
        return false
    } elif (node.op == ReOp.Eos) {
        return false
    } elif (node.op == ReOp.Group) {
        return re_early_out(cset, node.subexpr)
    } elif (node.op == ReOp.Plus) {
        return re_early_out(cset, node.subexpr)
    } elif (node.op == ReOp.Star) {
        re_early_out(cset, node.subexpr)
        return true
    } elif (node.op == ReOp.Question) {
        re_early_out(cset, node.subexpr)
        return true
    } elif (node.op == ReOp.Concat) {
        let left = re_early_out(cset, node.left)
        return left ? re_early_out(cset, node.right) : false
    } elif (node.op == ReOp.Union) {
        var any = false
        for (sub in node.all) {
            any = re_early_out(cset, sub) || any
        }
        return any
    } elif (node.op == ReOp.Bos) {
        return true  // zero-width, transparent
    } elif (node.op == ReOp.WordBoundary || node.op == ReOp.NonWordBoundary) {
        return true  // zero-width, transparent
    } elif (node.op == ReOp.Lookahead || node.op == ReOp.NegativeLookahead) {
        return true  // zero-width, transparent
    } elif (node.op == ReOp.Repeat) {
        re_early_out(cset, node.subexpr)
        return node.min_rep == 0
    } else {
        panic("unsupported operation")
        return false
    }
}

/*
    top level API
*/

def is_valid(var re : Regex) {
    //! returns `true` if enumeration compiled correctly
    return re.root != null
}

def regex_compile(var re : Regex; expr : string; case_insensitive : bool = false; dot_all : bool = false) : bool {
    //! Compile regular expression.
    //! When `case_insensitive` is true, matching ignores ASCII case.
    //! When `dot_all` is true, '.' also matches newline characters.
    //! Validity of the compiled expression is checked by `is_valid`.
    re.caseInsensitive = case_insensitive
    re.dotAll = dot_all
    re.root = re_parse(expr)
    if (re.root != null) {
        re_assign_next(re)
        re_assign_groups(re)
        re_assign_match_functions(re)
        re_early_out(re.earlyOut, re.root)
        re.canEarlyOut = !is_set_empty(re.earlyOut)
    }
    return re.root != null
}

def regex_compile(expr : string; case_insensitive : bool = false; dot_all : bool = false) {
    //! Compiles a regular expression string into a Regex object, panics on failure.
    //! When `case_insensitive` is true, matching ignores ASCII case.
    //! When `dot_all` is true, '.' also matches newline characters.
    var re : Regex
    if (!regex_compile(re, expr, case_insensitive, dot_all)) {
        panic("regular expression {expr} did not compile")
    }
    return <- re
}

def regex_compile(var re : Regex) {
    //! Finalizes compilation of a Regex by assigning next pointers and match functions.
    if (re.root != null) {
        re_assign_next(re)
        re_assign_match_functions(re)
    }
    return <- re
}

def regex_match(var regex : Regex; str : string; offset : int = 0) : int {
    //! Returns first match for the regular expression in `str`.
    //! If `offset` is specified, first that many number of symbols will not be matched.
    if (empty(str)) {
        return -1
    }
    if (log_match_enabled) {
        print("matching with `{str}` at {offset}\n")
    }
    unsafe {
        regex.match = reinterpret<uint8?> str
        let mptr = invoke(regex.root.fun2, regex, regex.root, regex.match + offset)
        if (mptr == null) {
            return -1
        }
        return int(mptr - regex.match)
    }
}

def regex_group(regex : Regex; index : int; match : string) {
    //! Returns string for the given group index and match result.
    let sub_range = regex.groups[index]._0
    return slice(match, sub_range.x, sub_range.y)
}

[unsafe_deref]
def regex_foreach(var regex : Regex; str : string; blk : block<(at : range) : bool>) {
    //! Iterates through all matches for the given regular expression in `str`.
    if (empty(str)) {
        return
    }
    unsafe {
        regex.match = reinterpret<uint8 const?> str
        var root = regex.root
        var pstr = reinterpret<uint8 const?> str
        var cstr = pstr
        if (regex.canEarlyOut) {
            // lets try if it helps
            while (true) {
                let Ch = int(*cstr)
                if (Ch == 0) {
                    break
                }
                if (is_char_in_set(Ch, regex.earlyOut)) {
                    let om = invoke(root.fun2, regex, root, cstr)
                    if (om != null) {
                        if (!invoke(blk, range(int(cstr - pstr), int(om - pstr)))) {
                            break
                        }
                        if (om == cstr) {
                            cstr++
                        } else {
                            cstr = om
                        }
                    } else {
                        cstr++
                    }
                } else {
                    cstr++
                }
            }
        } else {
            while (int(*cstr) != 0) {
                let om = invoke(root.fun2, regex, root, cstr)
                if (om != null) {
                    if (!invoke(blk, range(int(cstr - pstr), int(om - pstr)))) {
                        break
                    }
                    if (om == cstr) {
                        cstr++
                    } else {
                        cstr = om
                    }
                } else {
                    cstr ++
                }
            }
        }
    }
}

[unsafe_deref]
def regex_replace(var regex : Regex; str : string; blk : block<(at : string) : string>) {
    //! Iterates through all matches for the given regular expression in `str`.
    if (empty(str)) {
        return ""
    }
    unsafe {
        return build_string() $(writer) {
            regex.match = reinterpret<uint8 const?> str
            var root = regex.root
            var pstr = reinterpret<uint8 const?> str
            var cstr = pstr
            if (regex.canEarlyOut) {
                // lets try if it helps
                while (true) {
                    let Ch = int(*cstr)
                    if (Ch == 0) {
                        break
                    }
                    if (is_char_in_set(Ch, regex.earlyOut)) {
                        let om = invoke(root.fun2, regex, root, cstr)
                        if (om != null) {
                            let repl = invoke(blk, slice(str, int(cstr - pstr), int(om - pstr)))
                            writer |> write(repl)
                            if (om == cstr) {
                                writer |> write_char(Ch)
                                cstr++
                            } else {
                                cstr = om
                            }
                        } else {
                            writer |> write_char(Ch)
                            cstr++
                        }
                    } else {
                        writer |> write_char(Ch)
                        cstr++
                    }
                }
            } else {
                while (int(*cstr) != 0) {
                    let om = invoke(root.fun2, regex, root, cstr)
                    if (om != null) {
                        let repl = invoke(blk, slice(str, int(cstr - pstr), int(om - pstr)))
                        writer |> write(repl)
                        if (om == cstr) {
                            writer |> write_char(int(*cstr))
                            cstr++
                        } else {
                            cstr = om
                        }
                    } else {
                        writer |> write_char(int(*cstr))
                        cstr ++
                    }
                }
            }
        }
    }
}

def private expand_replacement(var regex : Regex; str : string; replacement : string; match_start, match_end : int) : string {
    //! Expands replacement template with group references: $0 or $& for whole match,
    //! $1-$9 for numbered groups, ${name} for named groups, $$ for literal $.
    return build_string() $(writer) {
        let rlen = length(replacement)
        peek_data(replacement) $(rdata) {
            var i = 0
            while (i < rlen) {
                let ch = int(rdata[i])
                if (ch == '$' && i + 1 < rlen) {
                    let nch = int(rdata[i + 1])
                    if (nch == '$') {
                        writer |> write_char('$')
                        i += 2
                    } elif (nch == '&' || nch == '0') {
                        writer |> write(slice(str, match_start, match_end))
                        i += 2
                    } elif (nch >= '1' && nch <= '9') {
                        let group_num = nch - '0'
                        if (group_num < length(regex.groups)) {
                            let grng = regex.groups[group_num]._0
                            writer |> write(slice(str, grng.x, grng.y))
                        }
                        i += 2
                    } elif (nch == '{') {
                        let close = find(replacement, "}", i + 2)
                        if (close != -1) {
                            let name = slice(replacement, i + 2, close)
                            // try numeric group reference first (${0}, ${1}, ...)
                            var is_numeric = !empty(name)
                            for (nc in name) {
                                if (nc < '0' || nc > '9') {
                                    is_numeric = false
                                    break
                                }
                            }
                            if (is_numeric) {
                                var group_num = 0
                                for (nc in name) {
                                    group_num = group_num * 10 + (nc - '0')
                                }
                                if (group_num == 0) {
                                    writer |> write(slice(str, match_start, match_end))
                                } elif (group_num < length(regex.groups)) {
                                    let grng = regex.groups[group_num]._0
                                    writer |> write(slice(str, grng.x, grng.y))
                                }
                            } else {
                                let grp = regex_group_by_name(regex, name, str)
                                writer |> write(grp)
                            }
                            i = close + 1
                        } else {
                            writer |> write_char(ch)
                            i++
                        }
                    } else {
                        writer |> write_char(ch)
                        i++
                    }
                } else {
                    writer |> write_char(ch)
                    i++
                }
            }
        }
    }
}

[unsafe_deref]
def regex_replace(var regex : Regex; str : string; replacement : string) : string {
    //! Replaces all matches of the regular expression in `str` with `replacement`.
    //! Supports group references in the replacement string:
    //! `$0` or `$&` for whole match, `$1`-`$9` for numbered groups,
    //! `${name}` for named groups, `$$` for literal `$`.
    if (empty(str)) {
        return ""
    }
    unsafe {
        return build_string() $(writer) {
            regex.match = reinterpret<uint8 const?> str
            var root = regex.root
            var pstr = reinterpret<uint8 const?> str
            var cstr = pstr
            if (regex.canEarlyOut) {
                while (true) {
                    let Ch = int(*cstr)
                    if (Ch == 0) {
                        break
                    }
                    if (is_char_in_set(Ch, regex.earlyOut)) {
                        let om = invoke(root.fun2, regex, root, cstr)
                        if (om != null) {
                            let match_start = int(cstr - pstr)
                            let match_end = int(om - pstr)
                            let repl = expand_replacement(regex, str, replacement, match_start, match_end)
                            writer |> write(repl)
                            if (om == cstr) {
                                writer |> write_char(Ch)
                                cstr++
                            } else {
                                cstr = om
                            }
                        } else {
                            writer |> write_char(Ch)
                            cstr++
                        }
                    } else {
                        writer |> write_char(Ch)
                        cstr++
                    }
                }
            } else {
                while (int(*cstr) != 0) {
                    let om = invoke(root.fun2, regex, root, cstr)
                    if (om != null) {
                        let match_start = int(cstr - pstr)
                        let match_end = int(om - pstr)
                        let repl = expand_replacement(regex, str, replacement, match_start, match_end)
                        writer |> write(repl)
                        if (om == cstr) {
                            writer |> write_char(int(*cstr))
                            cstr++
                        } else {
                            cstr = om
                        }
                    } else {
                        writer |> write_char(int(*cstr))
                        cstr++
                    }
                }
            }
        }
    }
}

[unsafe_deref]
def regex_search(var regex : Regex; str : string; offset : int = 0) : int2 {
    //! Searches for the first occurrence of the regular expression in `str`.
    //! Returns int2 with x = start position and y = end position, or int2(-1,-1) if not found.
    //! Unlike `regex_match` which only matches at the given offset, this scans the entire string.
    if (empty(str)) {
        return int2(-1, -1)
    }
    unsafe {
        regex.match = reinterpret<uint8 const?> str
        var root = regex.root
        var pstr = reinterpret<uint8 const?> str
        var cstr = pstr + offset
        if (regex.canEarlyOut) {
            while (true) {
                let Ch = int(*cstr)
                if (Ch == 0) {
                    break
                }
                if (is_char_in_set(Ch, regex.earlyOut)) {
                    let om = invoke(root.fun2, regex, root, cstr)
                    if (om != null) {
                        return int2(int(cstr - pstr), int(om - pstr))
                    }
                }
                cstr++
            }
        } else {
            while (int(*cstr) != 0) {
                let om = invoke(root.fun2, regex, root, cstr)
                if (om != null) {
                    return int2(int(cstr - pstr), int(om - pstr))
                }
                cstr++
            }
        }
        return int2(-1, -1)
    }
}

def regex_split(var regex : Regex; str : string) : array<string> {
    //! Splits `str` by matches of the regular expression. Returns array of substrings between matches.
    var result : array<string>
    if (empty(str)) {
        return <- result
    }
    var last_end = 0
    regex_foreach(regex, str) $(at : range) {
        push(result, slice(str, last_end, at.x))
        last_end = at.y
        return true
    }
    push(result, slice(str, last_end))
    return <- result
}

def regex_match_all(var regex : Regex; str : string) : array<range> {
    //! Returns an array of all match ranges for the regular expression in `str`.
    var result : array<range>
    regex_foreach(regex, str) $(at : range) {
        push(result, at)
        return true
    }
    return <- result
}

def regex_group_by_name(regex : Regex; name : string; str : string) : string {
    //! Returns the matched string for the given named group.
    //! Returns empty string if the group name is not found.
    for (g in regex.groups) {
        if (g._1 == name) {
            return slice(str, g._0.x, g._0.y)
        }
    }
    return ""
}

def operator [] (regex : Regex; index : int) : range {
    //! Returns the match range for the given group index.
    return regex.groups[index]._0
}

def operator [] (regex : Regex; name : string) : range {
    //! Returns the match range for the given named group.
    //! Returns ``range(0,0)`` if not found.
    for (g in regex.groups) {
        if (g._1 == name) {
            return g._0
        }
    }
    return range(0, 0)
}

/*
    printer
*/

def regex_debug(regex : Regex) {
    //! Prints regular expression and its related information in human readable form.
    debug_re(regex.root, 1)
    print("\n")
    if (regex.canEarlyOut) {
        print("early out: ")
        debug_set(regex.earlyOut)
        print("\n")
    }
    if (length(regex.groups) != 0) {
        print("groups:")
        for (g in regex.groups) {
            print("\t{g._1}\n")
        }
    }
}

def debug_set(cset : CharSet) {
    //! Prints character set in human readable form.
    for (x in range(256)) {
        if ((cset[x >> 5] & uint(1 << (x & 31))) != 0u) {
            if (x >= 32 && x <= 127) {
                print("{to_char(x)}")
            } else {
                print("\\{x}")
            }
        }
    }
}

def private debug_re(node : ReNode?; tab : int) {
    print("(")
    if (node != null) {
        print("#{node.id}")
        if (node.next != null) {
            print("->{node.next.id}")
        } else {
            print("->!")
        }
        print(" ")
    }
    if (node == null) {
        print("null)")
    } elif (node.op == ReOp.Char) {
        print("Char `{node.text}`)")
    } elif (node.op == ReOp.Set) {
        print("Set ")
        debug_set(node.cset)
        print(")")
    } elif (node.op == ReOp.Any) {
        print("Any)")
    } elif (node.op == ReOp.Eos) {
        print("Eos)")
    } elif (node.op == ReOp.Group) {
        if (node.index == -1) {
            print("NonCapGroup ")
        } elif (!empty(node.text)) {
            print("Group<{node.text}> ")
        } else {
            print("Group ")
        }
        debug_re(node.subexpr, tab + 1)
        print(")")
    } elif (node.op == ReOp.Plus) {
        print(node.lazy ? "Plus? " : "Plus ")
        debug_re(node.subexpr, tab)
        print(")")
    } elif (node.op == ReOp.Star) {
        print(node.lazy ? "Star? " : "Star ")
        debug_re(node.subexpr, tab)
        print(")")
    } elif (node.op == ReOp.Question) {
        print(node.lazy ? "Question? " : "Question ")
        debug_re(node.subexpr, tab)
        print(")")
    } elif (node.op == ReOp.Concat) {
        print("Contact")
        print("\n{repeat("\t",tab)}")
        debug_re(node.left, tab + 1)
        print("\n{repeat("\t",tab)}")
        debug_re(node.right, tab + 1)
        print("\n{repeat("\t",tab-1)})")
    } elif (node.op == ReOp.Union) {
        print("Union\n")
        for (sub in node.all) {
            print("{repeat("\t",tab)}")
            debug_re(sub, tab + 1)
            print("\n")
        }
        print("{repeat("\t",tab-1)})")
    } elif (node.op == ReOp.Bos) {
        print("Bos)")
    } elif (node.op == ReOp.Repeat) {
        print(node.lazy ? "Repeat? \{{node.min_rep},{node.max_rep}\} " : "Repeat \{{node.min_rep},{node.max_rep}\} ")
        debug_re(node.subexpr, tab)
        print(")")
    } elif (node.op == ReOp.WordBoundary) {
        print("WordBoundary)")
    } elif (node.op == ReOp.NonWordBoundary) {
        print("NonWordBoundary)")
    } elif (node.op == ReOp.Lookahead) {
        print("Lookahead ")
        debug_re(node.subexpr, tab + 1)
        print(")")
    } elif (node.op == ReOp.NegativeLookahead) {
        print("NegLookahead ")
        debug_re(node.subexpr, tab + 1)
        print(")")
    } else {
        panic("unsupported op")
    }
}



def private rnd_set_character(cset : uint[8]; var rnd : ReGenRandom) : int {
    let total = set_total(cset)
    var idx = 0u
    next(rnd, idx)
    idx = idx % total
    return set_element(int(idx), cset)
}

def private re_gen2_fail(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen fail\n")
    }
    assert(false, "not running generated regex")
}

def private re_gen2_char(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen char `{node.text}`\n")
    }
    str |> write(node.text)
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_union(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen union\n")
    }
    var nidx = 0u
    next(rnd, nidx)
    nidx = nidx % uint(length(node.all))
    var sub = node.all[nidx]
    invoke(sub.gen2, sub, rnd, str)
}

def private re_gen2_set(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen set {node.cset}\n")
    }
    str |> write_char(rnd_set_character(node.cset, rnd))
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_any(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen any\n")
    }
    var ch : uint
    next(rnd, ch)
    ch = (ch % 254u) + 1u  // 1..254, avoids 0 and newline (10)
    if (ch >= 10u) {
        ch++  // skip newline character
    }
    str |> write_char(int(ch))
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_concat(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen concat\n")
    }
    var left = node.left
    invoke(left.gen2, left, rnd, str)
}

def private re_gen2_eos(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen eos\n")
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_question(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen question\n")
    }
    var a = 0u
    next(rnd, a)
    if ((a & 1u) != 0u) {// zero or one?
        var nsub = node.subexpr
        if (nsub != null) {
            invoke(nsub.gen2, nsub, rnd, str)
        }
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_plus(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen plus\n")
    }
    var cnt = 0u
    next(rnd, cnt)
    cnt = (cnt % re_gen_rep_limit) + 1u
    var nsub = node.subexpr
    if (nsub != null) {
        for (i in urange(cnt)) {
            invoke(nsub.gen2, nsub, rnd, str)
        }
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_star(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen star\n")
    }
    var cnt = 0u
    next(rnd, cnt)
    cnt = cnt % re_gen_rep_limit
    var nsub = node.subexpr
    if (nsub != null) {
        for (i in urange(cnt)) {
            invoke(nsub.gen2, nsub, rnd, str)
        }
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_group(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("match2 group\n")
    }
    var nsub = node.subexpr
    if (nsub != null) {
        invoke(nsub.gen2, nsub, rnd, str)
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_bos(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen bos\n")
    }
    // zero-width, nothing to emit
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_repeat(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen repeat \{{node.min_rep},{node.max_rep}\}\n")
    }
    var cnt : uint
    if (node.max_rep == -1) {
        next(rnd, cnt)
        cnt = (cnt % re_gen_rep_limit) + uint(node.min_rep)
    } elif (node.max_rep == node.min_rep) {
        cnt = uint(node.min_rep)
    } else {
        next(rnd, cnt)
        cnt = uint(node.min_rep) + (cnt % uint(node.max_rep - node.min_rep + 1))
    }
    var nsub = node.subexpr
    if (nsub != null) {
        for (i in urange(cnt)) {
            invoke(nsub.gen2, nsub, rnd, str)
        }
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_word_boundary(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen word_boundary\n")
    }
    // zero-width, nothing to emit
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_non_word_boundary(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen non_word_boundary\n")
    }
    // zero-width, nothing to emit
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_lookahead(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen lookahead\n")
    }
    // zero-width, nothing to emit — subexpr contributes to continuation
    var nsub = node.subexpr
    if (nsub != null) {
        invoke(nsub.gen2, nsub, rnd, str)
    }
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def private re_gen2_negative_lookahead(var node : ReNode?; var rnd : ReGenRandom; var str : StringBuilderWriter) : void {
    if (log_gen_enabled) {
        print("gen negative_lookahead\n")
    }
    // cannot meaningfully generate for negative lookahead — skip subexpr, chain to next
    var node2 = node.next
    if (node2 != null) {
        invoke(node2.gen2, node2, rnd, str)
    }
}

def public re_gen(var re : Regex; var rnd : ReGenRandom) : string {
    //! Generates a random string that matches the compiled regular expression.
    return build_string() $(writer) {
        invoke(re.root.gen2, re.root, rnd, writer)
    }
}
