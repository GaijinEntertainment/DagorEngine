options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers
options indenting = 4
options no_aot = true

module debug_eval shared private

require daslib/strings_boost
require daslib/match
require daslib/math_bits
require daslib/ast_boost

require rtti public
require debugapi

variant Token
    number : int64
    punkt : int
    ident : string
    invalid : string
    eos : string

def each_token ( st:string )
    return <- generator<Token> () <|
        var tst := st
        var chars <- unsafe(each(tst))
        var ahead = ' '
        while !empty(chars)
            while is_white_space(ahead) && next(chars,ahead)
                pass
            if empty(chars)
                break
            if (ahead=='+' || ahead=='-' || ahead=='*' || ahead=='/' || ahead=='[' || ahead==']' || ahead=='(' || ahead==')' || ahead=='.' ||
                ahead=='!' || ahead=='~' || ahead=='&' || ahead=='|' || ahead=='^' || ahead=='<' || ahead=='>' || ahead=='=')
                yield [[Token punkt=ahead]]
                next(chars,ahead)
            elif is_number(ahead)
                var str : array<uint8>
                str |> push(uint8(ahead))
                while next(chars,ahead) && is_number(ahead)
                    str |> push(uint8(ahead))
                if length(str)==1 && str[0]=='0'u8 && (ahead=='x' || ahead=='X')
                    str |> push(uint8(ahead))
                    while next(chars,ahead) && (is_number(ahead) || (ahead>='a' && ahead<='f') || (ahead>='A' && ahead<='F'))
                        str |> push(uint8(ahead))
                    yield [[Token number=to_int64(string(str),true)]]
                else
                    yield [[Token number=to_int64(string(str))]]
            elif is_alpha(ahead) || ahead=='_' || ahead=='`'
                var str : array<uint8>
                str |> push(uint8(ahead))
                while next(chars,ahead) && (is_alpha(ahead) || is_number(ahead) || ahead=='_' || ahead=='`')
                    str |> push(uint8(ahead))
                yield [[Token ident=string(str)]]
            else
                yield [[Token invalid="invalid character {to_char(ahead)}"]]
                return false
        yield [[Token eos="eos"]]
        return false

let INVALID_VALUE = LONG_MIN

struct public Result
    tinfo : TypeInfo
    value : float4
    data  : void?
    error : string

struct TokenStream
    tokens : iterator<Token>
    stream : array<Token>
    errors : array<string>
    names  : table<string; Result>

def TokenStream ( st:string )
    return <- [[TokenStream tokens<-each_token(st)]]

def token ( var st:TokenStream )
    var token : Token
    if st.stream |> empty
        if !next(st.tokens, token)
            token = [[Token eos="eos"]]
    else
        token = st.stream |> back
        st.stream |> pop
    return token

def unput ( var st:TokenStream; token:Token )
    st.stream |> push(token)

def Result ( value:int64 )
    var self : Result
    self.tinfo.basicType = Type tInt64
    self.value = cast_to_vec4f(value)
    return self

def Result ( value:bool )
    var self : Result
    self.tinfo.basicType = Type tBool
    self.value = cast_to_vec4f(value)
    return self

def InvalidResult
    var self : Result
    self.tinfo.basicType = Type tVoid
    return self

def error ( var st:TokenStream; error:string )
    st.errors |> push(error)
    return InvalidResult()

def loadI ( data:void?; t:auto(TT) )
    unsafe
        return *(reinterpret<TT?> data)

def getPD ( var st:TokenStream; result:Result; offset:int = 0 ) : void?
    unsafe
        var data : void?
        if result.data != null
            data = result.data
        else
            data = addr(result.value)
            if result.tinfo.flags.refType
                let pdata = reinterpret<void??>data
                if pdata == null
                    return null
                data = *pdata
        if result.tinfo.flags.ref
            let pdata = reinterpret<void??>data
            if pdata == null
                return null
            data = *pdata
        return (reinterpret<uint8?>data) + offset

def getI ( var st:TokenStream; result:Result ) : int64
    if result.tinfo.dimSize != 0u
        st |> error("can't get value of [{result.tinfo.dimSize}]")
        return INVALID_VALUE
    let bt = result.tinfo.basicType
    let data = st |> getPD(result)
    if data==null
        st |> error("null pointer dereference for {result.tinfo.basicType}")
        return INVALID_VALUE
    return loadI(data,type<int64>) if bt==Type tInt64 || bt==Type tUInt64
    return int64(loadI(data,type<int>)) if bt==Type tInt || bt==Type tUInt
    return int64(loadI(data,type<int16>)) if bt==Type tInt16 || bt==Type tUInt16
    return int64(loadI(data,type<int8>)) if bt==Type tInt8 || bt==Type tUInt8
    return int64(loadI(data,type<bool>) ? 1l : 0l) if bt==Type tBool
    st |> error("can't get value of {result.tinfo.basicType}")
    return INVALID_VALUE

