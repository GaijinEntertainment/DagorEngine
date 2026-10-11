options gen2
options strict_smart_pointers
options indenting = 4

module debug_eval shared private

//! Expression evaluator for the debugger.
//!
//! Parses and evaluates simple C-like expressions (arithmetic, field access,
//! indexing, casts) against live debug context data. Used by the DAP debug
//! server to implement watch and hover evaluation.

require daslib/strings_boost
require daslib/match
require daslib/math_bits
require daslib/ast_boost

require daslib/rtti public
require daslib/debugger

variant Token {
    number : int64
    punkt : int
    ident : string
    invalid : string
    eos : string
}

def each_token(st : string) {
    return <- generator<Token> {
        var tst := st
        var chars <- unsafe(each(tst))
        var ahead = ' '
        while (!empty(chars)) {
            while (is_white_space(ahead) && next(chars, ahead)) {
                pass
            }
            if (empty(chars)) {
                break
            }
            if (ahead == '+' || ahead == '-' || ahead == '*' || ahead == '/' || ahead == '[' || ahead == ']' || ahead == '(' || ahead == ')' || ahead == '.' ||
                ahead == '!' || ahead == '~' || ahead == '&' || ahead == '|' || ahead == '^' || ahead == '<' || ahead == '>' || ahead == '=') {
                yield Token(punkt = ahead)
                next(chars, ahead)
            } elif (is_number(ahead)) {
                var str : array<uint8>
                str |> push(uint8(ahead))
                while (next(chars, ahead) && is_number(ahead)) {
                    str |> push(uint8(ahead))
                }
                if (length(str) == 1 && str[0] == '0'u8 && (ahead == 'x' || ahead == 'X')) {
                    str |> push(uint8(ahead))
                    while (next(chars, ahead) && (is_number(ahead) || (ahead >= 'a' && ahead <= 'f') || (ahead >= 'A' && ahead <= 'F'))) {
                        str |> push(uint8(ahead))
                    }
                    yield Token(number = to_int64(string(str), true))
                } else {
                    yield Token(number = to_int64(string(str)))
                }
            } elif (is_alpha(ahead) || ahead == '_' || ahead == '`') {
                var str : array<uint8>
                str |> push(uint8(ahead))
                while (next(chars, ahead) && (is_alpha(ahead) || is_number(ahead) || ahead == '_' || ahead == '`')) {
                    str |> push(uint8(ahead))
                }
                yield Token(ident = string(str))
            } else {
                yield Token(invalid = "invalid character {to_char(ahead)}")
                return false
            }
        }
        yield Token(eos = "eos")
        return false
    }
}

let INVALID_VALUE = LONG_MIN

struct public Result {
    //! Result of evaluating a debug expression.
    tinfo : TypeInfo //! Type information of the result.
    value : float4 //! Raw value storage for the result.
    data  : void const? //! Pointer to the result data, if available.
    error : string //! Error message, empty if evaluation succeeded.
}

struct TokenStream {
    //! Token stream for parsing debug expressions.
    tokens : iterator<Token> //! Iterator over lexed tokens.
    stream : array<Token> //! Buffered array of tokens.
    errors : array<string> //! Accumulated parsing errors.
    names  : table<string; Result> //! Named variable lookup table for expression evaluation.
    shortCircuited : bool //! True while an operand is being parsed whose value a logical operator has already decided without it; such an operand reports syntax errors but not evaluation errors.
}

def TokenStream(st : string) {
    return <- TokenStream(tokens <- each_token(st))
}

def token(var st : TokenStream) {
    var token : Token
    if (st.stream |> empty) {
        if (!next(st.tokens, token)) {
            token = Token(eos = "eos")
        }
    } else {
        token = st.stream |> back
        st.stream |> pop
    }
    return token
}

def unput(var st : TokenStream; token : Token) {
    st.stream |> push(token)
}

def Result(value : int64) {
    var self : Result
    self.tinfo.basicType = Type.tInt64
    self.value = cast_to_vec4f(value)
    return self
}

