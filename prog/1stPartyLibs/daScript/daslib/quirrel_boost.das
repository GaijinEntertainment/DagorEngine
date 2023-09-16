options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers
options no_aot = true

module quirrel_boost shared private

require ast
require rtti
require strings public
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require quirrel public


def public getStackValue(var vm : SQVM?; idx, nargs : int64; cb : block<(res : auto(T)) : void>)
    if idx < nargs
        return _::getStackValue(vm, idx, cb)
    cb |> invoke(null)
    return true


def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : auto(T)?) : void>)
    let itemType = sq_gettype(vm, idx)
    if itemType != tagSQObjectType OT_NULL
        return _::getStackValue(vm, idx) <| $(res2 : T)
            cb |> invoke(unsafe(reinterpret<T? const> unsafe(addr(res2))))
    else
        cb |> invoke(null)
        return true
    return false


def public getStackIntValue(var vm : SQVM?; idx : int64; cb : block<(res : auto(T)) : void>)
    var res = 0l
    if sq_getinteger(vm, idx, unsafe(addr(res))) >= 0l
        cb |> invoke(T(res))
        return true
    return false

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : int) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : int8) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : int16) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : int64) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : uint) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : uint8) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : uint16) : void>)
    return getStackIntValue(vm, idx, cb)
def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : uint64) : void>)
    return getStackIntValue(vm, idx, cb)

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : bool) : void>)
    var res = 0ul
    if sq_getbool(vm, idx, unsafe(addr(res))) >= 0l
        let resT = res != 0ul
        cb |> invoke(resT)
        return true
    return false

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : float) : void>)
    var res = 0.
    if sq_getfloat(vm, idx, unsafe(addr(res))) >= 0l
        cb |> invoke(res)
        return true
    return false

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : string) : void>)
    var res : string
    let itemType = sq_gettype(vm, idx)
    if itemType == tagSQObjectType OT_STRING
        if sq_getstring(vm, idx, unsafe(addr(res))) >= 0l
            cb |> invoke(unsafe(reinterpret<string> res))
            return true
    elif itemType == tagSQObjectType OT_NULL
        cb |> invoke(unsafe(reinterpret<string> res))
        return true
    return false

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : array<auto(T)>) : void>)
    var res : array<T>
    var runtimeError = false
    let len = sq_getsize(vm, idx)
    var i = 0l
    while !runtimeError && i < len
        sq_pushinteger(vm, i)
        sq_rawget(vm, idx)
        if !_::getStackValue(vm, -1l, $(v : T) { res |> push(v); })
            let itemType = sq_gettype(vm, -1l)
            to_log(LOG_ERROR, "Unable to read [{i}] item. Expected {typeinfo(typename type<T>)}, got {itemType}")
            runtimeError = true
            return false
        sq_pop(vm, 1l);
        i += 1l

    if !runtimeError
        cb |> invoke(res)
    delete res
    return !runtimeError

def public getStackValue(var vm : SQVM?; idx : int64; cb : block<(res : table<auto(K); auto(V)>) : void>)
    var res : table<K; V>
    var runtimeError = false
    sq_pushnull(vm)
    while !runtimeError && sq_next(vm, idx) >= 0l
        let hasKey = _::getStackValue(vm, -2l) <| $(k : K)
            let hasValue = _::getStackValue(vm, -1l) <| $(v : V)
                res[k] = v // TODO: emplace/copy
            if !hasValue
                let valueType = sq_gettype(vm, -1l)
                to_log(LOG_ERROR, "Unable to read table item at key '{k}'. Expected {typeinfo(typename type<V>)}, got {valueType}")
                runtimeError = true
        if !hasKey
            let keyType = sq_gettype(vm, -2l)
            to_log(LOG_ERROR, "Unable to read table key. Expected {typeinfo(typename type<K>)}, got {keyType}")
            runtimeError = true
        sq_pop(vm, 2l) // pop key and value
    sq_pop(vm, 1l) // pop iterator

    if !runtimeError
        cb |> invoke(res)

    delete res
    return !runtimeError

