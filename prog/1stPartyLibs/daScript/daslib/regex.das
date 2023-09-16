options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module regex shared public

require daslib/strings_boost

enum ReOp
    //! Type of regular expression operation.
    Char
    Set
    Any
    Eos
    Group
    Plus
    Star
    Question
    Concat
    Union

typedef
    CharSet = uint[8]   //! Bit array which represents an 8-bit character set.
    ReGenRandom = iterator<uint>

struct ReNode
    //! Single node in regular expression parsing tree.
    op      : ReOp
    id      : int
    [[do_not_convert]] fun2 : function< (var regex:Regex; var node:ReNode?; str:uint8?) : uint8? >
    [[do_not_convert]] gen2 : function< ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void >
    at      : range
    text    : string
    textLen : int
    all     : array<ReNode?>
    left    : ReNode?
    right   : ReNode?
    subexpr : ReNode?
    [[do_not_delete,do_not_convert]] next : ReNode?
    cset    : CharSet
    index   : int
    [[do_not_convert]] tail : uint8?

struct Regex
    //! Regular expression.
    root        : ReNode?
    [[do_not_convert]] match : uint8?
    groups      : array<tuple<range;string>>
    earlyOut    : CharSet
    canEarlyOut : bool

variant MaybeReNode
    //! Single regular expression node or nothing.
    value   : ReNode?
    nothing : void?

/*
    Parser
*/

let private log_parse_enabled = false
let private log_match_enabled = false
let private range_check_enabled = false
let private log_gen_enabled = false

var private trace_tab = 0
var private re_gen_rep_limit = 8u

def public re_gen_get_rep_limit
    return re_gen_rep_limit

def private trace(msg:string; offset:int)
    static_if log_parse_enabled
        print("{repeat(" ",trace_tab)}{offset}: {msg}\n")
        trace_tab ++

def private note(msg:string; offset:int)
    static_if log_parse_enabled
        print("{repeat(" ",trace_tab)}{offset}: {msg}\n")

def private trace_backtrack()
    static_if log_parse_enabled
        trace_tab --
        print("{repeat(" ",trace_tab)}backtrack\n")

def private trace_value(value : ReNode?)
    static_if log_parse_enabled
        if value==null
            panic("trace_value null?")
        trace_tab --
        print("{repeat(" ",trace_tab)}matched {value.op}, next {value.at.y}\n")

def private nada()
    trace_backtrack()
    return [[MaybeReNode nothing=null]]

def private maybe( value : ReNode? )
    trace_value(value)
    return [[MaybeReNode value=value]]

def private eos(expr:string; offset:int)
    return offset >= length(expr)

def private at(expr:string; offset:int) : int
    static_if range_check_enabled
        let len = length(expr)
        if offset<len
            return character_at(expr, offset)
        elif offset==len
            return 0
        else
            stackwalk()
            panic("out of range {offset} of {len}")
            return 0
    else
        unsafe
            return character_uat(expr, offset)

def private next(re:MaybeReNode)
    if !(re is value)
        stackwalk()
        panic("expecting value")
    return (re as value).at.y

def private is_set_empty(cset:CharSet)
    for x in cset
        if x != 0u
            return false
    return true

def private set_or_char(var cset:CharSet; ch:int)
    cset[ch>>5] |= uint(1 << (ch & 31))

def private set_or_range(var cset:CharSet; bits:range)
    for ch in range(bits.x,bits.y+1)
        cset[ch>>5] |= uint(1 << (ch & 31))

def private set_invert(var cset:CharSet)
    for x in cset
        x ^= 0xffffffff

def private set_or_set(var cset:CharSet; eset:CharSet)
    for x,y in cset,eset
        x |= y

def private set_negative(var cset:CharSet)
    for x in cset
        x = ~x

def private set_meta(var cset:CharSet; che:int)
    if che=='w'
        set_or_range(cset,range('a','z'))
        set_or_range(cset,range('A','Z'))
        set_or_range(cset,range('0','9'))
        set_or_char(cset,'_')
    elif che=='W'
        var eset:CharSet
        set_meta(eset,'w')
        set_or_set(cset,eset)
    elif che=='s'
        set_or_char(cset,' ')
        set_or_char(cset,'\t')
    elif che=='S'
        var eset:CharSet
        set_meta(eset,'s')
        set_or_set(cset,eset)
    elif che=='d'
        set_or_range(cset,range('0','9'))
    elif che=='D'
        var eset:CharSet
        set_meta(eset,'d')
        set_or_set(cset,eset)
    else
        set_or_char(cset,che)