def getT ( var st:TokenStream; result:Result ) : string
    if !empty(result.error)
        return ""
    if result.data != null
        return sprint_data(result.data, unsafe(addr(result.tinfo)), print_flags singleLine)
    else
        return sprint_data(result.value, unsafe(addr(result.tinfo)), print_flags singleLine)

def func_call_length ( var st : TokenStream; result:Result ) : Result
    match result.tinfo
        if [[TypeInfo dimSize=$v(dim)]] && (dim!=0u)
            let idim = result.tinfo |> get_dim(int(result.tinfo.dimSize-1u))
            return <- Result(int64(idim))
        if [[TypeInfo basicType=Type tString]]
            let pstr = unsafe(reinterpret<string?> getPD(st,result))
            return <- Result(int64(length(*pstr)))
        if [[TypeInfo basicType=Type tArray]]
            let parr = unsafe(reinterpret<DapiArray?> getPD(st,result))
            return <- Result(int64(parr.size))
        if [[TypeInfo basicType=Type tTable]]
            let ptab = unsafe(reinterpret<DapiTable?> getPD(st,result))
            return <- Result(int64(ptab.size))
    return st |> error("unsupported length for type {result.tinfo.basicType}")

def func_call ( var st : TokenStream; name:string; arg:Result ) : Result
    return func_call_length(st,arg)    if name == "length"
    return st |> error("unknown function {name}")

def expr_value ( var st : TokenStream ) : Result
    let token = token(st)
    match token
        if [[Token invalid=_]]
            return st |> error("invalid token {token}, expecting number or (")
        if [[Token eos=_]]
            return st |> error("unexpected end of stream")
        if [[Token number=$v(value)]]                           // number
            return <- Result(value)
        if [[Token ident=$v(name)]]                             // identifier
            var ntoken = token(st)
            if ntoken ?as punkt ?? 0 == '('
                let arg = expr(st)
                ntoken = token(st)
                if ntoken ?as punkt ?? 0 != ')'
                    return st |> error("expecting )")
                return func_call(st, name, arg)
            unput(st, ntoken)
            if name=="true"
                return <- Result(true)
            elif name=="false"
                return <- Result(false)
            if st.names |> key_exists(name)
                return st.names[name]
            return st |> error("unknown variable {name}")
        if [[Token punkt='(']]                                  // '(' expr ')
            var res = expr(st)
            if token(st) ?as punkt ?? 0 != ')'
                return st |> error("expecting )")
            return <- res
        if [[Token punkt=$v(any)]]
            return st |> error("unexpected character {to_char(any)}, expecting number or (")
    return st |> error("unexpected token {token}, expecting number or (")

def getStructFieldOffset ( var st:TokenStream; ident:string; var result:Result ) : tuple<int;TypeInfo>
    var size = 0
    for field in *result.tinfo.structType
        var tinfo : TypeInfo?
        unsafe
            tinfo = reinterpret<TypeInfo?>(unsafe(addr(field)))
        if field.name==ident
            return [[auto size, *tinfo]]
        let al = get_type_align(tinfo) - 1
        size = (size + al) & ~al
        size += get_type_size(tinfo)
    return [[auto -1,[[TypeInfo]]]]

def getTupleFieldOffset ( var st:TokenStream; ident:string; var result:Result ) : tuple<int;TypeInfo>
    var size = 0
    for cnt in range(result.tinfo.argCount)
        let tinfo = unsafe(result.tinfo.argTypes[cnt])
        if ident=="_{cnt}"
            return [[auto size, *tinfo]]
        let al = get_type_align(tinfo) - 1
        size = (size + al) & ~al
        size += get_type_size(tinfo)
    return [[auto -1,[[TypeInfo]]]]

def getVectorOffset ( var st:TokenStream; ident:string; var result:Result ) : tuple<int;TypeInfo>
    let ofs = getVectorOffset(result.tinfo.basicType, ident)
    return [[auto -1,[[TypeInfo]]]] if ofs < 0
    let fieldSize = getVectorElementSize(result.tinfo.basicType)
    let tinfo = [[TypeInfo
        basicType = getVectorElementType(result.tinfo.basicType),
        size = uint(fieldSize)
    ]]
    return [[auto ofs*fieldSize,tinfo]]