def public pushValue(var vm : SQVM?; val : int) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : int8) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : int16) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : int64) { sq_pushinteger(vm, val); }
def public pushValue(var vm : SQVM?; val : uint) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : uint8) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : uint16) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : uint64) { sq_pushinteger(vm, int64(val)); }
def public pushValue(var vm : SQVM?; val : float) { sq_pushfloat(vm, val); }
def public pushValue(var vm : SQVM?; val : bool) { sq_pushbool(vm, val ? 1ul : 0ul); }
def public pushValue(var vm : SQVM?; val : string) { sq_pushstring(vm, val, int64(length(val))); }

def public pushValue(var vm : SQVM?; val : array<auto(T)>)
    sq_newarray(vm, int64(length(val)))
    for it, idx in val, iter_range(val)
        _::pushValue(vm, idx)
        _::pushValue(vm, it)
        sq_rawset(vm, -3l)

def public pushValue(var vm : SQVM?; val : table<auto(K); auto(V)>)
    sq_newtable(vm)
    for k, v in keys(val), values(val)
        _::pushValue(vm, k)
        _::pushValue(vm, v)
        sq_rawset(vm, -3l)

// https://quirrel.io/doc/reference/api/object_creation_and_handling.html?highlight=sq_setparamscheck#c.sq_setparamscheck
// 'o' null                         'u' userdata
// 'i' integer                      'p' userpointer
// 'f' float                        'c' closure and nativeclosure
// 'n' integer or float             'g' generator
// 'b' bool                         'v' thread
// 's' string                       'x' instance(class instance)
// 't' table                        'y' class
// 'a' array                        '.' any type
[macro_function]
def public paramcheck(typ)
    return "i" if (typ.baseType == Type tInt || typ.baseType == Type tInt8 || typ.baseType == Type tInt16
                || typ.baseType == Type tInt64 || typ.baseType == Type tUInt || typ.baseType == Type tUInt8
                || typ.baseType == Type tUInt16 || typ.baseType == Type tUInt64
                || typ.baseType == Type tEnumeration || typ.baseType == Type tEnumeration8 || typ.baseType == Type tEnumeration16
                || typ.baseType == Type tVariant || typ.baseType == Type tBitfield)
    return "n" if typ.baseType == Type tFloat
    return "b" if typ.baseType == Type tBool
    return "s|o" if typ.baseType == Type tString
    return "a" if typ.baseType == Type tArray
    return "t" if typ.baseType == Type tTable
    return "x" if typ.baseType == Type tHandle && typ.annotation.isRefType
    return "o|{paramcheck(typ.firstType)}" if typ.baseType == Type tPointer
    return "." // any type