let private meta = "\\+-*.()[]|^"
let private meta_set = "wWsSdD"

def private is_meta_character(ch:int) : bool
    return find(meta,ch) != -1

def private is_set_character(ch:int) : bool
    return find(meta_set,ch) != -1

def private from_hex ( ch : int )
    if ch>='0' && ch<='9'
        return ch-'0'
    if ch>='a' && ch<='f'
        return ch-'a'+10
    if ch>='A' && ch<='F'
        return ch-'A'+10
    panic("from_hex")
    return 0

def private from_hex ( ch1, ch2 )
    return from_hex(ch1)*16+from_hex(ch2)

def private string_from_hex ( ch1 : int )
    return to_char(from_hex(ch1))

def private string_from_hex ( ch1,ch2 : int )
    return to_char(from_hex(ch1,ch2))

// <char>	::=	any non metacharacter | "\" metacharacter
def private re_char (expr : string; offset:int) : MaybeReNode
    trace("re_char",offset)
    if eos(expr,offset)
        return nada()
    let ch = at(expr,offset)
    if ch == '\\'
        let ech = at(expr,offset+1)
        if ech==0
            note("syntax error, expecting meta", offset+1)
            return nada()
        if ech=='x'
            let len = length(expr)
            if offset+2 >= len
                note("syntax error, expecting hex on hex1, too long {offset+1} vs {len}", offset+2)
                return nada()
            let hex1 = at(expr,offset+2)
            if !is_hex(hex1)
                note("syntax error, expecting hex on hex1, {to_char(hex1)} is not a hex", offset+2)
                return nada()
            if offset+2<len
                let hex2 = at(expr,offset+3)
                if is_hex(hex2)
                    note("hex \\{to_char(hex1)}{to_char(hex2)} {uint(from_hex(hex1,hex2))}", offset)
                    return maybe(new [[ReNode op=ReOp Char, text=string_from_hex(hex1,hex2), textLen=1, at=range(offset,offset+3)]])
            note("hex \\{to_char(hex1)} {uint(from_hex(hex1))}", offset)
            return maybe(new [[ReNode op=ReOp Char, text=string_from_hex(hex1), textLen=1, at=range(offset,offset+2)]])
        if is_set_character(ech)
            var cset : CharSet
            set_meta(cset,ech)
            return maybe(new [[ReNode op=ReOp Set, cset=cset, at=range(offset,offset+2)]])
        return maybe(new [[ReNode op=ReOp Char, text=to_char(ech), textLen=1, at=range(offset,offset+2)]])
    elif is_meta_character(ch)
        return nada()
    return maybe(new [[ReNode op=ReOp Char, text=to_char(ch), textLen=1, at=range(offset,offset+1)]])