def Result(value : bool) {
    var self : Result
    self.tinfo.basicType = Type.tBool
    self.value = cast_to_vec4f(value)
    return self
}

def InvalidResult {
    var self : Result
    self.tinfo.basicType = Type.tVoid
    return self
}

def syntax_error(var st : TokenStream; message : string) {
    st.errors |> push(message)
    return InvalidResult()
}

def eval_error(var st : TokenStream; message : string) {
    if (!st.shortCircuited) st.errors |> push(message)
    return InvalidResult()
}

def loadI(data : void?; t : auto(TT)) {
    unsafe {
        return *(reinterpret<TT?>(data))
    }
}

[unused_argument(st)]
def getPD(var st : TokenStream; result : Result; offset : int = 0) : void? {
    unsafe {
        var data : void const?
        if (result.data != null) {
            data = result.data
        } else {
            data = addr(result.value)
            if (result.tinfo.flags.refType) {
                let pdata = reinterpret<void ??>(data)
                if (pdata == null) return null
                data = *pdata
            }
        }
        if (result.tinfo.flags.ref) {
            let pdata = reinterpret<void ??>(data)
            if (pdata == null) return null
            data = *pdata
        }
        return (reinterpret<uint8 const?>(data)) + offset
    }
}

def getI(var st : TokenStream; result : Result) : int64 {
    if (result.tinfo.dimSize != 0u) {
        st |> eval_error("can't get value of [{result.tinfo.dimSize}]")
        return INVALID_VALUE
    }
    let bt = result.tinfo.basicType
    let data = st |> getPD(result)
    if (data == null) {
        st |> eval_error("null pointer dereference for {result.tinfo.basicType}")
        return INVALID_VALUE
    }
    return loadI(data, type<int64>) if (bt == Type.tInt64 || bt == Type.tUInt64)
    return int64(loadI(data, type<int>)) if (bt == Type.tInt || bt == Type.tUInt)
    return int64(loadI(data, type<int16>)) if (bt == Type.tInt16 || bt == Type.tUInt16)
    return int64(loadI(data, type<int8>)) if (bt == Type.tInt8 || bt == Type.tUInt8)
    return (loadI(data, type<bool>) ? 1l : 0l) if (bt == Type.tBool)
    st |> eval_error("can't get value of {result.tinfo.basicType}")
    return INVALID_VALUE
}

[unused_argument(st)]
def getT(var st : TokenStream; result : Result) : string {
    if (!empty(result.error)) return ""
    if (result.data != null) {
        return sprint_data(result.data, unsafe(addr(result.tinfo)), print_flags.singleLine)
    } else {
        return sprint_data(result.value, unsafe(addr(result.tinfo)), print_flags.singleLine)
    }
}

def func_call_length(var st : TokenStream; result : Result) : Result {
    match (result.tinfo) {
        if (TypeInfo(dimSize = $v(dim)) && (dim != 0u)) {
            let idim = result.tinfo |> get_dim(int(result.tinfo.dimSize - 1u))
            return <- Result(int64(idim))
        }
        if (TypeInfo(basicType = Type.tString)) {
            let pstr = unsafe(reinterpret<string?>(getPD(st, result)))
            return <- Result(long_length(*pstr))
        }
        if (TypeInfo(basicType = Type.tArray)) {
            let parr = unsafe(reinterpret<DapiArray?>(getPD(st, result)))
            return <- Result(int64(parr.size))
        }
        if (TypeInfo(basicType = Type.tTable)) {
            let ptab = unsafe(reinterpret<DapiTable?>(getPD(st, result)))
            return <- Result(int64(ptab.size))
        }
    }
    return st |> eval_error("unsupported length for type {result.tinfo.basicType}")
}

def func_call(var st : TokenStream; name : string; arg : Result) : Result {
    return func_call_length(st, arg) if (name == "length")
    return st |> eval_error("unknown function {name}")
}

