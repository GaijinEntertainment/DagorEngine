

// this breaks safety. i.e. a:array<FOO>; temp_addr(a[0]); resize(a,100500)

[tag_function(temp_addr_tag)]
def public temp_addr(var x : auto(T)& ==const) : T-& ? #
    //! returns temporary pointer to the given expression
    unsafe
        return reinterpret<T-&?#>(addr(x))

[tag_function(temp_addr_tag)]
def public temp_addr(x : auto(T)& ==const) : T-& ? const #
    unsafe
        return reinterpret<T-&? const #>(addr(x))

[tag_function_macro(tag="temp_addr_tag")]
class TempAddrMacro : AstFunctionAnnotation
    //! This macro reports an error if temp_addr is attempted outside of function arguments.
    def override verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        if call.flags.isForLoopSource || call.flags.isCallArgument
            return true
        compiling_program() |> macro_error(call.at,"{call.name} can only be used in function arguments or for loop source")
        return false