// <set-items>	::=	<set-item> | <set-item> <set-items>
// <set-item>	::=	<range> | <char>
// <range>	::=	<char> "-" <char>
// <char>	::=	any non metacharacter | "\" metacharacter
def private re_set_items(expr:string; offset:int) : MaybeReNode
    trace("re_set_items",offset)
    if eos(expr,offset)
        return nada()
    var cset : CharSet
    var iofs = offset
    var prev_char = -1
    var next_range = false
    while !eos(expr,iofs)
        let ch = at(expr,iofs)
        var next_char = -1
        if ch == ']'
            note("set - break - ]",iofs)
            break
        elif ch=='\\'
            let che = at(expr,iofs+1)
            if che==0
                note("syntax error, expecting meta", iofs)
                return nada()
            if che=='x'
                let len = length(expr)
                if iofs+2 >= len
                    note("syntax error, expecting hex on hex1, too long {iofs+1} vs {len}", iofs+2)
                    return nada()
                let hex1 = at(expr,iofs+2)
                if !is_hex(hex1)
                    note("syntax error, expecting hex on hex1, {to_char(hex1)} is not a hex", iofs+2)
                    return nada()
                if iofs+2<len
                    let hex2 = at(expr,iofs+3)
                    if is_hex(hex2)
                        next_char = from_hex(hex1,hex2)
                        note("set hex \\{to_char(hex1)}{to_char(hex2)} {uint(from_hex(hex1,hex2))}", iofs)
                        iofs += 4
                    else
                        next_char = from_hex(hex1)
                        note("set hex \\{to_char(hex1)} {uint(from_hex(hex1))}", iofs)
                        iofs += 3
                else
                    next_char = from_hex(hex1)
                    note("set hex \\{to_char(hex1)} {uint(from_hex(hex1))}", offset)
                    iofs += 3
            else
                if next_range
                    note("syntax error, expecting range, got meta", iofs)
                    return nada()
                if is_set_character(che)
                    next_char = -1
                    set_meta(cset,che)
                    note("set - meta {to_char(che)}",iofs)
                else
                    next_char = che
                    note("set - {to_char(che)}",iofs)
                iofs += 2
        elif ch=='-'
            if prev_char == -1
                note("syntax error, expecting range, got -", iofs)
                return nada()
            next_range = true
            iofs += 1
        else
            next_char = ch
            note("set - character {uint(ch)} {ch} {uint(ch)}",iofs)
            iofs += 1
        note("next_char {uint(next_char)} prev_char {uint(prev_char)} next_range {next_range}", iofs)
        if next_char != -1
            if next_range
                note("set - range {uint(prev_char)} {uint(next_char)}",iofs)
                set_or_range(cset,range(prev_char,next_char))
                next_range = false
                prev_char = -1
            else
                note("set - char {uint(next_char)}",iofs)
                set_or_char(cset,next_char)
                prev_char = next_char
        elif !next_range
            note("syntax error???",iofs)
            return nada()
    return maybe(new [[ReNode op=ReOp Set, cset=cset, at=range(offset,iofs)]])

// <negative-set>	::=	"[^" <set-items> "]"
// <positive-set>	::=	"[" <set-items> "]"
// <set>	::=	<positive-set> | <negative-set>
def private re_set(expr:string; offset:int) : MaybeReNode
    trace("re_set",offset)
    if eos(expr,offset)
        return nada()
    if at(expr,offset)!='['
        return nada()
    var negative = false
    if at(expr,offset+1)=='^'
        negative = true
    var oset = re_set_items(expr,negative ? offset+2 : offset+1)
    if oset is nothing
        return nada()
    if at(expr,next(oset)) != ']'
        unsafe
            delete oset
        return nada()
    var oval = oset as value
    oval.at.x = offset
    oval.at.y ++
    oval.op = ReOp Set
    if negative
        set_negative(oval.cset)
    trace_value(oval)
    return oset

// <any>	::=	"."
def private re_any(expr:string; offset:int) : MaybeReNode
    trace("re_any",offset)
    if eos(expr,offset)
        return nada()
    if at(expr,offset)=='.'
        return maybe(new [[ReNode op=ReOp Any, at=range(offset,offset+1)]])
    return nada()

// <eos>	::=	"$"
def private re_eos(expr:string; offset:int) : MaybeReNode
    trace("re_eos",offset)
    if eos(expr,offset)
        return nada()
    if at(expr,offset)=='$'
        return maybe(new [[ReNode op=ReOp Eos, at=range(offset,offset+1)]])
    return nada()

// <group>	::=	"(" <RE> ")"
def private re_group(expr:string; offset:int) : MaybeReNode
    trace("re_group",offset)
    if eos(expr,offset)
        return nada()
    if at(expr,offset)!='('
        return nada()
    var ore = re_re(expr,offset+1)
    if ore is nothing
        return nada()
    if at(expr,next(ore))!=')'
        delete ore
        return nada()
    return maybe(new [[ReNode op=ReOp Group, subexpr=ore as value, at=range(offset,next(ore)+1)]])

// <elementary-RE>	::=	<group> | <any> | <eos> | <set> | <char>
def private re_elementary(expr:string; offset:int) : MaybeReNode
    trace("re_elementary",offset)
    if eos(expr,offset)
        return nada()
    var ogr = re_group(expr,offset)
    if ogr is value
        return ogr
    var oany = re_any(expr,offset)
    if oany is value
        return oany
    var oeos = re_eos(expr,offset)
    if oeos is value
        return oeos
    var oset = re_set(expr,offset)
    if oset is value
        return oset
    var ochr = re_char(expr,offset)
    if ochr is value
        return ochr
    return nada()