def expr_value(var st : TokenStream) : Result {
    let token = token(st)
    match (token) {
        if (Token(invalid = _)) {
            return st |> syntax_error("invalid token {token}, expecting number or (")
        }
        if (Token(eos = _)) {
            return st |> syntax_error("unexpected end of stream")
        }
        if (Token(number = $v(value))) {
            return <- Result(value)
        }
        if (Token(ident = $v(name))) {
            var ntoken = token(st)
            if (ntoken ?as punkt ?? 0 == '(') {
                let arg = expr(st);
                ntoken = token(st);
                if (ntoken ?as punkt ?? 0 != ')') return st |> syntax_error("expecting )")
                return func_call(st, name, arg)
            }
            unput(st, ntoken)
            if (name == "true") {
                return <- Result(true)
            } elif (name == "false") return <- Result(false)
            if (st.names |> key_exists(name)) return st.names |> get_value(name)
            return st |> eval_error("unknown variable {name}")
        }
        if (Token(punkt = '(')) {
            var res = expr(st);
            if (token(st) ?as punkt ?? 0 != ')') return st |> syntax_error("expecting )")
            return <- res
        }
        if (Token(punkt = $v(any))) return st |> syntax_error("unexpected character {to_char(any)}, expecting number or (")  // nolint:STYLE005 (qmatch macro expansion)
    }
    return st |> syntax_error("unexpected token {token}, expecting number or (")
}

[unused_argument(st)]
def getStructFieldOffset(var st : TokenStream; ident : string; var result : Result) : tuple<offset : int; tinfo : TypeInfo> {
    var size = 0
    for (field in *result.tinfo.structType) {
        var tinfo = unsafe(addr<TypeInfo?>(field))
        if (field.name == ident) return (offset = size, tinfo = *tinfo)
        let al = get_type_align(tinfo) - 1
        size = (size + al) & ~al
        size += get_type_size(tinfo)
    }
    return (offset = -1, tinfo = TypeInfo(uninitialized))
}

[unused_argument(st)]
def getTupleFieldOffset(var st : TokenStream; ident : string; result : Result) : tuple<offset : int; tinfo : TypeInfo> {
    var size = 0
    for (cnt in range(result.tinfo.argCount)) {
        let tinfo = unsafe(result.tinfo.argTypes[cnt])
        if (ident == "_{cnt}") return (offset = size, tinfo = *tinfo)
        let al = get_type_align(tinfo) - 1
        size = (size + al) & ~al
        size += get_type_size(tinfo)
    }
    return (offset = -1, tinfo = TypeInfo(uninitialized))
}

[unused_argument(st)]
def getVectorOffset(var st : TokenStream; ident : string; result : Result) : tuple<offset : int; tinfo : TypeInfo> {
    let ofs = getVectorOffset(result.tinfo.basicType, ident)
    return (offset = -1, tinfo = TypeInfo(uninitialized)) if (ofs < 0)
    let fieldSize = getVectorElementSize(result.tinfo.basicType)
    let tinfo = TypeInfo(
        basicType = getVectorElementType(result.tinfo.basicType),
        size = uint(fieldSize)
    )
    return (offset = ofs * fieldSize, tinfo = tinfo)
}

[unused_argument(st)]
def getHandleFieldOffset(var st : TokenStream; ident : string; result : Result) : tuple<offset : int; tinfo : TypeInfo> {
    var ann = unsafe(reinterpret<TypeAnnotation?>(result.tinfo.annotation))
    if (ann == null) return (offset = -1, tinfo = TypeInfo(uninitialized))
    let ofs  = int(get_handled_type_field_offset(ann, ident))
    if (ofs == -1) return (offset = -1, tinfo = TypeInfo(uninitialized))
    let tinfo = get_handled_type_field_type(ann, ident)
    return (offset = ofs, tinfo = *tinfo)
}

