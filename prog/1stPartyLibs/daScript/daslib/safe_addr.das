options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module safe_addr shared private

require ast
require rtti
require daslib/ast_boost
require daslib/is_local

[tag_function(safe_addr_tag)]
def public safe_addr(var x : auto(T)& ==const) : T-& ? #
    //! returns temporary pointer to the given expression
    unsafe
        return reinterpret<T-&?#>(addr(x))

[tag_function(safe_addr_tag)]
def public safe_addr(x : auto(T)& ==const) : T-& ? const #
    unsafe
        return reinterpret<T-&? const #>(addr(x))

[tag_function_macro(tag="safe_addr_tag")]
class SafeAddrMacro : AstFunctionAnnotation
    //! This macro reports an error if safe_addr is attempted on the object, which is not local to the scope.
    //! I.e. if the object can `expire` while in scope, with delete, garbage collection, or on the C++ side.
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        if !is_local_or_global_expr(ExpressionPtr(call.arguments[0]))
            compiling_program() |> macro_error(call.at,"not a local value")
        return <- [[ExpressionPtr]]

// [tag_function(temp_addr_tag)]
def public temp_ptr(x : auto(T)? implicit ==const) : T ? const #
    //! returns temporary pointer from a given pointer
    unsafe
        return reinterpret<T? const #>(x)

// [tag_function(temp_addr_tag)]
def public temp_ptr(var x : auto(T)? implicit ==const) : T ?#
    unsafe
        return reinterpret<T?#>(x)

[tag_function(shared_addr_tag)]
def public shared_addr ( tab:table<auto(KEY); auto(VAL)>; k:KEY )
    return unsafe(find(tab,k))

[tag_function(shared_addr_tag)]
def public shared_addr ( val:auto(VALUE)& )
    //! returns address of the given shared variable. it's safe because shared variables never go out of scope
    return unsafe(addr(val))

[tag_function_macro(tag="shared_addr_tag")]
class SharedAddrMacro : AstFunctionAnnotation
    //! This macro reports an error if shared_addr is attempted on anything other that shared global variables.
    //! I.e. only global variables are safe to use with shared_addr.
    def override verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        if is_shared_expr(ExpressionPtr(call.arguments[0]))
            return true
        compiling_program() |> macro_error(call.at,"{call.name} can only be used with shared global variables")
        return false


/*

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
*/