def private mk_concat(var left,right:ReNode?)
    if left == null
        return right
    elif left.op==ReOp Char && right.op==ReOp Char
        // mk_concat(char(x),char(y)) = char(xy)
        left.at.y = right.at.y
        left.text += right.text
        left.textLen += right.textLen
        unsafe
            delete right
        return left
    elif left.op==ReOp Concat && left.right.op==ReOp Char && right.op==ReOp Char
        // mk_concat(concat(z,char(x)),char(y)) = concat(z,char(xy))
        var lor = left.right
        left.at.y = right.at.y
        lor.at.y = right.at.y
        lor.text += right.text
        lor.textLen += right.textLen
        unsafe
            delete right
        return left
    else
        return new [[ReNode op=ReOp Concat, left = left, right = right, at=range(left.at.x,right.at.y)]]

def private mk_union(var left,right:ReNode?) : ReNode?
    if left.op==ReOp Union
        left.at.y = right.at.y
        if right.op==ReOp Union                    // union(union(x),union(y)) = union(xy)
            for x in right.all
                push(left.all,x)
            unsafe
                delete right
        else                                       // union(union(x),y) = union(xy)
            push(left.all,right)
        return left
    elif right.op==ReOp Union
        right.at.x = left.at.x
        push(right.all,left)                      // union(x,union(y)) = union(xy)
        return right
    else
        return new [[ReNode op=ReOp Union, all <- [{ReNode? left; right}], at=range(left.at.x,right.at.y)]]

// <RE>	::=	<union> | <simple-RE>
// <union>	::=	<RE> "|" <simple-RE>
// <simple-RE>	::=	<concatenation> | <basic-RE>
// <concatenation>	::=	<simple-RE> <basic-RE>
def private  re_re(expr:string; offset:int) : MaybeReNode
    trace("re_re",offset)
    if eos(expr,offset)
        return nada()
    var cofs = offset
    var last : ReNode?
    while !eos(expr,cofs)
        if at(expr,cofs)=='|'
            if last == null
                note("syntax error, nada | union", cofs)
                return nada()
            var oright = re_re(expr,cofs+1)
            if oright is nothing
                note("syntax error, re | nada", cofs+1)
                return nada()
            last = mk_union(last,oright as value)
            cofs = last.at.y
        else
            var onext = re_basic(expr, cofs)
            if onext is nothing
                return last==null? nada() : maybe(last)
            last = mk_concat(last,onext as value)
            cofs = last.at.y
    return maybe(last)

// <basic-RE>	::=	<star> | <plus> | <elementary-RE>
// <star>	::=	<elementary-RE> "*"
// <plus>	::=	<elementary-RE> "+"
def private re_basic(expr:string; offset:int) : MaybeReNode
    trace("re_basic",offset)
    if eos(expr,offset)
        return nada()
    var oelem = re_elementary(expr,offset)
    if oelem is nothing
        return nada()
    let ch = at(expr,next(oelem))
    if ch=='*'
        return maybe(new [[ReNode op=ReOp Star, subexpr=oelem as value, at=range(offset,next(oelem)+1)]])
    elif ch=='+'
        return maybe(new [[ReNode op=ReOp Plus, subexpr=oelem as value, at=range(offset,next(oelem)+1)]])
    elif ch=='?'
        return maybe(new [[ReNode op=ReOp Question, subexpr=oelem as value, at=range(offset,next(oelem)+1)]])
    return oelem

def private re_parse ( expr:string ) : ReNode?
    var ore = re_re(expr, 0)
    if ore is nothing
        return null
    let at = next(ore)
    if at != length(expr)
        note("syntax error",at)
        return null
    return ore as value

def public visit_top_down ( var node:ReNode?; blk : block<(var n:ReNode?):void> )
    invoke(blk,node)
    for x in node.all
        visit_top_down(x, blk)
    if node.subexpr!=null
        visit_top_down(node.subexpr, blk)
    if node.left!=null
        visit_top_down(node.left, blk)
    if node.right!=null
        visit_top_down(node.right, blk)

