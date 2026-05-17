options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module jobque_boost shared public

//! Job queue macro helpers.
//!
//! Provides ``new_job``, ``new_thread``, ``with_job_que``,
//! ``with_job_status``, ``channel_next``, ``channel_push``, and
//! other convenience wrappers that capture lambda closures and
//! correctly manage context cloning for multi-threaded execution.
//! Extends the built-in ``jobque`` module.

require jobque public

require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/templates
require daslib/macro_boost
require daslib/templates_boost

[tag_function(new_job_tag)]
def new_job(var l : lambda) {
    //! Create a new job.
    //!     * new context is cloned from the current context.
    //!     * lambda is cloned to the new context.
    //!     * new job is added to the job queue.
    //!     * once new job is invoked, lambda is invoked on the new context on the job thread.
    invoke(l)   // note, this is never called if job-que is there.
}

[tag_function(new_job_tag)]
def new_thread(var l : lambda) {
    //! Create a new thread
    //!     * new context is cloned from the current context.
    //!     * lambda is cloned to the new context.
    //!     * new thread is created.
    //!     * lambda is invoked on the new context on the new thread.
    invoke(l)   // note, this is never called if job-que is there
}

[tag_function_macro(tag="new_job_tag")]
class private NewJobMacro : AstFunctionAnnotation {
    //! this macro handles `new_job` and `new_thread` calls.
    //! the call is replaced with `new_job_invoke` and `new_thread_invoke` accordingly.
    //! a cloning infrastructure is generated for the lambda, which is invoked in the new context.
    def override transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr {
        //! Transforms the new_job or new_thread call by generating lambda cloning infrastructure.
        macro_verify(call.arguments[0] is ExprAscend, compiling_program(), call.at, "expecting lambda declaration, ExprAscend")
        var asc = call.arguments[0] as ExprAscend
        macro_verify(asc.subexpr is ExprMakeStruct, compiling_program(), call.at, "expecting lambda declaration, ExprMakeStruct")
        var mks = asc.subexpr as ExprMakeStruct
        macro_verify(!(mks._type == null || mks._type.baseType != Type.tStructure), compiling_program(), call.at, "expecting lambda declaration, not a structure")
        // clone structure type. make fields non-constant, so that they can be cloned
        var stype <- clone_structure(mks._type.structType)
        stype.name := "{stype.name}_new_job_clone"
        var stype_ptr = stype
        var sttype = new TypeDecl(at = call.at, baseType = Type.tStructure, structType = stype_ptr)
        for (fld in stype.fields) {// TODO: verify field type here
            fld._type.flags &= ~(TypeDeclFlags.constant)
            fld.flags &= ~(FieldDeclarationFlags.capturedConstant)
        }
        var pclone = make_clone_structure(stype_ptr)
        let const_clone = pclone.arguments[1]._type.flags.constant
        compiling_module() |> add_function(pclone)
        compiling_module() |> add_structure(stype)
        // make an @@<function<(var L;L):void> type or @@<function<(var L;var L):void> type - depending on how the capture went
        var ftype = new TypeDecl(at = call.at, baseType = Type.tFunction)
        ftype.firstType = new TypeDecl(at = call.at, baseType = Type.tVoid)
        ftype.argTypes |> emplace_new <| clone_type(sttype)
        ftype.argTypes |> emplace <| sttype
        if (const_clone) {
            ftype.argTypes[1].flags |= TypeDeclFlags.constant
        }
        // make a new_job_invoke call
        var ncall = new ExprCall(at = call.at, name := "{call.name}_invoke")
        ncall.arguments |> emplace_new <| clone_expression(call.arguments[0])
        ncall.arguments |> emplace_new <| new ExprAddr(at = call.at, target := "clone", funcType <- ftype)
        ncall.arguments |> emplace_new <| new ExprConstInt(at = call.at, value = int(mks._type.sizeOf))
        return <- ncall
    }
}

def gather(ch : Channel?; blk : block<(arg : auto(TT)#) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather(ch) $(var data : void?) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))
    }
}

def gather_ex(ch : Channel?; blk : block<(arg : auto(TT)#; info : TypeInfo const?; var ctx : Context) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather_ex(ch) $(var data : void?; info : TypeInfo const?; var ctx : Context) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)), info, ctx)
    }
}