[macro_function]
def read_arg(var arg : smart_ptr<ast::Variable>&; vmIdx : int64; argName : string; optional_args: bool; var errors : das_string;
             var funcBody : array<ExpressionPtr>; var paramsCheck : array<string>; var nullable_arg, vm_arg : bool&) : array<ExpressionPtr>
    nullable_arg = false
    vm_arg = false
    var definedParamCheck = ""
    for ann in arg.annotation
        if ann.name == "param_check"
            if ann.basicType != Type tString
                errors := "{errors}\nuse incorrect param_check annotation type, string required ([[param_check=<paramCheckName>...]] {arg.name}...)"
            else
                definedParamCheck := ann.sValue
        elif ann.name == "raw_param"
            if arg._type.baseType != Type tInt64
                errors := "{errors}\nuse int64 type for argument with raw_param annotation ([[raw_param={ann.sValue}...]] {arg.name} : int64)"
            if ann.basicType != Type tString
                errors := "{errors}\nuse incorrect raw_param annotation type, string required ([[raw_param=<paramCheckName>...]] {arg.name} : int64)"
            else
                var inscope expr <- qmacro_expr(${let $i(argName) = $v(vmIdx);})
                funcBody |> emplace(expr, 0)
                paramsCheck |> push("{ann.sValue}", 0)
            return <- funcBody

    if arg._type.baseType == Type tPointer
        if arg._type.firstType.annotation != null && arg._type.firstType.annotation.name == "SQVM"
            vm_arg = true
            var inscope expr <- qmacro_expr(${var $i(argName) = _vm_;})
            funcBody |> emplace(expr, 0)
            return <- funcBody
        else
            nullable_arg = true
            if arg._type.isConst && !arg._type.isRefType && !arg._type.firstType.isConst
                errors := "{errors}\nargument {arg.name} which is constant pointer should point to constant data\nAdd const before question mark ('int const?)"

    var inscope res : array<ExpressionPtr>
    if nullable_arg && optional_args
        var inscope reader <- qmacro_block <|
            if !(_::getStackValue(_vm_, $v(vmIdx), _nargs_) <| $($i(argName) : $t(arg._type)) { $b(funcBody); })
                let valueType = sq_gettype(_vm_, $v(vmIdx))
                sq_throwerror(_vm_, "Unable to read argument '{$v(argName)}'. Expected '{$v(describe(arg._type))}, got {valueType}'")
                _runtime_error_ = true
        res |> emplace(reader)
    else
        var inscope reader <- qmacro_block <|
            if !(_::getStackValue(_vm_, $v(vmIdx)) <| $($i(argName) : $t(arg._type)) { $b(funcBody); })
                let valueType = sq_gettype(_vm_, $v(vmIdx))
                sq_throwerror(_vm_, "Unable to read argument '{$v(argName)}'. Expected '{$v(describe(arg._type))}, got {valueType}'")
                _runtime_error_ = true
        res |> emplace(reader)
    paramsCheck |> push(empty(definedParamCheck) ? paramcheck(arg._type) : definedParamCheck, 0)
    return <- res


[function_macro(name="quirrel_wrapper")]
class QuirrelCallAnnotation : AstFunctionAnnotation
    def override complete ( var func:FunctionPtr; var ctx:smart_ptr<Context> ) : void
        if is_in_aot() || is_in_completion()
            return
        for ann in func.annotations
            if ann.annotation.name == "quirrel_wrapper"
                let nameArg = ann.arguments |> find_arg("name")
                let moduleArg = ann.arguments |> find_arg("module_name")
                let paramsCheckArg = ann.arguments |> find_arg("params_check")
                let paramsArg = ann.arguments |> find_arg("params")
                let mn = get_mangled_name(func)
                let mnh = hash(mn)
                sqdas_bind_func(*get_ptr(ctx), mnh, "{nameArg}", "{moduleArg}", paramsArg as tInt, "{paramsCheckArg}")
                print("{ctx.name}: link quirrel binding '{moduleArg}.{nameArg}' with {paramsArg as tInt} params ({paramsCheckArg}) hash: {mnh}\n")