def private re_assign_next ( var re:Regex )
    var id = 0
    visit_top_down(re.root) <| $ ( var node )
        node.id = id ++
        if node.op == ReOp Concat
            node.left.next = node.right
            node.right.next = node.next
        elif node.op == ReOp Group
            node.subexpr.next = node.next
        elif node.op == ReOp Union
            for sub in node.all
                sub.next = node.next

def private re_assign_groups ( var re:Regex )
    push(re.groups, [[auto range(0),""]])
    visit_top_down(re.root) <| $ ( var node )
        if node.op == ReOp Group
            let index = length(re.groups)
            node.index = index
            push(re.groups, [[auto range(0),"{index}"]])

/*
    matching
*/

def private re_assign_match_functions ( var re:Regex )
    visit_top_down(re.root) <| $ ( var node )
        if node.op==ReOp Char
            if node.textLen==1
                node.fun2 = @@re_match2_single_char
            else
                node.fun2 = @@re_match2_char
            node.gen2 = @@re_gen2_char
        elif node.op==ReOp Union
            node.fun2 = @@re_match2_union
            node.gen2 = @@re_gen2_union
        elif node.op==ReOp Set
            node.fun2 = @@re_match2_set
            node.gen2 = @@re_gen2_set
        elif node.op==ReOp Any
            node.fun2 = @@re_match2_any
            node.gen2 = @@re_gen2_any
        elif node.op==ReOp Eos
            node.fun2 = @@re_match2_eos
            node.gen2 = @@re_gen2_eos
        elif node.op==ReOp Concat
            node.fun2 = @@re_match2_concat
            node.gen2 = @@re_gen2_concat
        elif node.op==ReOp Plus
            if node.subexpr.op==ReOp Set
                node.cset = node.subexpr.cset
                node.fun2 = @@re_match2_plus_set
            else
                node.fun2 = @@re_match2_plus
            node.fun2 = @@re_match2_plus
            node.gen2 = @@re_gen2_plus
        elif node.op==ReOp Star
            node.fun2 = @@re_match2_star
            node.gen2 = @@re_gen2_star
        elif node.op==ReOp Question
            node.fun2 = @@re_match2_question
            node.gen2 = @@re_gen2_question
        elif node.op==ReOp Group
            node.fun2 = @@re_match2_group
            node.gen2 = @@re_gen2_group
        else
            panic("unsupported {node.op}")

[unsafe_deref]
def private re_match2_single_char ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match single char `{node.text}` {str}\n")
    unsafe
        if character_uat(node.text,0) != int(*str)
            return null
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if node2!=null
            return invoke(node2.fun2,regex,node2,tail)
        else
            return tail

[unsafe_deref]
def private re_match2_char ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match char `{node.text}` {str}\n")
    unsafe
        if memcmp(reinterpret<uint8?> node.text,str,node.textLen) != 0
            return null
        var tail = str + node.textLen
        node.tail = tail
        var node2 = node.next
        if node2!=null
            if log_match_enabled
                print("NOP invoke\n")
            return invoke(node2.fun2,regex,node2,tail)
        else
            return tail

[unsafe_deref]
def private re_match2_union ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match union {str}\n")
    for sub in node.all
        let osub = invoke(sub.fun2,regex,sub,str)
        if osub != null
            node.tail = sub.tail
            return osub
    return null

[unsafe_deref]
def private re_match2_set ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 set {str}\n")
    if !is_char_in_set(int(*str),node.cset)
        return null
    unsafe
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if node2!=null
            return invoke(node2.fun2,regex, node2, tail)
        else
            return tail

[unsafe_deref]
def private re_match2_any ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 any {str}\n")
    if int(*str)==0
        return null
    unsafe
        var tail = str + 1
        node.tail = tail
        var node2 = node.next
        if node2!=null
            return invoke(node2.fun2,regex, node2, tail)
        else
            return tail

[unsafe_deref]
def private re_match2_concat ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 concat {str}\n")
    var left = node.left
    let oleft = invoke(left.fun2, regex, left, str)
    if oleft == null
        return null
    node.tail = node.right.tail
    return oleft