def expr_field(var st : TokenStream) : Result {
    var result = expr_value(st)
    let token = token(st)
    if (token ?as punkt ?? 0 == '.') {
        let field = token(st)
        if (field is ident) {
            var oti : tuple<offset : int; tinfo : TypeInfo>
            oti.offset = -1
            match (result.tinfo) {
                if (TypeInfo(dimSize = $v(dimSize)) && (dimSize != 0u)) {
                    return st |> eval_error("can't access field of array")
                }
                if (TypeInfo(basicType = Type.tStructure)) {
                    oti = getStructFieldOffset(st, field as ident, result)
                }
                if (TypeInfo(basicType = Type.tTuple)) {
                    oti = getTupleFieldOffset(st, field as ident, result)
                }
                if (TypeInfo(basicType = $v(bt)) && (getVectorElementCount(bt) != 0)) {
                    oti = getVectorOffset(st, field as ident, result)
                }
                if (TypeInfo(basicType = Type.tHandle)) {
                    oti = getHandleFieldOffset(st, field as ident, result)
                }
            }
            if (oti.offset < 0) return st |> eval_error("unknown field {field} in type {result.tinfo.basicType}")
            let newData = getPD(st, result, oti.offset)
            result.data = newData
            result.tinfo = oti.tinfo
            return result
        } else {
            return st |> syntax_error("expecting field name")
        }
    } else {
        unput(st, token)
    }
    return result
}

def opDim(var st : TokenStream; index : int64; var result : Result) {
    let dim = result.tinfo |> get_dim(int(result.tinfo.dimSize - 1u))
    if (index < 0l || index >= int64(dim)) return st |> eval_error("index out of range")
    result.tinfo.dimSize--
    let elemSize = get_type_size(unsafe(addr(result.tinfo)))
    result.data = st |> getPD(result, int(index) * elemSize)
    return result
}

def opDimString(var st : TokenStream; index : int64; var result : Result) {
    var str : string
    if (result.data != null) {
        str = *(unsafe(reinterpret<string?>(result.data)))
    } else {
        str = cast_to_string(result.value)
    }
    let len = length(str)
    if (index >= int64(len)) return st |> eval_error("index out of range {index} of {len} (string too short)")
    return Result(int64(character_at(str, int(index)))) // nolint:PERF003 single indexed access, not a loop
}

def opArray(var st : TokenStream; index : int64; var result : Result) {
    unsafe {
        let pArray = reinterpret<DapiArray?>(getPD(st, result))
        if (pArray == null) return st |> eval_error("null pointer")
        let dim = pArray.size
        if (index < 0l || uint64(index) >= dim) return st |> eval_error("index out of range")
        let pData = reinterpret<int8?>(pArray.data)
        let elemSize = get_type_size(result.tinfo.firstType)
        let offset = index * int64(elemSize)
        result.data = pData + offset
        result.tinfo = *result.tinfo.firstType
        return result
    }
}

def opTable(var st : TokenStream; index : Result; var result : Result) {
    unsafe {
        let pTable = reinterpret<DapiTable?>(getPD(st, result))
        if (pTable == null) return st |> eval_error("table - null pointer")
        let pKey = reinterpret<float4?>(getPD(st, index))
        if (pKey == null) return st |> eval_error("key - null pointer")
        let key = *pKey
        let keyBaseType = result.tinfo.firstType.basicType
        let valueTypeSize = get_type_size(result.tinfo.secondType)
        let elementIndex = get_table_key_index(pTable, key, keyBaseType, valueTypeSize)
        if (elementIndex == -1) return st |> eval_error("key not found")
        let pValue = (reinterpret<uint8?>(pTable.data)) + elementIndex * valueTypeSize
        result.data = pValue
        result.tinfo = *result.tinfo.secondType
        return result
    }
}

def expr_at(var st : TokenStream) : Result {
    var result = expr_field(st)
    var token = token(st)
    while (token ?as punkt ?? 0 == '[') {
        let eindex = expr(st);
        if (token(st) ?as punkt ?? 0 != ']') return st |> syntax_error("expecting ]")
        var nresult : Result
        match (result.tinfo) {
            if (TypeInfo(basicType = Type.tString, dimSize = 0u)) {
                nresult = opDimString(st, st |> getI(eindex), result)
            }
            if (TypeInfo(basicType = Type.tArray, dimSize = 0u)) {
                nresult = opArray(st, st |> getI(eindex), result)
            }
            if (TypeInfo(basicType = Type.tTable, dimSize = 0u)) {
                nresult = opTable(st, eindex, result)
            }
            if (TypeInfo(dimSize = 0u)) {
                return st |> eval_error("can't index non-array")
            }
            if (_) {
                nresult = opDim(st, st |> getI(eindex), result)
            }
        }
        result = nresult
        token = token(st)
    }
    unput(st, token)
    return result
}