def gather_and_forward(ch, toCh : Channel?; blk : block<(arg : auto(TT)#) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is consumed
    _builtin_channel_gather_and_forward(ch, toCh) $(var data : void?) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))
    }
}

def peek(ch : Channel?; blk : block<(arg : auto(TT)#) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! afterwards input is not consumed
    _builtin_channel_peek(ch) $(var data : void?) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))
    }
}


[deprecated(message="use `for_each_clone` instead")]
def for_each(channel : Channel?; blk : block<(res : auto(TT)#) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! stops once channel is depleted (internal entry counter is 0)
    //! this can happen on multiple threads or jobs at the same time.
    while (true) {
        var void_data : void?
        _builtin_channel_pop(channel) $(vd) {
            void_data = vd
        }
        if (void_data == null) {
            break
        }
        unsafe {
            let typed_data = reinterpret<TT?#> void_data
            invoke(blk, *typed_data)
        }
    }
}

def for_each_clone(channel : Channel?; blk : block<(res : auto(TT)#) : void>) {
    //! reads input from the channel (in order it was pushed) and invokes the block on each input.
    //! stops once channel is depleted (internal entry counter is 0)
    //! this can happen on multiple threads or jobs at the same time.
    while (true) {
        var temp : TT -# -& -const
        var any = false
        _builtin_channel_pop(channel) $(vd) {
            if (vd != null) {
                any = true
                temp := *(unsafe(reinterpret<TT -# -& -const?#> vd))
            }
        }
        if (!any) {
            break
        }
        invoke(blk, unsafe(reinterpret<TT -& -const#> temp))
        delete temp
    }
}

[deprecated(message="use `pop_and_clone_one` instead")]
def pop_one(channel : Channel?; blk : block<(res : auto(TT)#) : void>) {
    //! reads one command from channel
    var void_data : void?
    _builtin_channel_pop(channel) $(vd) {
        void_data = vd
    }
    if (void_data == null) {
        return false
    }
    unsafe {
        let typed_data = reinterpret<TT?#> void_data
        invoke(blk, *typed_data)
        return true
    }
}

def pop_and_clone_one(channel : Channel?; blk : block<(res : auto(TT)#) : void>) {
    //! reads one command from channel
    var any = false
    var temp : TT -# -& -const
    _builtin_channel_pop(channel) $(vd) {
        if (vd != null) {
            any = true
            temp := *(unsafe(reinterpret<TT -# -& -const?#> vd))
        }
    }
    if (!any) {
        return false
    }
    invoke(blk, unsafe(reinterpret<TT -& -const#> temp))
    delete temp
    return true
}

def try_pop(channel : Channel?; blk : block<(res : auto(TT)#) : void>) : bool {
    //! Non-blocking pop from channel. Returns true if an item was available, false if channel was empty.
    //! Does not wait for data — returns immediately.
    var result = false
    result = _builtin_channel_try_pop(channel) $(data) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))
    }
    return result
}

def try_pop_clone(channel : Channel?; blk : block<(res : auto(TT)#) : void>) : bool {
    //! Non-blocking pop with clone from channel. Returns true if an item was available, false if channel was empty.
    //! The popped value is cloned to the current context before invoking the block.
    var temp : TT -# -& -const
    var any = false
    any = _builtin_channel_try_pop(channel) $(data) {
        temp := *(unsafe(reinterpret<TT -# -& -const?#> data))
    }
    if (!any) {
        return false
    }
    invoke(blk, unsafe(reinterpret<TT -& -const#> temp))
    delete temp
    return true
}

def pop_with_timeout(channel : Channel?; timeout_ms : int; blk : block<(res : auto(TT)#) : void>) : bool {
    //! Pop from channel with timeout in milliseconds.
    //! Returns true if an item was available within the timeout, false if timed out or channel exhausted.
    var result = false
    result = _builtin_channel_pop_with_timeout(channel, timeout_ms) $(data) {
        invoke(blk, *(unsafe(reinterpret<TT?#> data)))
    }
    return result
}

def pop_with_timeout_clone(channel : Channel?; timeout_ms : int; blk : block<(res : auto(TT)#) : void>) : bool {
    //! Pop from channel with timeout and clone. Returns true if an item was available within the timeout.
    //! The popped value is cloned to the current context before invoking the block.
    var temp : TT -# -& -const
    var any = false
    any = _builtin_channel_pop_with_timeout(channel, timeout_ms) $(data) {
        temp := *(unsafe(reinterpret<TT -# -& -const?#> data))
    }
    if (!any) {
        return false
    }
    invoke(blk, unsafe(reinterpret<TT -& -const#> temp))
    delete temp
    return true
}

def push_clone(channel : Channel?; data : auto(TT)) {
    //! clones data and pushes value to the channel (at the end)
    var heap_data = new TT
    *heap_data := data
    _builtin_channel_push(channel, heap_data)
}

def push(channel : Channel?; data : auto?) {
    //! pushes value to the channel (at the end)
    _builtin_channel_push(channel, data)
}

def push_batch_clone(channel : Channel?; data : array<auto(TT)>) {
    //! clones data and pushes values to the channel (at the end)
    var heap_data : array<TT?>
    for (d in data) {
        var tt = new TT
        *tt := d
        heap_data |> push(tt)
    }
    _builtin_channel_push_batch(channel, heap_data)
    heap_data |> resize(0)
    unsafe {
        delete heap_data
    }
}

def push_batch(channel : Channel?; data : array<auto?>) {
    //! pushes values to the channel (at the end)
    _builtin_channel_push_batch(channel, data)
}

def set(box : LockBox?; data : auto(TT)) {
    //! clones data and sets value to the lock box
    var heap_data = new TT
    *heap_data := data
    _builtin_lockbox_set(box, heap_data)
}

def set(box : LockBox?; data : auto?) {
    //! sets value to the lock box
    _builtin_lockbox_set(box, data)
}

def get(box : LockBox?; blk : block<(res : auto(TT)#) : void>) {
    //! reads value from the lock box and invokes the block on it
    var res = false
    _builtin_lockbox_get(box) $(void_data) {
        if (void_data != null) {
            let typed_data = unsafe(reinterpret<TT?#> void_data)
            invoke(blk, *typed_data)
            res = true
        }
    }
    return res
}

def update(box : LockBox?; blk : block<(var res : auto(TT)#) : void>) {
    //! update value in the lock box and invokes the block on it
    _builtin_lockbox_update(box, unsafe(reinterpret<TypeInfo?>(typeinfo rtti_typeinfo(type<TT? -#>)))) $(var void_data : void?) {
        if (void_data != null) {
            var typed_data = unsafe(reinterpret<TT?#> void_data)
            invoke(blk, *typed_data)
            return void_data
        } else {
            var heap_data = new TT
            invoke(blk, *heap_data)
            return unsafe(reinterpret<void?> heap_data)
        }
    }
}

[template(type_)]
def clear(box : LockBox?; type_ : auto(TT)) {
    //! clear value from the lock box
    _builtin_lockbox_update(box, unsafe(reinterpret<TypeInfo?>(typeinfo rtti_typeinfo(type<TT? -#>)))) $(var void_data : void?) : void? {
        if (void_data != null) {
            var typed_data = unsafe(reinterpret<TT -const?> void_data)
            unsafe {
                delete typed_data
            }
        }
        return null
    }
}

def fill(box : LockBox?; data : auto(TT)) {
    //! stores pointer to data in the lock box and marks it as full (isReady becomes false).
    //! data must outlive the grab — caller owns the lifetime.
    _builtin_lockbox_fill(box, unsafe(addr(data)))
}

def grab(box : LockBox?; blk : block<(var res : auto(TT)) : void>) : bool {
    //! grabs data from lock box, marks it as empty (isReady becomes true), invokes block.
    //! does not free the data — caller owns the lifetime.
    var res = false
    _builtin_lockbox_grab(box) $(void_data) {
        if (void_data != null) {
            var typed_data = unsafe(reinterpret<TT -const?> void_data)
            invoke(blk, *typed_data)
            res = true
        }
    }
    return res
}

[template(tinfo), deprecated(message="use `each_clone` instead")]
def each(var channel : Channel?; tinfo : auto(TT)) {
    //! this iterator is used to iterate over the channel in order it was pushed.
    //! iterator stops once channel is depleted (internal entry counter is 0)
    //! iteration can happen on multiple threads or jobs at the same time.
    unsafe {
        return <- generator<TT&#> {
            while (true) {
                var void_data : void?
                _builtin_channel_pop(channel) $(vd) {
                    void_data = vd
                }
                if (void_data == null) {
                    break
                }
                unsafe {
                    yield *reinterpret<TT?#> void_data
                }
            }
            return false
        } finally {
            channel |> release
        }
    }
}

[unused_argument(tinfo)]
def each_clone(var channel : Channel?; tinfo : auto(TT)) {
    //! this iterator is used to iterate over the channel in order it was pushed.
    //! iterator stops once channel is depleted (internal entry counter is 0)
    //! iteration can happen on multiple threads or jobs at the same time.
    unsafe {
        return <- generator<TT&#> {
            while (true) {
                var temp : TT -# -& -const
                var any = false
                _builtin_channel_pop(channel) $(vd) {
                    if (vd != null) {
                        any = true
                        temp := *(unsafe(reinterpret<TT -# -& -const?#> vd))
                    }
                }
                if (!any) {
                    break
                }
                yield unsafe(reinterpret<TT -& -const#> temp)
                delete temp
            }
            return false
        } finally {
            channel |> release
        }
    }
}

//
// Stream wrappers — cross-context byte pipeline. Bytes are owned C++-side,
// so either end may free the Stream without coordinating with the other.
//

require daslib/archive

def push(stream : Stream?; var data : array<uint8>) {
    //! Copy bytes into the stream (at the tail). Bytes are owned by the Stream C++-side
    //! after push — caller may free the source buffer immediately.
    _builtin_stream_push(stream, data)
}

def push(stream : Stream?; data : array<uint8> const#) {
    //! Overload for temporary/borrowed views (e.g. bytes popped from another stream).
    _builtin_stream_push(stream, data)
}

def push_batch(stream : Stream?; var data : array<array<uint8>>) {
    //! Copy a batch of byte blobs into the stream atomically under a single lock.
    _builtin_stream_push_batch(stream, data)
}

def push_archive(stream : Stream?; var value : auto(TT)) {
    //! Serialize ``value`` via ``daslib/archive`` and push the resulting bytes onto the stream.
    //! Use ``pop_archive`` or ``gather_archive`` on the consumer side to deserialize.
    var bytes <- mem_archive_save(value)
    _builtin_stream_push(stream, bytes)
    unsafe {
        delete bytes
    }
}

def pop(stream : Stream?; blk : block<(bytes : array<uint8># -const) : void>) {
    //! Blocking pop from the stream. Invokes the block with a borrowed view over the
    //! popped bytes. The view is only valid for the duration of the block.
    _builtin_stream_pop(stream, blk)
}

def try_pop(stream : Stream?; blk : block<(bytes : array<uint8># -const) : void>) : bool {
    //! Non-blocking pop. Returns true if an item was consumed, false if the stream was empty.
    return _builtin_stream_try_pop(stream, blk)
}

def pop_with_timeout(stream : Stream?; timeout_ms : int; blk : block<(bytes : array<uint8># -const) : void>) : bool {
    //! Pop with a timeout in milliseconds. Returns true if an item was consumed in time.
    return _builtin_stream_pop_with_timeout(stream, timeout_ms, blk)
}

def gather(stream : Stream?; blk : block<(bytes : array<uint8># -const) : void>) {
    //! Drain the stream: invoke the block once per buffered blob in push order, then clear.
    _builtin_stream_gather(stream, blk)
}

def peek(stream : Stream?; blk : block<(bytes : array<uint8># -const) : void>) {
    //! Iterate buffered blobs in push order WITHOUT consuming them.
    _builtin_stream_peek(stream, blk)
}

def pop_archive(stream : Stream?; blk : block<(var value : auto(TT)&) : void>) : bool {
    //! Blocking pop + deserialize via ``daslib/archive``. Returns true if a value was consumed.
    //! The block receives an owned reference — safe to pass fields to functions expecting ``const``.
    var any = false
    var value : TT -# -& -const
    _builtin_stream_pop(stream) $(bytes : array<uint8># -const) {
        if (length(bytes) > 0) {
            any = true
            mem_archive_load(bytes, value)
        }
    }
    if (any) {
        invoke(blk, value)
    }
    unsafe {
        delete value
    }
    return any
}

def try_pop_archive(stream : Stream?; blk : block<(var value : auto(TT)&) : void>) : bool {
    //! Non-blocking pop + deserialize. Returns true if a value was consumed.
    var any = false
    var value : TT -# -& -const
    any = _builtin_stream_try_pop(stream) $(bytes : array<uint8># -const) {
        mem_archive_load(bytes, value)
    }
    if (any) {
        invoke(blk, value)
    }
    unsafe {
        delete value
    }
    return any
}

def gather_archive(stream : Stream?; blk : block<(var value : auto(TT)&) : void>) {
    //! Drain the stream, deserializing each blob via ``daslib/archive`` and invoking the block on each.
    _builtin_stream_gather(stream) $(bytes : array<uint8># -const) {
        var value : TT -# -& -const
        mem_archive_load(bytes, value)
        invoke(blk, value)
        unsafe {
            delete value
        }
    }
}

def with_wait_group(count : int; blk : block<(var status : JobStatus?) : void>) {
    //! Creates a wait group with the given notification count and auto-joins after the block.
    //! Each unit of work should call ``notify_and_release`` (or ``done``) when complete.
    //! The block returns only after all notifications have been received.
    with_job_status(count) $(status) {
        invoke(blk, status)
        status |> join
    }
}

def with_wait_group(blk : block<(var status : JobStatus?) : void>) {
    //! Creates a wait group starting at count 0 with auto-join.
    //! Use ``append`` to dynamically add expected notifications before dispatching work.
    //! The block returns only after all notifications have been received.
    with_job_status(0) $(status) {
        invoke(blk, status)
        status |> join
    }
}

def done(var status : JobStatus?&) {
    //! Mark one unit of work as done in a wait group.
    //! Alias for ``notify_and_release``. Decrements the notification counter and releases the reference.
    //! Sets the pointer to null, preventing double-release.
    status |> notify_and_release
}

def _parallel_for(range_begin, range_end : int; num_jobs : int; blk : block<(job_begin : int; job_end : int; var wg : JobStatus?) : void>) {
    //! Partitions ``[range_begin..range_end)`` into ``num_jobs`` chunks and invokes ``blk`` once per chunk
    //! on the calling thread with ``(chunk_begin, chunk_end, wg)``. The block is expected to dispatch work
    //! via ``new_job`` and call ``wg |> notify_and_release`` when each job finishes.
    //! ``parallel_for`` blocks until all notifications are received (via internal ``with_wait_group``).
    //! Requires ``with_job_que`` context.
    let total = range_end - range_begin
    if (total <= 0 || num_jobs <= 0) {
        return
    }
    let actual_jobs = num_jobs < total ? num_jobs : total
    let chunk = total / actual_jobs
    let remainder = total % actual_jobs
    with_wait_group(actual_jobs) $(wg) {
        var offset = range_begin
        for (i in range(actual_jobs)) {
            let this_chunk = chunk + (i < remainder ? 1 : 0)
            let job_begin = offset
            let job_end = offset + this_chunk
            offset = job_end
            invoke(blk, job_begin, job_end, wg)
        }
    }
}

[tag_function(tag_parallel_for)]
def parallel_for(range_begin, range_end : int; num_jobs : int; blk : block<(job_begin : int; job_end : int; var wg : JobStatus?) : void>) {
    //! this one is stub for _parallel_for
}

[tag_function_macro(tag="tag_parallel_for")]
class private ParallelForJobMacro : AstFunctionAnnotation {
    //! Base macro for parallel_for, parallel_for_each, and parallel_map.
    //! Wraps the block body in ``new_job`` and redirects to the runtime implementation.
    function_name : string = "jobque_boost::_parallel_for"
    def override transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr {
        let last_index = call.arguments.length() - 1
        macro_verify(call.arguments[last_index] is ExprMakeBlock, compiling_program(), call.at, "expecting block as {last_index} argument")
        var newCall = clone_expression(call)
        assume newCallCall = (newCall as ExprCall)
        newCallCall.name := function_name
        var new_body = qmacro(new_job(@ {$b(((call.arguments[last_index] as ExprMakeBlock)._block as ExprBlock).list); }))
        var new_block = ((newCallCall.arguments[last_index] as ExprMakeBlock)._block as ExprBlock)
        new_block.list.clear()
        new_block.list.emplace(new_body)
        return <- newCall
    }
}

def _parallel_for_each(arr : array<auto(TT)>; num_jobs : int; blk : block<(job_begin : int; job_end : int; var wg : JobStatus?) : void>) {
    //! Runtime implementation for ``parallel_for_each``.
    //! Partitions array indices ``[0..length(arr))`` into ``num_jobs`` chunks and invokes ``blk``
    //! with ``(chunk_begin_idx, chunk_end_idx, wg)`` on the calling thread.
    //! The block should dispatch ``new_job`` calls that process ``arr[i]`` for ``i`` in ``[chunk_begin_idx..chunk_end_idx)``,
    //! then call ``wg |> notify_and_release``.
    //! Blocks until all jobs finish. Requires ``with_job_que`` context.
    _parallel_for(0, length(arr), num_jobs, blk)
}

[tag_function(tag_parallel_for_each)]
def parallel_for_each(arr : array<auto(TT)>; num_jobs : int; blk : block<(job_begin : int; job_end : int; var wg : JobStatus?) : void>) {
    //! Convenience wrapper around ``parallel_for`` for arrays.
    //! Partitions array indices ``[0..length(arr))`` into ``num_jobs`` chunks.
    //! The block body is automatically wrapped in ``new_job``.
    //! Blocks until all jobs finish. Requires ``with_job_que`` context.
    pass
}

[tag_function_macro(tag="tag_parallel_for_each")]
class private ParallelForEachJobMacro : ParallelForJobMacro {
    //! This macro handles `parallel_for_each`.
    //! Wraps block body in ``new_job`` and redirects to ``_parallel_for_each``.
    override function_name : string = "jobque_boost::_parallel_for_each"
}

def _parallel_map(arr : array<auto(TT)>; num_jobs : int; var results_channel : Channel?; blk : block<(job_begin : int; job_end : int; var ch : Channel?; var wg : JobStatus?) : void>) {
    //! Runtime implementation for ``parallel_map``.
    //! Partitions array indices ``[0..length(arr))`` into ``num_jobs`` chunks and invokes ``blk``
    //! on the calling thread with ``(chunk_begin_idx, chunk_end_idx, results_channel, wg)``.
    //! Blocks until all jobs finish. Results are available in ``results_channel`` after this call returns.
    //! Requires ``with_job_que`` context.
    _parallel_for(0, length(arr), num_jobs) $(job_begin, job_end, wg) {
        invoke(blk, job_begin, job_end, results_channel, wg)
    }
}

[tag_function(tag_parallel_map)]
def parallel_map(arr : array<auto(TT)>; num_jobs : int; var results_channel : Channel?; blk : block<(job_begin : int; job_end : int; var ch : Channel?; var wg : JobStatus?) : void>) {
    //! Partitions array indices ``[0..length(arr))`` into ``num_jobs`` chunks.
    //! The block body is automatically wrapped in ``new_job``.
    //! Blocks until all jobs finish. Results are available in ``results_channel`` after this call returns.
    //! Requires ``with_job_que`` context.
    pass
}

[tag_function_macro(tag="tag_parallel_map")]
class private ParallelMapJobMacro : ParallelForJobMacro {
    //! This macro handles `parallel_map`.
    //! Wraps block body in ``new_job`` and redirects to ``_parallel_map``.
    override function_name : string = "jobque_boost::_parallel_map"
}

def public capture_jobque_channel(var ch : Channel?) : Channel? {
    //! this function is used to capture a channel that is used by the jobque.
    ch |> add_ref
    return ch
}

def public capture_jobque_job_status(var js : JobStatus?) : JobStatus? {
    //! this function is used to capture a job status that is used by the jobque.
    js |> add_ref
    return js
}

def public capture_jobque_lock_box(var js : LockBox?) : LockBox? {
    //! this function is used to capture a lock box that is used by the jobque.
    js |> add_ref
    return js
}

def public capture_jobque_stream(var s : Stream?) : Stream? {
    //! this function is used to capture a stream that is used by the jobque.
    s |> add_ref
    return s
}

def public release_capture_jobque_channel(ch : Channel?) {
    //! this function is used to release a channel that is used by the jobque.
    if (ch != null) {
        panic("Channel has not been released. missing channel|>release or channel|>notify_and_release")
    }
}

def public release_capture_jobque_job_status(js : JobStatus?) {
    //! this function is used to release a job status that is used by the jobque.
    if (js != null) {
        panic("JobStatus has not been released. missing status|>release or status|>notify_and_release")
    }
}

def public release_capture_jobque_lock_box(js : LockBox?) {
    //! this function is used to release a lock box that is used by the jobque.
    if (js != null) {
        panic("LockBox has not been released. missing box|>release or box|>notify_and_release")
    }
}

def public release_capture_jobque_stream(s : Stream?) {
    //! this function is used to release a stream that is used by the jobque.
    if (s != null) {
        panic("Stream has not been released. missing stream|>release or stream|>notify_and_release")
    }
}


[macro_function]
def private isPtrToJQ(typ : TypeDeclPtr; jqt : string) : bool {
    if (!typ.isPointer) {
        return false
    }
    if (typ.firstType == null) {
        return false
    }
    if (!typ.firstType.isHandle) {
        return false
    }
    if (typ.firstType.annotation._module.name != "jobque") {
        return false
    }
    if (typ.firstType.annotation.name != jqt) {
        return false
    }
    return true
}

[capture_macro(name=channel_and_status_capture_macro)]
class private ChannelAndStatusCapture : AstCaptureMacro {
    //! Capture macro for channel and job status objects in lambda captures.
    def make_capture_call(expr : ExpressionPtr; name : string) : ExpressionPtr {
        //! Creates a call expression that captures (adds a reference to) a jobque object.
        var pCall = new ExprCall(at = expr.at, name := name)
        pCall.arguments |> emplace_new <| clone_expression(expr)
        return <- pCall
    }
    def make_release_call(fld : FieldDeclaration; name : string) : ExpressionPtr {
        //! Creates a call expression that releases (decrements the reference of) a captured jobque object.
        var pCall = new ExprCall(at = fld.at, name := name)
        pCall.arguments |> emplace_new <| new ExprField(at = fld.at,
            value <- new ExprVar(at = fld.at, name := "__this"),
            name := fld.name,
            derefFlags = ExprFieldDerefFlags.ignoreCaptureConst
        )
        return <- pCall
    }
    //! This macro implements capturing of the `jobque::Channel`, `jobque::JobStatus`, `jobque::LockBox`, and `jobque::Stream` types.
    //! When captured reference counts are increased. When lambda is destroyed, reference counts are decreased.
    def override captureExpression(prog : Program?; mod : Module?; expr : ExpressionPtr; typ : TypeDeclPtr) : ExpressionPtr {
        //! Implementation details for the capture macro.
        if (typ |> isPtrToJQ("Channel")) {
            return <- make_capture_call(expr, "jobque_boost::capture_jobque_channel")
        } elif (typ |> isPtrToJQ("JobStatus")) {
            return <- make_capture_call(expr, "jobque_boost::capture_jobque_job_status")
        } elif (typ |> isPtrToJQ("LockBox")) {
            return <- make_capture_call(expr, "jobque_boost::capture_jobque_lock_box")
        } elif (typ |> isPtrToJQ("Stream")) {
            return <- make_capture_call(expr, "jobque_boost::capture_jobque_stream")
        }
        return null
    }
    def override captureFunction(prog : Program?; mod : Module?; var lcs : Structure?; var fun : FunctionPtr) : void {
        //! Implementation details for the capture macro.
        //! Note: generators are skipped because their ``finally`` section is called on every
        //! iteration, not just once at destruction. This means ``Channel``, ``JobStatus``,
        //! ``LockBox``, and ``Stream`` captures inside generators do NOT get automatic release checks.
        //! Users must manually call ``release`` on captured jobque objects inside generators.
        if (fun.flags._generator) {// generator gets finally section called every single time. nothing we can do
            return
        }
        for (fld in lcs.fields) {
            if (fld._type |> isPtrToJQ("Channel")) {
                var pCall = make_release_call(fld, "jobque_boost::release_capture_jobque_channel")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            } elif (fld._type |> isPtrToJQ("JobStatus")) {
                var pCall = make_release_call(fld, "jobque_boost::release_capture_jobque_job_status")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            } elif (fld._type |> isPtrToJQ("LockBox")) {
                var pCall = make_release_call(fld, "jobque_boost::release_capture_jobque_lock_box")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            } elif (fld._type |> isPtrToJQ("Stream")) {
                var pCall = make_release_call(fld, "jobque_boost::release_capture_jobque_stream")
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            }
        }
    }
}