[unsafe_deref]
def private re_match2_eos ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match eos {str}\n")
    if int(*str)!=0
        return null
    node.tail = str
    var node2 = node.next
    if node2!=null
        return invoke(node2.fun2,regex, node2, str)
    else
        return str

[unsafe_deref]
def private re_match2_question ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match question {str}\n")
    var nsub = node.subexpr
    var tail = invoke(nsub.fun2,regex,nsub,str)
    if tail == null
        tail = str
    node.tail = tail
    var node2 = node.next
    if node2!=null
        return invoke(node2.fun2,regex,node2,tail)
    else
        return tail

[unsafe_deref]
def private re_match2_plus ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 plus {str}\n")
    var nsub = node.subexpr
    var osym = invoke(nsub.fun2,regex,nsub,str)
    if osym == null
        return null
    var ofs = osym
    var node2 = node.next
    if node2!=null
        while osym != null
            node.tail = osym
            var otail = invoke(node2.fun2,regex,node2,osym)
            if otail != null
                return otail
            osym = invoke(nsub.fun2,regex,nsub,osym)
        return null
    else
        while osym != null
            ofs = osym
            osym = invoke(nsub.fun2,regex,nsub,ofs)
        node.tail = ofs
        return ofs

[unsafe_deref]
def private re_match2_plus_set ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 plus {str}\n")
    var nsub = node.subexpr
    if !is_char_in_set(int(*str),node.cset)
        return null
    unsafe
        var ofs = str + 1
        var osym = ofs
        var node2 = node.next
        if node2!=null
            while osym != null
                node.tail = osym
                var otail = invoke(node2.fun2,regex,node2,osym)
                if otail != null
                    return otail
                if !is_char_in_set(int(*osym),node.cset)
                    return null
                osym++
            return null
        else
            while osym != null
                ofs = osym
                if !is_char_in_set(int(*osym),node.cset)
                    break
                osym++
            node.tail = ofs
            return ofs

[unsafe_deref]
def private re_match2_star ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 plus {str}\n")
    var ofs = str
    var node2 = node.next
    var nsub = node.subexpr
    if node2!=null
        while ofs != null
            node.tail = ofs
            let oany = invoke(node2.fun2,regex,node2,ofs)
            if oany != null
                return oany
            ofs = invoke(nsub.fun2,regex,nsub,ofs)
        return null
    else
        var osym = ofs
        while osym != null
            ofs = osym
            osym = invoke(nsub.fun2,regex,nsub,ofs)
        node.tail = ofs
        return ofs

[unsafe_deref]
def private re_match2_group ( var regex:Regex; var node:ReNode?; str:uint8? ) : uint8?
    if log_match_enabled
        print("match2 group {str}\n")
    var nsub = node.subexpr
    let osub = invoke(nsub.fun2,regex,nsub,str)
    if osub==null
        return null
    regex.groups[node.index]._0 = range(int(str-regex.match),int(node.subexpr.tail-regex.match))
    return osub

/*
    early out
*/

def private re_early_out ( var cset:CharSet; node:ReNode? ) : bool
    if node.op==ReOp Char
        set_or_char(cset,character_at(node.text,0))
        return false
    elif node.op==ReOp Set
        set_or_set(cset,node.cset)
        return false
    elif node.op==ReOp Any
        for x in cset
            x = 0xffffffffu
        return false
    elif node.op==ReOp Eos
        return false
    elif node.op==ReOp Group
        return re_early_out(cset,node.subexpr)
    elif node.op==ReOp Plus
        return re_early_out(cset,node.subexpr)
    elif node.op==ReOp Star
        re_early_out(cset,node.subexpr)
        return true
    elif node.op==ReOp Question
        re_early_out(cset,node.subexpr)
        return true
    elif node.op==ReOp Concat
        let left = re_early_out(cset,node.left)
        return left ? re_early_out(cset,node.right) : false
    elif node.op==ReOp Union
        var any = false
        for sub in node.all
            any = re_early_out(cset,sub) || any
        return any
    else
        panic("unsupported operation")
        return false

/*
    top level API
*/

def is_valid(var re:Regex)
    //! returns `true` if enumeration compiled correctly
    return re.root != null