def expr_unary(var st : TokenStream) : Result {
    let token = token(st)
    match (token) {
        if (Token(punkt = '+')) {
            return Result(st |> getI(expr_unary(st)))
        }
        if (Token(punkt = '-')) {
            return Result(-st |> getI(expr_unary(st)))
        }
        if (Token(punkt = '~')) {
            return Result(~st |> getI(expr_unary(st)))
        }
        if (Token(punkt = '!')) return Result(st |> getI(expr_unary(st)) == 0l)  // nolint:STYLE005 (qmatch macro expansion)
    }
    unput(st, token)
    return expr_at(st)
}

def try_accept_punkt(var st : TokenStream; punkt : int) {
    let token = token(st)
    if (token ?as punkt ?? 0 != punkt) {
        unput(st, token)
        return false
    }
    return true
}

def try_accept_punkt_pair(var st : TokenStream; firstPunkt : int; secondPunkt : int) {
    let first = token(st)
    if (first ?as punkt ?? 0 != firstPunkt) {
        unput(st, first)
        return false
    }
    let second = token(st)
    if (second ?as punkt ?? 0 != secondPunkt) {
        unput(st, second)
        unput(st, first)
        return false
    }
    return true
}

def try_accept_solo_punkt(var st : TokenStream; punkt : int) {
    let first = token(st)
    if (first ?as punkt ?? 0 != punkt) {
        unput(st, first)
        return false
    }
    let second = token(st)
    unput(st, second)
    if (second ?as punkt ?? 0 == punkt) {
        unput(st, first)
        return false
    }
    return true
}

def expr_mul_div(var st : TokenStream) : Result {
    var leftValue = expr_unary(st)
    while (true) {
        if (try_accept_punkt(st, '*')) {
            let rightValue = expr_unary(st)
            leftValue = Result((st |> getI(leftValue)) * (st |> getI(rightValue)))
        } elif (try_accept_punkt(st, '/')) {
            let rightValue = expr_unary(st)
            let divisor = st |> getI(rightValue)
            if (divisor == 0l) return st |> eval_error("runtime error, division by 0")
            let dividend = st |> getI(leftValue)
            if (divisor == -1l) {
                leftValue = Result(-dividend)
            } else {
                leftValue = Result(dividend / divisor)
            }
        } else {
            break
        }
    }
    return <- leftValue
}

def expr_add_sub(var st : TokenStream) : Result {
    var leftValue = expr_mul_div(st)
    while (true) {
        if (try_accept_punkt(st, '+')) {
            let rightValue = expr_mul_div(st)
            leftValue = Result((st |> getI(leftValue)) + (st |> getI(rightValue)))
        } elif (try_accept_punkt(st, '-')) {
            let rightValue = expr_mul_div(st)
            leftValue = Result((st |> getI(leftValue)) - (st |> getI(rightValue)))
        } else {
            break
        }
    }
    return <- leftValue
}

def expr_relational(var st : TokenStream) : Result {
    var leftValue = expr_add_sub(st)
    while (true) {
        if (try_accept_punkt_pair(st, '<', '=')) {
            let rightValue = expr_add_sub(st)
            leftValue = Result((st |> getI(leftValue)) <= (st |> getI(rightValue)))
        } elif (try_accept_punkt_pair(st, '>', '=')) {
            let rightValue = expr_add_sub(st)
            leftValue = Result((st |> getI(leftValue)) >= (st |> getI(rightValue)))
        } elif (try_accept_punkt(st, '<')) {
            let rightValue = expr_add_sub(st)
            leftValue = Result((st |> getI(leftValue)) < (st |> getI(rightValue)))
        } elif (try_accept_punkt(st, '>')) {
            let rightValue = expr_add_sub(st)
            leftValue = Result((st |> getI(leftValue)) > (st |> getI(rightValue)))
        } else {
            break
        }
    }
    return <- leftValue
}