[function_macro(name="quirrel_bind")]
class QuirrelBindAnnotation : AstFunctionAnnotation
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if func.isGeneric
            errors := "generic function. Define all arguments types"
            return false
        return true

    def override patch ( var fn:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool
        let bindedArg = args |> find_arg("binded")
        if bindedArg is tBool // function was already binded
            return true

        let nameArg = args |> find_arg("name")
        if !(nameArg is nothing) && !(nameArg is tString)
            errors := "annotation argument 'name' expecting string [quirrel_bind(name=<bind name>)]"
            return false

        let moduleArg = args |> find_arg("module_name")
        if !(moduleArg is nothing) && !(moduleArg is tString)
            errors := "annotation argument 'module_name' expecting string [quirrel_bind(module_name=<mod name>)]"
            return false

        let voidResult = fn.result.baseType == Type tVoid

        let rawReturnArg = args |> find_arg("raw_return")
        if !(rawReturnArg is nothing) && !(rawReturnArg is tBool)
            errors := "annotation argument 'raw_return' expecting bool [quirrel_bind(raw_return)] or [quirrel_bind(raw_return=true)]"
            return false

        let isManualReturn = rawReturnArg ?as tBool ?? false
        if !voidResult && isManualReturn
            errors := "function '{fn.name}(...) : {describe(fn.result)}' already has return result, 'raw_return' annotation is redundant"
            return false

        if fn._module.moduleFlags.isModule
            errors := "quirrel_bind shouldn't be placed in the module. Please move the function to a file without module directive";
            return false

        let bindName = nameArg is nothing ? string(fn.name) : nameArg as tString
        let wrapperName = "wrapper_{bindName}"

        var inscope sameWrapper <- fn._module |> find_unique_function(wrapperName)
        if sameWrapper != null
            errors := "multiple functions with same bind name '{bindName}'"
            return false

        var inscope argsList : array<ExpressionPtr>
        var inscope funcBody : array<ExpressionPtr>
        var inscope argsLog : array<string>
        let INITIAL_IDX_OFFSET = 2l
        var vmIdx = INITIAL_IDX_OFFSET
        for arg in fn.arguments
            let argName = "{arg.name}"
            argsList |> emplace_new <| qmacro($i(argName))
            argsLog |> push("{argName} : {describe(arg._type)}")
            vmIdx += 1l

        let fnName = "::{fn.name}"
        if voidResult
            funcBody |> emplace_new <| qmacro_expr(${ $c(fnName)($a(argsList)); })
        else
            funcBody |> emplace_new <| qmacro_expr(${ _::pushValue(_vm_, $c(fnName)($a(argsList))); })

        var argIdx = length(fn.arguments) - 1
        var inscope paramsCheck : array<string>
        var optionalArgs = true
        var optionalArgsNum = 0
        while argIdx >= 0
            if true
                assume arg = fn.arguments[argIdx]
                let argName = "{arg.name}"
                var nullableArg = false
                var vmArg = false
                var inscope newFuncBody <- read_arg(arg, int64(argIdx) + INITIAL_IDX_OFFSET, argName, optionalArgs, errors, funcBody, paramsCheck, nullableArg, vmArg)
                unsafe
                    delete funcBody
                    funcBody <- newFuncBody
                if !vmArg
                    optionalArgs &&= nullableArg
                    optionalArgsNum += optionalArgs ? 1 : 0
                argIdx -= 1
        paramsCheck |> push(".", 0)

        if length(errors) > 0
            return false

        for ann in fn.annotations
            if ann.annotation.name=="quirrel_bind"
                astChanged = true
                ann.arguments |> add_annotation_argument("binded", true)

        if length(fn.arguments) > 0
            if optionalArgsNum > 0
                var inscope nArgsExpr <- qmacro_expr(${ var _nargs_ = sq_gettop(_vm_); })
                funcBody |> emplace(nArgsExpr, 0)
            var inscope errorsExpr <- qmacro_expr(${ var _runtime_error_ = false; })
            funcBody |> emplace(errorsExpr, 0)
            funcBody |> emplace_new <| qmacro_expr(${ return _runtime_error_ ? -1l : $v(voidResult && !isManualReturn ? 0l : 1l); })
        else
            funcBody |> emplace_new <| qmacro_expr(${ return $v(voidResult && !isManualReturn ? 0l : 1l); })

        var inscope bindFn <- qmacro_function(wrapperName) <| $ [unused_argument(_vm_)] (var _vm_ : quirrel::SQVM?) : int64
            $b(funcBody)

        bindFn.body |> force_at(fn.body.at)

        bindFn.flags |= FunctionFlags exports | FunctionFlags privateFunction | FunctionFlags generated
        let moduleName = moduleArg ?as tString ?? "dasbind"
        bindFn |> append_annotation("quirrel_boost", "quirrel_wrapper", [{auto
            [[auto "name", [[RttiValue tString="{bindName}"]] ]];
            [[auto "module_name", [[RttiValue tString=moduleName ]] ]];
            [[auto "params_check", [[RttiValue tString="{join(paramsCheck, "")}"]] ]];
            [[auto "params", [[RttiValue tInt=optionalArgsNum == 0 ? length(paramsCheck) : -length(paramsCheck)+optionalArgsNum]] ]]
        }])
        fn._module |> add_function(bindFn)
        if !(is_in_aot() || is_in_completion())
            print("{describe(fn.at)}: bind 'def {fn.name}(...)' to quirrel {moduleName}.{bindName}({join(argsLog, ", ")})\n")

        return true