def regex_compile( var re:Regex; expr:string ) : bool
    //! Compile regular expression.
    //! Validity of the compiled expression is checked by `is_valid`.
    re.root = re_parse(expr)
    if re.root != null
        re_assign_next(re)
        re_assign_groups(re)
        re_assign_match_functions(re)
        re_early_out(re.earlyOut, re.root)
        re.canEarlyOut = !is_set_empty(re.earlyOut)
    return re.root != null

def regex_compile ( expr : string )
    var re:Regex
    if !regex_compile(re,expr)
        panic("regular expression {expr} did not compile")
    return <- re

def regex_compile ( var re:Regex )
    if re.root != null
        re_assign_next(re)
        re_assign_match_functions(re)
    return <- re

def regex_match ( var regex:Regex; str:string; offset:int=0 ) : int
    //! Returns first match for the regular expression in `str`.
    //! If `offset` is specified, first that many number of symbols will not be matched.
    if empty(str)
        return -1
    if log_match_enabled
        print("matching with `{str}` at {offset}\n")
    unsafe
        regex.match = reinterpret<uint8?> str
        let mptr = invoke(regex.root.fun2,regex,regex.root,regex.match)
        if mptr == null
            return -1
        return int(mptr-regex.match)

def regex_group ( regex:Regex; index:int; match:string )
    //! Returns string for the given group index and match result.
    let sub_range = regex.groups[index]._0
    return slice(match, sub_range.x, sub_range.y)

[unsafe_deref]
def regex_foreach ( var regex:Regex; str:string; blk : block<(at:range):bool> )
    //! Iterates through all matches for the given regular expression in `str`.
    if empty(str)
        return
    unsafe
        regex.match = reinterpret<uint8?> str
        var root = regex.root
        var pstr = reinterpret<uint8?> str
        var cstr = pstr
        if regex.canEarlyOut
            // lets try if it helps
            while true
                let Ch = int(*cstr)
                if Ch==0
                    break
                if is_char_in_set(Ch,regex.earlyOut)
                    let om = invoke(root.fun2,regex,root,cstr)
                    if om != null
                        if !invoke(blk,range(int(cstr-pstr),int(om-pstr)))
                            break
                        cstr = om
                    else
                        cstr++
                else
                    cstr++
        else
            while int(*cstr) != 0
                let om = invoke(root.fun2,regex,root,cstr)
                if om != null
                    if !invoke(blk,range(int(cstr-pstr),int(om-pstr)))
                        break
                    cstr = om
                else
                    cstr ++

[unsafe_deref]
def regex_replace ( var regex:Regex; str:string; blk : block<(at:string):string> )
    //! Iterates through all matches for the given regular expression in `str`.
    if empty(str)
        return ""
    unsafe
        return build_string <| $ ( writer )
            regex.match = reinterpret<uint8?> str
            var root = regex.root
            var pstr = reinterpret<uint8?> str
            var cstr = pstr
            if regex.canEarlyOut
                // lets try if it helps
                while true
                    let Ch = int(*cstr)
                    if Ch==0
                        break
                    if is_char_in_set(Ch,regex.earlyOut)
                        let om = invoke(root.fun2,regex,root,cstr)
                        if om != null
                            let repl = invoke(blk,slice(str,int(cstr-pstr),int(om-pstr)))
                            writer |> write(repl)
                            cstr = om
                        else
                            writer |> write_char(Ch)
                            cstr++
                    else
                        writer |> write_char(Ch)
                        cstr++
            else
                while int(*cstr) != 0
                    let om = invoke(root.fun2,regex,root,cstr)
                    if om != null
                        let repl = invoke(blk,slice(str,int(cstr-pstr),int(om-pstr)))
                        writer |> write(repl)
                        cstr = om
                    else
                        writer |> write_char(int(*cstr))
                        cstr ++

/*
    printer
*/

def regex_debug ( regex:Regex )
    //! Prints regular expression and its related information in human readable form.
    debug_re(regex.root,1)
    print("\n")
    if regex.canEarlyOut
        print("early out: ")
        debug_set(regex.earlyOut)
        print("\n")
    if length(regex.groups) != 0
        print("groups:")
        for g in regex.groups
            print("\t{g._1}\n")