def getHandleFieldOffset ( var st:TokenStream; ident:string; var result:Result ) : tuple<int;TypeInfo>
    unsafe
        var ann <- unsafe(reinterpret<smart_ptr<TypeAnnotation>> result.tinfo.annotation)
        if ann == null
            return [[auto -1, [[TypeInfo()]]]]
        let ofs  = int(get_handled_type_field_offset(ann, ident))
        if ofs == -1
            return [[auto -1, [[TypeInfo()]]]]
        let tinfo = get_handled_type_field_type(ann, ident)
        return [[auto ofs, *tinfo]]

def expr_field ( var st:TokenStream ) : Result
    var result = expr_value(st)
    let token = token(st)
    if token ?as punkt ?? 0 == '.'
        let field = token(st)
        if field is ident
            var oti : tuple<offset:int; tinfo:TypeInfo>
            oti.offset = -1
            match result.tinfo
                if [[TypeInfo dimSize=$v(dimSize)]] && (dimSize!=0u)
                    return st |> error("can't access field of array")
                if [[TypeInfo basicType=Type tStructure]]
                    oti = getStructFieldOffset(st, field as ident, result)
                if [[TypeInfo basicType=Type tTuple]]
                    oti = getTupleFieldOffset(st, field as ident, result)
                if [[TypeInfo basicType=$v(bt)]] && (getVectorElementCount(bt)!=0)
                    oti = getVectorOffset(st, field as ident, result)
                if [[TypeInfo basicType=Type tHandle]]
                    oti = getHandleFieldOffset(st, field as ident, result)
            if oti.offset < 0
                return st |> error("unknown field {field} in type {result.tinfo.basicType}")
            let newData = getPD(st, result,oti.offset)
            result.data = newData
            result.tinfo = oti.tinfo
            return result
        else
            return st |> error("expecting field name")
    else
        unput(st, token)
    return result

def opDim ( var st:TokenStream; index:int64; var result:Result )
    let dim = result.tinfo |> get_dim(int(result.tinfo.dimSize-1u))
    if index < 0l || index >= int64(dim)
        return st |> error("index out of range")
    result.tinfo.dimSize--
    let elemSize = get_type_size(unsafe(addr(result.tinfo)))
    result.data = st |> getPD(result, int(index)*elemSize)
    return result

def opDimString ( var st:TokenStream; index:int64; var result:Result )
    var str : string
    unsafe
        if result.data!=null
            str = *(reinterpret<string?>(result.data))
        else
            str = cast_to_string(result.value)
    let len = length(str)
    if index >= int64(len)
        return st |> error("index out of range {index} of {len} (string too short)")
    return Result(int64(character_at(str, int(index))))

def opArray ( var st:TokenStream; index:int64; var result:Result )
    unsafe
        let pArray = reinterpret<DapiArray?> getPD(st, result)
        if pArray==null
            return st |> error("null pointer")
        let dim = pArray.size
        if index < 0l || index >= int64(dim)
            return st |> error("index out of range")
        let pData = reinterpret<int8?> pArray.data
        let elemSize = get_type_size(result.tinfo.firstType)
        let offset = int(index) * elemSize
        result.data = pData + offset
        result.tinfo = *result.tinfo.firstType
        return result

def opTable ( var st:TokenStream; index:Result; var result:Result )
    unsafe
        let pTable = reinterpret<DapiTable?> getPD(st, result)
        if pTable==null
            return st |> error("table - null pointer")
        let pKey = reinterpret<float4?>(getPD(st, index))
        if pKey==null
            return st |> error("key - null pointer")
        let key = *pKey
        let keyBaseType = result.tinfo.firstType.basicType
        let valueTypeSize = get_type_size(result.tinfo.secondType)
        let elementIndex = get_table_key_index(pTable, key, keyBaseType, valueTypeSize)
        if elementIndex == -1
            return st |> error("key not found")
        let pValue = (reinterpret<uint8?>pTable.data) + elementIndex * valueTypeSize
        result.data = pValue
        result.tinfo = *result.tinfo.secondType
        return result

def expr_at ( var st:TokenStream ) : Result
    var result = expr_field(st)
    var token = token(st)
    while token ?as punkt ?? 0 == '['
        let eindex = expr(st)
        if token(st) ?as punkt ?? 0 != ']'
            return st |> error("expecting ]")
        var nresult : Result
        match result.tinfo
            if [[TypeInfo basicType=Type tString, dimSize=0u]]
                nresult = opDimString(st, st|>getI(eindex), result)
            if [[TypeInfo basicType=Type tArray, dimSize=0u]]
                nresult = opArray(st, st|>getI(eindex), result)
            if [[TypeInfo basicType=Type tTable, dimSize=0u]]
                nresult = opTable(st, eindex, result)
            if [[TypeInfo dimSize=0u]]
                return st |> error("can't index non-array")
            if _
                nresult = opDim(st, st|>getI(eindex), result)
        result = nresult
        token = token(st)
    unput(st, token)
    return result