def expr_equality(var st : TokenStream) : Result {
    var leftValue = expr_relational(st)
    while (true) {
        if (try_accept_punkt_pair(st, '=', '=')) {
            let rightValue = expr_relational(st)
            leftValue = Result((st |> getT(leftValue)) == (st |> getT(rightValue)))
        } elif (try_accept_punkt_pair(st, '!', '=')) {
            let rightValue = expr_relational(st)
            leftValue = Result((st |> getT(leftValue)) != (st |> getT(rightValue)))
        } else {
            break
        }
    }
    return <- leftValue
}

def expr_bit_and(var st : TokenStream) : Result {
    var leftValue = expr_equality(st)
    while (try_accept_solo_punkt(st, '&')) {
        let rightValue = expr_equality(st)
        leftValue = Result((st |> getI(leftValue)) & (st |> getI(rightValue)))
    }
    return <- leftValue
}

def expr_bit_xor(var st : TokenStream) : Result {
    var leftValue = expr_bit_and(st)
    while (try_accept_solo_punkt(st, '^')) {
        let rightValue = expr_bit_and(st)
        leftValue = Result((st |> getI(leftValue)) ^ (st |> getI(rightValue)))
    }
    return <- leftValue
}

def expr_bit_or(var st : TokenStream) : Result {
    var leftValue = expr_bit_xor(st)
    while (try_accept_solo_punkt(st, '|')) {
        let rightValue = expr_bit_xor(st)
        leftValue = Result((st |> getI(leftValue)) | (st |> getI(rightValue)))
    }
    return <- leftValue
}

def expr_logic_and(var st : TokenStream) : Result {
    var leftValue = expr_bit_or(st)
    while (try_accept_punkt_pair(st, '&', '&')) {
        let leftTrue = (st |> getI(leftValue)) != 0l
        let wasShortCircuited = st.shortCircuited
        st.shortCircuited = wasShortCircuited || !leftTrue
        let rightValue = expr_bit_or(st)
        let bothTrue = leftTrue && ((st |> getI(rightValue)) != 0l)
        st.shortCircuited = wasShortCircuited
        leftValue = Result(bothTrue)
    }
    return <- leftValue
}

def expr_logic_xor(var st : TokenStream) : Result {
    var leftValue = expr_logic_and(st)
    while (try_accept_punkt_pair(st, '^', '^')) {
        let rightValue = expr_logic_and(st)
        let leftTrue = (st |> getI(leftValue)) != 0l
        leftValue = Result(leftTrue != ((st |> getI(rightValue)) != 0l))
    }
    return <- leftValue
}

def expr_logic_or(var st : TokenStream) : Result {
    var leftValue = expr_logic_xor(st)
    while (try_accept_punkt_pair(st, '|', '|')) {
        let leftTrue = (st |> getI(leftValue)) != 0l
        let wasShortCircuited = st.shortCircuited
        st.shortCircuited = wasShortCircuited || leftTrue
        let rightValue = expr_logic_xor(st)
        let eitherTrue = leftTrue || ((st |> getI(rightValue)) != 0l)
        st.shortCircuited = wasShortCircuited
        leftValue = Result(eitherTrue)
    }
    return <- leftValue
}

def expr(var st : TokenStream) : Result {
    return expr_logic_or(st)
}

def public debug_eval(context : table<string; Result>; expr : string) : Result {
    //! Evaluates a debug expression string with the given variable context and returns the result.
    var st <- TokenStream(expr)
    st.names := context
    var result = expr(st)
    let token = token(st)
    if (!(token is eos)) return Result(error = "unexpected token {token}")
    if (!empty(st.errors)) return Result(error = st.errors[0])
    return <- result
}