def debug_set(cset:CharSet)
    //! Prints character set in human readable form.
    for x in range(256)
        if (cset[x>>5] & uint(1<<(x & 31))) != 0u
            if x>=32 && x<=127
                print("{to_char(x)}")
            else
                print("\\{x}")

def private debug_re ( node: ReNode?; tab:int )
    print("(")
    if node != null
        print("#{node.id}")
        if node.next!=null
            print("->{node.next.id}")
        else
            print("->!")
        print(" ")
    if node==null
        print("null)")
    elif node.op==ReOp Char
        print("Char `{node.text}`)")
    elif node.op==ReOp Set
        print("Set ")
        debug_set(node.cset)
        print(")")
    elif node.op==ReOp Any
        print("Any)")
    elif node.op==ReOp Eos
        print("Eos)")
    elif node.op==ReOp Group
        print("Group ")
        debug_re(node.subexpr,tab+1)
        print(")")
    elif node.op==ReOp Plus
        print("Plus ")
        debug_re(node.subexpr,tab)
        print(")")
    elif node.op==ReOp Star
        print("Star ")
        debug_re(node.subexpr,tab)
        print(")")
    elif node.op==ReOp Question
        print("Question ")
        debug_re(node.subexpr,tab)
        print(")")
    elif node.op==ReOp Concat
        print("Contact")
        print("\n{repeat("\t",tab)}")
        debug_re(node.left,tab+1)
        print("\n{repeat("\t",tab)}")
        debug_re(node.right,tab+1)
        print("\n{repeat("\t",tab-1)})")
    elif node.op==ReOp Union
        print("Union\n")
        for sub in node.all
            print("{repeat("\t",tab)}")
            debug_re(sub,tab+1)
            print("\n")
        print("{repeat("\t",tab-1)})")
    else
        panic("unsupported op")



def private rnd_set_character ( cset:uint[8]; var rnd:ReGenRandom ) : int
    let total = set_total(cset)
    var idx = 0u
    next(rnd,idx)
    idx = idx % total
    return set_element(int(idx), cset)

def private re_gen2_fail ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen fail\n")
    assert(false, "not running generated regex")

def private re_gen2_char ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen char `{node.text}`\n")
    str |> write(node.text)
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_union ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen union\n")
    var nidx = 0u
    next(rnd,nidx)
    nidx = nidx % uint(length(node.all))
    var sub = node.all[nidx]
    invoke(sub.gen2,sub,rnd,str)

def private re_gen2_set ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen set {node.cset}\n")
    str |> write_char(rnd_set_character(node.cset,rnd))
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_any ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen any\n")
    var ch : uint
    next(rnd, ch)
    str |> write_char( int((ch%255u)+1u) )
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_concat ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen concat\n")
    var left = node.left
    invoke(left.gen2,left,rnd,str)

def private re_gen2_eos ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen eos\n")
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_question ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen question\n")
    var a = 0u
    next(rnd,a)
    if (a & 1u) != 0u               // zero or one?
        var nsub = node.subexpr
        if nsub!=null
            invoke(nsub.gen2,nsub,rnd,str)
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_plus ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen plus\n")
    var cnt = 0u
    next(rnd,cnt)
    cnt = (cnt % re_gen_rep_limit) + 1u
    var nsub = node.subexpr
    if nsub!=null
        for i in urange(cnt)
            invoke(nsub.gen2,nsub,rnd,str)
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_star ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("gen star\n")
    var cnt = 0u
    next(rnd,cnt)
    cnt = cnt % re_gen_rep_limit
    var nsub = node.subexpr
    if nsub!=null
        for i in urange(cnt)
            invoke(nsub.gen2,nsub,rnd,str)
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def private re_gen2_group ( var node:ReNode?; var rnd:ReGenRandom; var str:StringBuilderWriter ) : void
    if log_gen_enabled
        print("match2 group\n")
    var nsub = node.subexpr
    if nsub!=null
        invoke(nsub.gen2,nsub,rnd,str)
    var node2 = node.next
    if node2!=null
        invoke(node2.gen2,node2,rnd,str)

def public re_gen ( var re:Regex; var rnd:ReGenRandom ) : string
    return build_string <| $ ( writer )
        invoke(re.root.gen2,re.root,rnd,writer)
