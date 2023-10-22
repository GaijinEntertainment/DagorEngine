options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module jobque_boost shared public

require jobque public

require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/templates
require daslib/macro_boost

[tag_function(new_job_tag)]
def new_job ( var l : lambda )
    //! Create a new job.
    //!     * new context is cloned from the current context.
    //!     * lambda is cloned to the new context.
    //!     * new job is added to the job queue.
    //!     * once new job is invoked, lambda is invoked on the new context on the job thread.
    invoke(l)   // note, this is never called if job-que is there.

[tag_function(new_job_tag)]
def new_thread ( var l : lambda )
    //! Create a new thread
    //!     * new context is cloned from the current context.
    //!     * lambda is cloned to the new context.
    //!     * new thread is created.
    //!     * lambda is invoked on the new context on the new thread.
    invoke(l)   // note, this is never called if job-que is there

[tag_function_macro(tag="new_job_tag")]
class private NewJobMacro : AstFunctionAnnotation
    //! this macro handles `new_job` and `new_thread` calls.
    //! the call is replaced with `new_job_invoke` and `new_thread_invoke` accordingly.
    //! a cloning infastructure is generated for the lambda, which is invoked in the new context.
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        macro_verify(call.arguments[0] is ExprAscend,compiling_program(),call.at,"expecting lambda declaration, ExprAscend")
        var asc = call.arguments[0] as ExprAscend
        macro_verify(asc.subexpr is ExprMakeStruct,compiling_program(),call.at,"expecting lambda declaration, ExprMakeStruct")
        var mks = asc.subexpr as ExprMakeStruct
        macro_verify(!(mks._type==null || mks._type.baseType!=Type tStructure),compiling_program(),call.at,"expecting lambda declaration, not a structure")
        // clone structure type. make fields non-constant, so that they can be cloned
        var inscope stype <- clone_structure(mks._type.structType)
        stype.name := "{stype.name}_new_job_clone"
        var stype_ptr = get_ptr(stype)
        var inscope sttype <- new [[TypeDecl() at=call.at, baseType=Type tStructure, structType = stype_ptr]]
        for fld in stype.fields // TODO: verify field type here
            fld._type.flags &= ~(TypeDeclFlags constant)
            fld.flags &= ~(FieldDeclarationFlags capturedConstant)
        var inscope pclone <- make_clone_structure(stype_ptr)
        compiling_module() |> add_function(pclone)
        compiling_module() |> add_structure(stype)
        // make an @@<function<(var L;L):void> type
        var inscope ftype <- new [[TypeDecl() at=call.at, baseType=Type tFunction ]]
        move_new(ftype.firstType) <| new [[TypeDecl() at=call.at, baseType=Type tVoid]]
        ftype.argTypes |> emplace_new <| clone_type(sttype)
        ftype.argTypes |> emplace <| sttype
        ftype.argTypes[1].flags |= TypeDeclFlags constant
        // make a new_job_invoke call
        var inscope ncall <- new [[ExprCall() at=call.at, name:="{call.name}_invoke"]]
        ncall.arguments |> emplace_new <| clone_expression(call.arguments[0])
        ncall.arguments |> emplace_new <| new [[ExprAddr() at=call.at, target:="clone", funcType <- ftype]]
        ncall.arguments |> emplace_new <| new [[ExprConstInt() at=call.at, value=int(mks._type.sizeOf)]]
        return <- ncall

def gather ( ch:Channel?; blk:block<(arg:auto(TT)#):void>)
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather(ch) <| $ ( var data : void?  )
        var pv = unsafe(reinterpret<int?> data)
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))