def expr_unary ( var st:TokenStream ) : Result
    let token = token(st)
    match token
        if [[Token punkt='+']]
            return Result(st|>getI(expr_unary(st)))
        if [[Token punkt='-']]
            return Result(-st|>getI(expr_unary(st)))
        if [[Token punkt='~']]
            return Result(~st|>getI(expr_unary(st)))
        if [[Token punkt='!']]
            return Result(st|>getI(expr_unary(st))==0l)
    unput(st, token)
    return expr_at(st)

def expr_mul_div ( var st:TokenStream ) : Result
    var leftValue = expr_unary(st)
    let token = token(st)
    match token
        if [[Token punkt='*']]
            return Result(st|>getI(leftValue) * st|>getI(expr_mul_div(st)))
        if [[Token punkt='/']]
            let rightValue = st|>getI(expr_mul_div(st))
            if rightValue==0l
                return st |> error("runtime error, division by 0")
            return Result(st|>getI(leftValue) * rightValue)
        if _
            unput(st, token)
    return <- leftValue

def expr_add_sub ( var st:TokenStream ) : Result
    var leftValue = expr_mul_div(st)
    let token = token(st)
    match token
        if [[Token punkt='+']]
            return Result(st|>getI(leftValue) + st|>getI(expr_add_sub(st)))
        if [[Token punkt='-']]
            return Result(st|>getI(leftValue) - st|>getI(expr_add_sub(st)))
        if _
            unput(st, token)
    return <- leftValue

def try_accept_punkt ( var st:TokenStream; punkt:int )
    let token = token(st)
    if token ?as punkt ?? 0 != punkt
        unput(st, token)
        return false
    return true

def expr_and ( var st:TokenStream ) : Result
    var leftValue = expr_add_sub(st)
    if try_accept_punkt(st, '&')
        if try_accept_punkt(st, '&')
            return Result((st|>getI(leftValue) & st|>getI(expr_and(st))) != 0l)
        else
            return Result(st|>getI(leftValue) & st|>getI(expr_and(st)))
    return <- leftValue

def expr_xor ( var st:TokenStream ) : Result
    var leftValue = expr_and(st)
    if try_accept_punkt(st, '^')
        if try_accept_punkt(st, '^')
            return Result((st|>getI(leftValue) ^ st|>getI(expr_xor(st))) != 0l)
        else
            return Result(st|>getI(leftValue) ^ st|>getI(expr_xor(st)))
    return <- leftValue

def expr_or ( var st:TokenStream ) : Result
    var leftValue = expr_xor(st)
    if try_accept_punkt(st, '|')
        if try_accept_punkt(st, '|')
            return Result((st|>getI(leftValue) | st|>getI(expr_or(st))) != 0l)
        else
            return Result(st|>getI(leftValue) | st|>getI(expr_or(st)))
    return <- leftValue

def expr_bool_op ( var st:TokenStream ) : Result
    var leftValue = expr_or(st)
    let token = token(st)
    let subtoken = token(st)
    match token
        if [[Token punkt='!']] && (subtoken ?as punkt ?? 0 == '=')
            return Result(st|>getT(leftValue) != st|>getT(expr_bool_op(st)))
        if [[Token punkt='=']] && (subtoken ?as punkt ?? 0 == '=')
            return Result(st|>getT(leftValue) == st|>getT(expr_bool_op(st)))
        if [[Token punkt='<']] && (subtoken ?as punkt ?? 0 == '=')
            return Result(st|>getI(leftValue) <= st|>getI(expr_bool_op(st)))
        if [[Token punkt='<']]
            unput(st, subtoken)
            return Result(st|>getI(leftValue) < st|>getI(expr_bool_op(st)))
        if [[Token punkt='>']] && (subtoken ?as punkt ?? 0 == '=')
            return Result(st|>getI(leftValue) >= st|>getI(expr_bool_op(st)))
        if [[Token punkt='>']]
            unput(st, subtoken)
            return Result(st|>getI(leftValue) > st|>getI(expr_bool_op(st)))
        if _
            unput(st, subtoken)
            unput(st, token)
    return <- leftValue

def expr ( var st:TokenStream ) : Result
    return expr_bool_op(st)

def public debug_eval ( context:table<string;Result>; expr:string ) : Result
    var st <- TokenStream(expr)
    st.names := context
    var result = expr(st)
    let token = token(st)
    if !(token is eos)
        return [[Result error="unexpected token {token}"]]
    if !empty(st.errors)
        return [[Result error=st.errors[0]]]
    return <- result