def gather_ex ( ch:Channel?; blk:block<(arg:auto(TT)#; info:TypeInfo const?; var ctx:Context):void>)
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather_ex(ch) <| $ ( var data : void?; info:TypeInfo const?; var ctx:Context )
        var pv = unsafe(reinterpret<int?> data)
        invoke(blk, *(unsafe(reinterpret<TT?#> data)), info, ctx)

def gather_and_forward ( ch,toCh:Channel?; blk:block<(arg:auto(TT)#):void>)
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather_and_forward(ch,toCh) <| $ ( var data : void?  )
        var pv = unsafe(reinterpret<int?> data)
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))

def peek ( ch:Channel?; blk:block<(arg:auto(TT)#):void>)
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is not consumed
    _builtin_channel_peek(ch) <| $ ( var data : void?  )
        var pv = unsafe(reinterpret<int?> data)
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))


[deprecated(message="use `for_each_clone` instead")]
def for_each ( channel:Channel?; blk:block<(res:auto(TT)#):void> )
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! stops once channel is depleted (internal entry counter is 0)
    //! this can happen on multiple threads or jobs at the same time.
    while true
        var void_data : void?
        _builtin_channel_pop(channel) <| $ ( vd )
            void_data = vd
        if void_data==null
            break
        unsafe
            let typed_data = reinterpret<TT?#> void_data
            invoke ( blk, *typed_data )

def for_each_clone ( channel:Channel?; blk:block<(res:auto(TT)#):void> )
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! stops once channel is depleted (internal entry counter is 0)
    //! this can happen on multiple threads or jobs at the same time.
    while true
        var temp : TT-#-&-const
        var any = false
        _builtin_channel_pop(channel) <| $ ( vd )
            if vd != null
                any = true
                temp := *(unsafe(reinterpret<TT-#-&-const?#> vd))
        if !any
            break
        invoke ( blk, unsafe(reinterpret<TT-&-const#>temp) )
        delete temp

[deprecated(message="use `pop_and_clone_one` instead")]
def pop_one ( channel:Channel?; blk:block<(res:auto(TT)#):void> )
    //! reads one command from channel
    var void_data : void?
    _builtin_channel_pop(channel) <| $ ( vd )
        void_data = vd
    if void_data==null
        return false
    unsafe
        let typed_data = reinterpret<TT?#> void_data
        invoke ( blk, *typed_data )
        return true

def pop_and_clone_one ( channel:Channel?; blk:block<(res:auto(TT)#):void> )
    //! reads one command from channel
    var any = false
    var temp : TT-#-&-const
    _builtin_channel_pop(channel) <| $ ( vd )
        if vd != null
            any = true
            temp := *(unsafe(reinterpret<TT-#-&-const?#> vd))
    if !any
        return false
    invoke ( blk, unsafe(reinterpret<TT-&-const#> temp) )
    delete temp
    return true

def push_clone ( channel:Channel?; data : auto(TT) )
    //! clones data and pushed value to the channel (at the end)
    var heap_data = new TT
    *heap_data := data
    _builtin_channel_push(channel, heap_data)

def push ( channel:Channel?; data : auto? )
    //! pushes value to the channel (at the end)
    _builtin_channel_push(channel, data)

def push_batch_clone ( channel:Channel?; data : array<auto(TT)> )
    //! clones data and pushed values to the channel (at the end)
    var heap_data : array<TT?>
    for d in data
        var tt = new TT
        *tt := d
        heap_data |> push(tt)
    _builtin_channel_push(channel, heap_data)
    heap_data |> resize(0)
    unsafe
        delete heap_data

def push_batch ( channel:Channel?; data : array<auto?> )
    //! pushes values to the channel (at the end)
    _builtin_channel_push_batch(channel, data)

def set ( box:LockBox?; data : auto(TT) )
    //! clones data and sets value to the lock box
    var heap_data = new TT
    *heap_data := data
    _builtin_lockbox_set(box, heap_data)

def set ( box:LockBox?; data : auto? )
    //! sets value to the lock box
    _builtin_lockbox_set(box, data)

def get ( box:LockBox?; blk:block<(res:auto(TT)#):void> )
    //! reads value from the lock box and invokes the block on it
    var res = false
    _builtin_lockbox_get(box) <| $ ( void_data )
        if void_data!=null
            let typed_data = unsafe(reinterpret<TT?#> void_data)
            invoke ( blk, *typed_data )
            res = true
    return res

def update ( box:LockBox?; blk:block<(var res:auto(TT)#):void> )
    //! update value in the lock box and invokes the block on it
    _builtin_lockbox_update(box, unsafe(reinterpret<TypeInfo?>(typeinfo(rtti_typeinfo type<TT? -#>)))) <| $ ( var void_data : void? )
        if void_data!=null
            var typed_data = unsafe(reinterpret<TT?#> void_data)
            invoke ( blk, *typed_data )
            return void_data
        else
            var heap_data = new TT
            invoke ( blk, *heap_data )
            return unsafe(reinterpret<void?> heap_data)

[template(type_)]
def clear ( box:LockBox? ; type_ : auto(TT) )
    //! clear value from the lock box
    _builtin_lockbox_update(box, unsafe(reinterpret<TypeInfo?>(typeinfo(rtti_typeinfo type<TT? -#>)))) <| $ ( var void_data : void? ) : void?
        if void_data!=null
            var typed_data = unsafe(reinterpret<TT -const?> void_data)
            unsafe
                delete typed_data
        return null


[template (tinfo),deprecated(message="use `each_clone` instead")]
def each ( var channel:Channel?; tinfo : auto(TT) )
    //! this iterator is used to iterate over the channel in order it was pushed.
    //! iterator stops once channel is depleted (internal entry counter is 0)
    //! iteration can happen on multiple threads or jobs at the same time.
    unsafe
        return <- generator<TT&#> () <| $ ()
            while true
                var void_data : void?
                _builtin_channel_pop(channel) <| $ ( vd )
                    void_data = vd
                if void_data==null
                    break
                unsafe
                    yield * reinterpret<TT?#> void_data
            return false
        finally
            channel |> release

def each_clone ( var channel:Channel?; tinfo : auto(TT) )
    //! this iterator is used to iterate over the channel in order it was pushed.
    //! iterator stops once channel is depleted (internal entry counter is 0)
    //! iteration can happen on multiple threads or jobs at the same time.
    unsafe
        return <- generator<TT&#> () <| $ ()
            while true
                var temp : TT-#-&-const
                var any = false
                _builtin_channel_pop(channel) <| $ ( vd )
                    if vd != null
                        any = true
                        temp := *(unsafe(reinterpret<TT-#-&-const?#> vd))
                if !any
                    break
                yield unsafe(reinterpret<TT-&-const#>temp)
                delete temp
            return false
        finally
            channel |> release

def public capture_jobque_channel ( ch:Channel? ) : Channel?
    //! this function is used to capture a channel that is used by the jobque.
    ch |> add_ref
    return ch

def public capture_jobque_job_status ( js:JobStatus? ) : JobStatus?
    //! this function is used to capture a job status that is used by the jobque.
    js |> add_ref
    return js

def public capture_jobque_lock_box ( js:LockBox? ) : LockBox?
    //! this function is used to capture a lock box that is used by the jobque.
    js |> add_ref
    return js

def public release_capture_jobque_channel ( ch:Channel? )
    //! this function is used to release a channel that is used by the jobque.
    if ch != null
        panic("Channel has not been released. missing channel|>release or channel|>notify_and_release")

def public release_capture_jobque_job_status ( js:JobStatus? )
    //! this function is used to release a job status that is used by the jobque.
    if js != null
        panic("JobStatus has not been released. missing status|>release or status|>notify_and_release")

def public release_capture_jobque_lock_box ( js:LockBox? )
    //! this function is used to release a lock box that is used by the jobque.
    if js != null
        panic("LockBox has not been released. missing status|>release or status|>notify_and_release")


[macro_function]
def private isPtrToJQ ( typ:TypeDeclPtr; jqt:string ) : bool
    if !typ.isPointer
        return false
    if typ.firstType==null
        return false
    if !typ.firstType.isHandle
        return false
    if typ.firstType.annotation._module.name!="jobque"
        return false
    if typ.firstType.annotation.name!=jqt
        return false
    return true

[capture_macro(name=channel_and_status_capture_macro)]
class private ChannelAndStatusCapture : AstCaptureMacro
    def make_capture_call (expr:ExpressionPtr; name:string): ExpressionPtr
        var inscope pCall <- new [[ExprCall() at=expr.at, name:=name]]
        pCall.arguments |> emplace_new <| clone_expression(expr)
        return <- pCall
    def make_release_call (fld:FieldDeclaration; name:string): ExpressionPtr
        var inscope pCall <- new [[ExprCall() at=fld.at, name:=name]]
        pCall.arguments |> emplace_new <| new [[ExprField() at=fld.at,
            value <- new [[ExprVar() at=fld.at, name:="__this"]],
            name := fld.name,
            derefFlags = ExprFieldDerefFlags ignoreCaptureConst
        ]]
        return <- pCall
        //! This macro implements capturing of the `jobque::Channel` and `jobque::JobStatus` types.
    //! When captured reference counts are increased. When lambda is destroyed, reference counts are decreased.
    def override captureExpression ( prog:Program?; mod:Module?; expr:ExpressionPtr; typ:TypeDeclPtr ) : ExpressionPtr
        //! Implementation details for the capture macro.
        if typ |> isPtrToJQ("Channel")
            return <- make_capture_call(expr,"jobque_boost::capture_jobque_channel")
        elif typ |> isPtrToJQ("JobStatus")
            return <- make_capture_call(expr,"jobque_boost::capture_jobque_job_status")
        elif typ |> isPtrToJQ("LockBox")
            return <- make_capture_call(expr,"jobque_boost::capture_jobque_lock_box")
        return <- [[ExpressionPtr]]
    def override captureFunction ( prog:Program?; mod:Module?; var lcs:Structure?; var fun:FunctionPtr ) : void
        //! Implementation details for the capture macro.
        if fun.flags._generator // generator gets finally section called every single time. nothing we can do
            return
        for fld in lcs.fields
            if fld._type |> isPtrToJQ("Channel")
                var inscope pCall <- make_release_call(fld,"jobque_boost::release_capture_jobque_channel")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            elif fld._type |> isPtrToJQ("JobStatus")
                var inscope pCall <- make_release_call(fld,"jobque_boost::release_capture_jobque_job_status")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            elif fld._type |> isPtrToJQ("LockBox")
                var inscope pCall <- make_release_call(fld,"jobque_boost::release_capture_jobque_lock_box")
                (fun.body as ExprBlock).finalList |> emplace(pCall)


