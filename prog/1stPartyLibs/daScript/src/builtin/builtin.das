options indenting=4
options remove_unused_symbols=false

let
    print_flags_debugger = (
            print_flags escapeString
        |   print_flags namesAndDimensions
        |   print_flags typeQualifiers
        |   print_flags refAddresses
    )

[generic]
def intptr ( p : void? ) : uint64
	static_if typeinfo(sizeof p)==4
        return uint64(unsafe(reinterpret<uint> p))
	else
        return unsafe(reinterpret<uint64> p)

[generic]
def intptr ( p : smart_ptr<auto> ) : uint64
	static_if typeinfo(sizeof p)==4
        return uint64(unsafe(reinterpret<uint> p))
	else
        return unsafe(reinterpret<uint64> p)

def _move_with_lockcheck ( var a:auto(valA)&; var b:auto(valB)& )
    static_if typeinfo(need_lock_check type<valA>)
        _builtin_verify_locks(a)
    static_if typeinfo(need_lock_check type<valB>)
        _builtin_verify_locks(b)
    static_if typeinfo(is_class a)
        unsafe
            a <- b
    else
        unsafe
            a <- b

def _return_with_lockcheck ( var a:auto(valT)& ==const ) : auto&
    static_if typeinfo(need_lock_check type<valT>)
        _builtin_verify_locks(a)
    unsafe
        return a

def _return_with_lockcheck ( a:auto(valT)& ==const ) : auto&
    static_if typeinfo(need_lock_check type<valT>)
        _builtin_verify_locks(a)
    unsafe
        return a

def resize(var Arr:array<auto(numT)>;newSize:int)
	static_if typeinfo(is_unsafe_when_uninitialized type<numT>)
		make_function_unsafe()
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    __builtin_array_resize(Arr,newSize,typeinfo(sizeof Arr[0]))

def resize_and_init(var Arr:array<auto(numT)>;newSize:int)
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    let oldSize = length(Arr)
    __builtin_array_resize(Arr,newSize,typeinfo(sizeof Arr[0]))
    for i in range(oldSize,newSize)
        static_if typeinfo(is_workhorse type<numT>)
            Arr[i] = numT()
        static_elif typeinfo(can_move type<numT>)
            Arr[i] <- [[numT()]]
        static_elif typeinfo(can_copy type<numT>)
            Arr[i] = [[numT()]]
        else
            Arr[i] := [[numT()]]

def resize_and_init(var Arr:array<auto(numT)>;newSize:int; initValue:numT)
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    let oldSize = length(Arr)
    __builtin_array_resize(Arr,newSize,typeinfo(sizeof Arr[0]))
    for i in range(oldSize,newSize)
        static_if typeinfo(can_copy initValue)
            Arr[i] = initValue
        else
            Arr[i] := initValue


[unused_argument(Arr, newSize)]
def resize_no_init(var Arr:array<auto(numT)>;newSize:int)
    static_if typeinfo(is_raw type<numT>)
        static_if typeinfo(need_lock_check type<numT>)
            concept_assert(false,"internal error: raw array with lock check")
        __builtin_array_resize_no_init(Arr,newSize,typeinfo(sizeof Arr[0]))
    else
        concept_assert(false,"can't resize_no_init non-raw array")

def reserve(var Arr:array<auto(numT)>;newSize:int)
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    __builtin_array_reserve(Arr,newSize,typeinfo(sizeof Arr[0]))

def pop(var Arr:array<auto(numT)>)
    unsafe(resize(Arr, length(Arr)-1))  // resize will throw if negative

[unused_argument(Arr, value)]
def push(var Arr:array<auto(numT)>;value:numT-# const;at:int)
    static_if typeinfo(can_copy value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        Arr[__builtin_array_push(Arr,at,typeinfo(sizeof Arr[0]))] = value
    else
        concept_assert(false,"can't push value, which can't be copied")

[unused_argument(Arr, value)]
def push(var Arr:array<auto(numT)>;value:numT-# const)
    static_if typeinfo(can_copy value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] = unsafe(reinterpret<numT-#-const>(value))
    else
        concept_assert(false,"can't push value, which can't be copied")

[unused_argument(Arr, value)]
def push(var Arr:array<auto(numT)>; varr:array<numT>-#)
    static_if typeinfo(can_copy type<numT>)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        for t in varr
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] = t
    else
        concept_assert(false,"can't push value, which can't be copied")

[unused_argument(Arr, value)]
def push(var Arr:array<auto(numT)>; varr:numT[]-#)
    static_if typeinfo(can_copy type<numT>)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        for t in varr
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] = t
    else
        concept_assert(false,"can't push value, which can't be copied")

[unused_argument(Arr, value)]
def push(var Arr:array<auto(numT)[]>; varr:numT[]-#)
    static_if typeinfo(can_copy type<numT>)
        static_if typeinfo(sizeof Arr[0])==typeinfo(sizeof varr)
            static_if typeinfo(need_lock_check type<numT>)
                _builtin_verify_locks(Arr)
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] = varr
        else
            concept_assert(false,"can't push array of different size")
    else
        concept_assert(false,"can't push value, which can't be copied")

def emplace(var Arr:array<auto(numT)>;var value:numT-#&;at:int)
    static_if typeinfo(can_move value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        unsafe
            Arr[__builtin_array_push(Arr,at,typeinfo(sizeof Arr[0]))] <- value
    else
        concept_assert(false,"can't emplace value, which can't be moved")

def emplace(var Arr:array<auto(numT)>;var value:numT-#&)
    static_if typeinfo(can_move value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        unsafe
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] <- value
    else
        concept_assert(false,"can't emplace value, which can't be moved")

def emplace(var Arr:array<auto(numT)>;var value:numT[]-#)
    static_if typeinfo(can_move value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        for t in value
            unsafe
                Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] <- t
    else
        concept_assert(false,"can't emplace value, which can't be moved")

def emplace(var Arr:array<auto(numT)[]>;var value:numT[]-#)
    static_if typeinfo(can_move value)
        static_if typeinfo(sizeof Arr[0])==typeinfo(sizeof value)
            static_if typeinfo(need_lock_check type<numT>)
                _builtin_verify_locks(Arr)
            unsafe
                Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] <- value
        else
            concept_assert(false,"can't emplace array of different size")
    else
        concept_assert(false,"can't emplace value, which can't be moved")

def push_clone(var Arr:array<auto(numT)>;value:numT|#;at:int)
    static_if typeinfo(can_clone value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        Arr[__builtin_array_push_zero(Arr,at,typeinfo(sizeof Arr[0]))] := value
    else
        concept_assert(false,"can't push-clone value, which can't be cloned")

def push_clone(var Arr:array<auto(numT)>;value:numT|#)
    static_if typeinfo(can_clone value)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        Arr[__builtin_array_push_back_zero(Arr,typeinfo(sizeof Arr[0]))] := value
    else
        concept_assert(false,"can't push-clone value, which can't be cloned")

def push_clone(var Arr:array<auto(numT)>; varr:numT[])
    static_if typeinfo(can_clone type<numT>)
        static_if typeinfo(need_lock_check type<numT>)
            _builtin_verify_locks(Arr)
        for t in varr
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] := t
    else
        concept_assert(false,"can't push_clone value, which can't be cloned")

def push_clone(var Arr:array<auto(numT)[]>; varr:numT[])
    static_if typeinfo(can_copy type<numT>)
        static_if typeinfo(sizeof Arr[0])==typeinfo(sizeof varr)
            static_if typeinfo(need_lock_check type<numT>)
                _builtin_verify_locks(Arr)
            Arr[__builtin_array_push_back(Arr,typeinfo(sizeof Arr[0]))] := varr
        else
            concept_assert(false,"can't push_clone array of different size")
    else
        concept_assert(false,"can't push value, which can't be cloned")

def push_clone ( var A : auto(CT) -# -const; b : auto(TT) | # )
    static_if !typeinfo(can_clone type<TT-#>)
        concept_assert(false,"can't push_clone type which can't be cloned")
    static_elif !typeinfo(can_be_placed_in_container type<TT-#>)
        concept_assert(false,"can't push_clone type which can't be placed in a container")
    static_elif !typeinfo(builtin_function_exists @@ <(var a:CT -# -const;var b:TT -# -const):void> _::emplace)
        concept_assert(false,"can't push_clone, missing emplace({typeinfo(typename type<CT -# -const>)},{typeinfo(typename type<TT -# -const>)})")
    else
        unsafe
            var cb : TT -# -const := b
            _::emplace(A, cb)

def back ( var a : array<auto(TT)> -const ==const ) : TT &
    let l = length(a)
    if l==0
        panic("back empty array")
    unsafe
        return a[l-1]

def back ( var a : array<auto(TT)> -const ==const # ) : TT # &
    let l = length(a)
    if l==0
        panic("back empty array")
    unsafe
        return a[l-1]

def back ( a : array<auto(TT)> const ==const ) : TT const &
    let l = length(a)
    if l==0
        panic("back empty array")
    unsafe
        return a[l-1]

def back ( a : array<auto(TT)> const ==const # ) : TT # const &
    let l = length(a)
    if l==0
        panic("back empty array")
    unsafe
        return a[l-1]

[expect_any_vector(arr)]
def back ( var arr : auto(TT) ==const ) : auto &
    let n = _::length(arr)
    if n == 0
        panic("vector is empty")
    unsafe
        return arr[n-1]

[expect_any_vector(arr)]
def back ( arr : auto(TT) ==const ) : auto & const
    let n = _::length(arr)
    if n == 0
        panic("vector is empty")
    unsafe
        return arr[n-1]

def erase(var Arr:array<auto(numT)>;at:int)
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    __builtin_array_erase(Arr,at,typeinfo(sizeof Arr[0]))

def erase(var Arr:array<auto(numT)>;at:int;count:int)
    static_if typeinfo(need_lock_check type<numT>)
        _builtin_verify_locks(Arr)
    __builtin_array_erase_range(Arr,at,count,typeinfo(sizeof Arr[0]))


def erase_if(var arr:array<auto(TT)>; blk:block<(key:TT):bool> | block<(var key:TT&):bool>)
    static_if typeinfo(need_lock_check type<TT>)
        _builtin_verify_locks(arr)

    var i = length(arr)
    while --i >= 0
        if invoke(blk, arr[i])
            let endIndex = i
            while --i >= 0
                if !invoke(blk, arr[i])
                    break
            __builtin_array_erase_range(arr, i + 1, endIndex - i, typeinfo(sizeof arr[0]))


def remove_value(var arr : array<auto(TT)>|#; key : TT) : bool
    //! Removes the first occurrence of a specific object from the collection.
    let idx = find_index(arr, key)
    if idx != -1
        arr |> erase(idx)
        return true
    return false

[unused_argument(a),expect_dim(a)]
def length(a:auto|#):int
    return typeinfo(dim a)

def empty(a:array<auto>|#) : bool
    return length(a)==0

// table

def empty(a:table<auto;auto>|#) : bool
    return length(a)==0

[deprecated(message="use tab?[key] or 'get' instead")]
def find(Tab:table<auto(keyT);auto(valT)>|#;at:keyT-#;blk:block<(p:valT?#):void>)
    __builtin_table_lock(Tab)
    var val = __builtin_table_find(Tab,at)
    unsafe
        invoke(blk,reinterpret<valT?#> val)
    __builtin_table_unlock(Tab)

[deprecated(message="use tab?[key] or 'get' instead")]
def find(Tab:table<auto(keyT)>;at:keyT|#;blk:block<(p:void?):void>)
    concept_assert(false,"find(table<keyT; void>, ...) is not supported")

// one table lookup with block to rule them all 'get'

def get(Tab:table<auto(keyT);auto(valT)> ==const #;at:keyT|#;blk:block<(p:valT#&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?#> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

def get(Tab:table<auto(keyT);auto(valT)> ==const;at:keyT|#;blk:block<(p:valT&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

def get(var Tab:table<auto(keyT);auto(valT)> ==const #;at:keyT|#;blk:block<(var p:valT#&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?#> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

def get(var Tab:table<auto(keyT);auto(valT)> ==const;at:keyT|#;blk:block<(var p:valT&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

def get(Tab:table<auto(keyT)>;at:keyT|#;blk:block<(var p:void?):void>)
    concept_assert(false,"get(table<keyT>, ...) is not supported; use 'key_exists' instead")
    return false

// find if exists

[deprecated(message="use tab?[key] or 'get' instead")]
def find_if_exists(Tab:table<auto(keyT);auto(valT)>;at:keyT-#;blk:block<(p:valT&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        invoke(blk,*val)
        __builtin_table_unlock(Tab)
        return true
    else
        return false

[deprecated(message="use tab?[key] or 'get' instead")]
def find_if_exists(Tab:table<auto(keyT);auto(valT)>#;at:keyT-#;blk:block<(p:valT&#):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        invoke(blk,*(reinterpret<valT?#> val) )
        __builtin_table_unlock(Tab)
        return true
    else
        return false

[deprecated(message="use tab?[key] or 'get' instead")]
def find_if_exists(Tab:table<auto(keyT)>;at:keyT-#;blk:block<(p:void?):void>)
    concept_assert(false,"find_if_exists(table<keyT; void>, ...) is not supported")
    return false

// find for edit

[deprecated(message="use tab?[key] or 'get' instead")]
def find_for_edit(var Tab:table<auto(keyT);auto(valT)>;at:keyT-#;blk:block<(var p:valT?#):void>)
    __builtin_table_lock(Tab)
    var val = __builtin_table_find(Tab,at)
    unsafe
        invoke(blk,reinterpret<valT?#> val)
    __builtin_table_unlock(Tab)

[deprecated(message="use tab?[key] or 'get' instead")]
def find_for_edit(var Tab:table<auto(keyT)>;at:keyT|#;blk:block<(var p:void?):void>)
    concept_assert(false,"find_for_edit(table<keyT; void>, ...) is not supported")

[unsafe_operation, deprecated(message="use tab?[key] instead")]
def find_for_edit(var Tab:table<auto(keyT);auto(valT)>|#;at:keyT-#):valT?
    return __builtin_table_find(Tab,at)

[unsafe_operation, deprecated(message="use tab?[key] instead")]
def find_for_edit(var Tab:table<auto(keyT)>;at:keyT|#):void?
    concept_assert(false,"find_for_edit(table<keyT; void>, ...) is not supported")

// find for edit if exists

[deprecated(message="use tab?[key] or 'get' instead")]
def find_for_edit_if_exists(var Tab:table<auto(keyT);auto(valT)>#;at:keyT-#;blk:block<(var p:valT#&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?#> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

[deprecated(message="use tab?[key] or 'get' instead")]
def find_for_edit_if_exists(var Tab:table<auto(keyT);auto(valT)>;at:keyT-#;blk:block<(var p:valT&):void>)
    var val = __builtin_table_find(Tab,at)
    if val != null
        __builtin_table_lock(Tab)
        unsafe
            invoke(blk,*(reinterpret<valT?> val))
        __builtin_table_unlock(Tab)
        return true
    else
        return false

[deprecated(message="use tab?[key] or 'get' instead")]
def find_for_edit_if_exists(var Tab:table<auto(keyT)>;at:keyT|#;blk:block<(var p:void?):void>)
    concept_assert(false,"find_for_edit_if_exists(table<keyT; void>, ...) is not supported")
    return false

def _at_with_lockcheck ( var Tab:table<auto(keyT);auto(valT)>; at:keyT|# ) : valT &
    static_if typeinfo(need_lock_check type<valT>)
        _builtin_verify_locks(Tab)
    unsafe
        return Tab[at]

def erase(var Tab:table<auto(keyT);auto(valT)>;at:string#):bool
    if typeinfo(need_lock_check type<valT>)
        _builtin_verify_locks(Tab)
    let at_nt := at
    return __builtin_table_erase(Tab,at_nt)

def erase(var Tab:table<auto(keyT);auto(valT)>;at:keyT|#):bool
    if typeinfo(need_lock_check type<valT>)
        _builtin_verify_locks(Tab)
    return __builtin_table_erase(Tab,at)

def insert(var Tab:table<auto(keyT)>;at:keyT|#)
    __builtin_table_set_insert(Tab,at)

def key_exists(Tab:table<auto(keyT);auto(valT)>|#;at:string#):bool
    let at_nt := at
    return __builtin_table_key_exists(Tab,at_nt)

def key_exists(Tab:table<auto(keyT);auto(valT)>|#;at:keyT|#):bool
    return __builtin_table_key_exists(Tab,at)

def binary_save(obj; subexpr:block<(data:array<uint8>):void>)
    concept_assert(typeinfo(is_ref_type obj),"can only serialize ref types")
    _builtin_binary_save(obj,subexpr)

def binary_load(var obj; data:array<uint8>)
    concept_assert(typeinfo(is_ref_type obj),"can only serialize ref types")
    _builtin_binary_load(obj,data)

[skip_lock_check]
def copy_to_local ( a : auto(TT) ) : TT -const
    static_if typeinfo(can_copy a)
        return unsafe(reinterpret<TT-const> a)
    else
        concept_assert(false,"can't copy this type")

[skip_lock_check]
def move_to_local ( var a : auto(TT)& ) : TT -const -&
    static_if typeinfo(can_move a)
        return <- a
    else
        concept_assert(false,"can't move this type")

[skip_lock_check]
def clone(clone_src:auto(TT)|#) : TT -const -#
    unsafe
        var clone_dest : TT - #
        clone_dest := clone_src
        return <- clone_dest

[skip_lock_check]
def clone_to_move(clone_src:auto(TT)|#) : TT -const -#
    unsafe
        var clone_dest : TT - #
        clone_dest := clone_src
        return <- clone_dest

def clone_dim(var a;b:auto|#)
    static_if typeinfo(is_dim a) && typeinfo(is_dim b) && typeinfo(dim a)==typeinfo(dim b)
        if typeinfo(is_pod a)
            unsafe
                memcpy(addr(a[0]),addr(b[0]),typeinfo(sizeof a[0])*length(a))
        else
            for aV,bV in a,b
                aV := bV
    else
        concept_assert(false,"can't clone this array")

def clone(var a:array<auto(TT)>;b:array<TT>|#)
    let ln = length(b)
    unsafe(resize(a,ln)) // note: is this safe? do we actaully need to init first???
    if ln == 0
        return
    static_if typeinfo(is_pod type<TT>)
        unsafe
            memcpy(addr(a[0]),addr(b[0]),typeinfo(sizeof a[0])*ln)
    else
        for aV,bV in a,b
            aV := bV

def clone(var a:table<auto(KT)>;b:table<KT>|#)
    clear(a)
    for k in keys(b)
        __builtin_table_set_insert(a,k)

def clone(var a:table<string>;b:table<string>|#)
    clear(a)
    for k in keys(b)
        let kk := k
        __builtin_table_set_insert(a,kk)

def clone(var a:table<auto(KT);auto(VT)>;b:table<KT;VT>|#)
    clear(a)
    for k,v in keys(b),values(b)
        a[k] := v

def clone(var a:table<string;auto(VT)>;b:table<string;VT>|#)
    clear(a)
    for k,v in keys(b),values(b)
        let kk := k
        a[kk] := v

[unsafe_outside_of_for,nodiscard]
def keys(a:table<auto(keyT);auto(valT)>|#) : iterator<keyT - const>
    var it : iterator<keyT - const>
    __builtin_table_keys(it,a,typeinfo(sizeof type<keyT>))
    return <- it

def values(a:table<auto(keyT)> ==const|#)
    concept_assert(false,"can't iterate over values of a table<...; void>")

def values(var a:table<auto(keyT)> ==const|#)
    concept_assert(false,"can't iterate over values of a table<...; void>")

[unsafe_outside_of_for,nodiscard]
def values(a:table<auto(keyT);auto(valT)> ==const|#) : iterator<valT & const>
    var it : iterator<valT & const>
    __builtin_table_values(it,a,typeinfo(sizeof type<valT>))
    return <- it

[unsafe_outside_of_for,nodiscard]
def values(var a:table<auto(keyT);auto(valT)> ==const|#) : iterator<valT &>
    var it : iterator<valT &>
    __builtin_table_values(it,a,typeinfo(sizeof type<valT>))
    return <- it

def finalize_dim(var a : auto(TT)[])
    static_if typeinfo(can_delete type<TT>)
        for aV in a
            unsafe
                delete aV
    else
        concept_assert(false,"can't finalize this array")

def finalize(var a:array<auto(TT)>)
    static_if typeinfo(can_delete type<TT>)
        for aV in a
            unsafe
                delete aV
    __builtin_array_free(a,typeinfo(sizeof a[0]))

def finalize(var a:table<auto(TK);auto(TV)>)
    static_if typeinfo(can_delete type<TV>)
        for aV in values(a)
            unsafe
                delete aV
    __builtin_table_free(a,typeinfo(sizeof type<TK>),typeinfo(sizeof type<TV>))

// temporal

def lock ( Tab : table<auto(keyT);auto(valT)>|#; blk : block<(t : table<keyT;valT>#):void >  )
    __builtin_table_lock(Tab)
    unsafe
        invoke(blk,reinterpret<table<auto(keyT);auto(valT)> const>(Tab))
    __builtin_table_unlock(Tab)

def lock_forever ( var Tab : table<auto(keyT);auto(valT)>|# ) : table<keyT;valT> #
    __builtin_table_lock(Tab)
    unsafe
        return <- reinterpret<table<keyT;valT> #>(Tab)

// functional

def next ( it:iterator<auto(TT)>; var value : TT& ) : bool
    static_if !typeinfo(can_copy type<TT>)
        concept_assert(false, "requires type which can be copied")
    static_elif typeinfo(is_ref_value type<TT>)
        var pValue : TT - & ?
        unsafe
            if _builtin_iterator_iterate(it, addr(pValue))
                value = *pValue
                return true
            else
                return false
    else
        unsafe
            return _builtin_iterator_iterate(it, addr(value))

[generic,nodiscard]
def each ( rng : range ) : iterator<int>
    var it : iterator<int>
    _builtin_make_range_iterator(it,rng)
    return <- it

def iter_range ( foo )
    concept_assert(typeinfo(is_iterable foo),"requires iterable")
    return range(_::length(foo)) // assuming length(foo) exists

[generic,unsafe_outside_of_for,nodiscard]
def each ( str : string ) : iterator<int>
    var it : iterator<int>
    _builtin_make_string_iterator(it,str)
    return <- it

[unsafe_outside_of_for,nodiscard]
def each ( a : auto(TT)[] ) : iterator<TT&>
    unsafe
        var parr : void? = reinterpret<void?>(addr(a[0]))
        var it : iterator<TT&>
        _builtin_make_fixed_array_iterator(it,parr,typeinfo(dim a),typeinfo(sizeof a[0]))
        return <- it

[unsafe_outside_of_for,nodiscard]
def each ( a : array<auto(TT)> ) : iterator<TT&>
    var it : iterator<TT&>
    _builtin_make_good_array_iterator(it, a, typeinfo(sizeof a[0]))
    return <- it

[unsafe_outside_of_for,nodiscard]
def each ( a : array<auto(TT)># ) : iterator<TT#&>
    var it : iterator<TT#&>
    _builtin_make_good_array_iterator(it, a, typeinfo(sizeof a[0]))
    unsafe
        return <- it

[nodiscard]
def each ( lam : lambda<(var arg:auto(argT)):bool> ) : iterator<argT -&>
    concept_assert(typeinfo(is_ref type<argT>),"lamda-to-iterator argument has to be reference; expecting value& or reference type")
    var it : iterator<argT -&>
    _builtin_make_lambda_iterator(it,lam,typeinfo(sizeof type<argT>))
    unsafe
        return <- it

[nodiscard]
def each_ref ( lam : lambda<(var arg:auto(argT)?):bool> ) : iterator<argT &>
    var it : iterator<argT &>
    _builtin_make_lambda_iterator(it,lam,typeinfo(sizeof type<void?>))
    unsafe
        return <- it

[unused_argument(tt),nodiscard]
def each_enum(tt:auto(TT)) : iterator<TT -const -&>
    concept_assert(typeinfo(is_enum tt),"expecting 'each_enum(any_enum_value)'")
    var iter : iterator<TT -const -&>
    _builtin_make_enum_iterator(iter)
    unsafe
        return <- iter

def nothing( var it : iterator<auto(TT)> ) : iterator<TT>
    _builtin_make_nil_iterator(it)
    return <- it

[skip_lock_check]
def to_array ( it : iterator<auto(TT)> ) : array<TT -const -&>
    var arr : array<TT -const -&>
    for x in it
        push_clone(arr,x)
    return <- arr

[skip_lock_check]
def to_array(a:auto(TT)[]) : array<TT -const>
    var arr : array<TT -const>
    unsafe(resize(arr,length(a)))   // TODO: is this safe? do we actually need to init first???
    unsafe
        reinterpret<TT -const[typeinfo(dim a)]>(addr(arr[0])) := a
    return <- arr

[skip_lock_check]
def to_array_move(var a:auto(TT)[]) : array<TT -const>
    unsafe
        var arr : array<TT -const>
        static_if typeinfo(can_copy a)
            unsafe(resize(arr,length(a))) // TODO: is this safe? do we actually need to init first???
            unsafe
                reinterpret<TT -const[typeinfo(dim a)]>(addr(arr[0])) = a
        static_elif typeinfo(can_move a)
            unsafe(resize(arr,length(a))) // TODO: is this safe? do we actually need to init first???
            unsafe
                reinterpret<TT -const[typeinfo(dim a)]>(addr(arr[0])) <- a
        else
            concept_assert(false,"this array can't be copied or moved")
        return <- arr

[skip_lock_check]
def to_array_move(a:auto(TT) ==const) : array<TT -const>
    unsafe
        var arr : array<TT -const>
        static_if typeinfo(is_smart_ptr a)
            concept_assert(false,"can't create array from smart pointer by value. did you mean [\{Type[1] value\}] instead of [\{Type value\}]")
        static_if typeinfo(can_copy a)
            arr |> push(a)
        static_elif typeinfo(can_move a)
            concept_assert(false,"can't move from constant value")
        else
            concept_assert(false,"this array can't be copied or moved")
        return <- arr

[skip_lock_check]
def to_array_move(var a:auto(TT) ==const) : array<TT -const>
    unsafe
        var arr : array<TT -const>
        static_if typeinfo(is_smart_ptr a)
            concept_assert(false,"can't create array from smart pointer by value. did you mean [\{Type[1] value\}] instead of [\{Type value\}]")
        static_if typeinfo(can_copy a)
            arr |> push(a)
        static_elif typeinfo(can_move a)
            arr |> emplace(a)
        else
            concept_assert(false,"this array can't be copied or moved")
        return <- arr

[skip_lock_check]
def to_table(a:tuple<auto(keyT);auto(valT)>[]) : table<keyT -const;valT>
    var tab : table<keyT -const;valT>
    for x in a
        tab[x._0] := x._1
    return <- tab

[skip_lock_check]
def to_table(a:auto(keyT)[]) : table<keyT -const>
    var tab : table<keyT -const>
    for at in a
        __builtin_table_set_insert(tab,at)
    return <- tab

[skip_lock_check]
def to_table_move(a:auto(keyT)[]) : table<keyT -const>
    var tab : table<keyT -const>
    for at in a
        __builtin_table_set_insert(tab,at)
    return <- tab

[skip_lock_check]
def to_table_move(a:array<auto(keyT)>) : table<keyT -const>
    var tab : table<keyT -const>
    for at in a
        __builtin_table_set_insert(tab,at)
    return <- tab

[skip_lock_check]
def to_table_move(a:auto(keyT)) : table<keyT -const>
    var tab : table<keyT -const>
    __builtin_table_set_insert(tab,a)
    return <- tab

[skip_lock_check]
def to_table_move(var a:tuple<auto(keyT);auto(valT)>) : table<keyT -const;valT>
    var tab : table<keyT -const;valT>
    static_if typeinfo(can_copy type<valT>)
        tab[a._0] = a._1
    static_elif typeinfo(can_move type<valT>)
        tab[a._0] <- a._1
    else
        concept_assert(false,"this array can't be copied or moved")
    return <- tab

[skip_lock_check]
def to_table_move(var a:tuple<auto(keyT);auto(valT)>[]) : table<keyT -const;valT>
    var tab : table<keyT -const;valT>
    static_if typeinfo(can_copy type<valT>)
        for x in a
            tab[x._0] = x._1
    static_elif typeinfo(can_move type<valT>)
        for x in a
            tab[x._0] <- x._1
    else
        concept_assert(false,"this array can't be copied or moved")
    return <- tab

[skip_lock_check]
def to_table_move(var a:array<tuple<auto(keyT);auto(valT)>>) : table<keyT -const;valT>
    var tab : table<keyT -const;valT>
    static_if typeinfo(can_copy type<valT>)
        for x in a
            tab[x._0] = x._1
    static_elif typeinfo(can_move type<valT>)
        for x in a
            tab[x._0] <- x._1
    else
        concept_assert(false,"this array can't be copied or moved")
    return <- tab

def sort ( var a : auto(TT)[]|# )
    if length(a) <= 1
        return
    static_if typeinfo(is_numeric_comparable type<TT>)
        unsafe
            __builtin_sort ( addr(a[0]), length(a) )    // there is numeric specialization
    static_elif typeinfo(is_vector type<TT>)
        concept_assert(false,"there is no default compare function for vectors")
    static_elif typeinfo(is_string type<TT>)
        unsafe
            __builtin_sort_string ( addr(a[0]), length(a) )     // there is string specialization
    else
        unsafe
            static_if typeinfo(is_ref_type type<TT>)
                __builtin_sort_dim_any_cblock ( a, typeinfo(sizeof a[0]), length(a) ) <| $ ( x, y  : TT ) : bool
                    return x < y
            else
                __builtin_sort_dim_any_ref_cblock ( a, typeinfo(sizeof a[0]), length(a) ) <| $ ( x, y  : TT ) : bool
                    return x < y

def sort ( var a : array<auto(TT)>|# )
    if length(a) <= 1
        return
    static_if typeinfo(is_numeric_comparable type<TT>)
        __builtin_array_lock(a)
        unsafe
            __builtin_sort ( addr(a[0]), length(a) )    // there is numeric specialization
        __builtin_array_unlock(a)
    static_elif typeinfo(is_vector type<TT>)
        concept_assert(false,"there is no default compare function for vectors")
    static_elif typeinfo(is_string type<TT>)
        __builtin_array_lock(a)
        unsafe
            __builtin_sort_string ( addr(a[0]), length(a) )     // there is string specialization
        __builtin_array_unlock(a)
    else
        unsafe
            static_if typeinfo(is_ref_type type<TT>)
                __builtin_sort_array_any_cblock ( a, typeinfo(sizeof a[0]), length(a) ) <| $ ( x, y  : TT ) : bool
                    return x < y
            else
                __builtin_sort_array_any_ref_cblock ( a, typeinfo(sizeof a[0]), length(a) ) <| $ ( x, y  : TT ) : bool
                    return x < y

[builtin_array_sort]
def sort ( var a : auto(TT)[]|#; cmp : block<(x,y:TT):bool> )
    static_if typeinfo(is_numeric_comparable type<TT>)
        unsafe
            __builtin_sort_cblock_dim ( a, typeinfo(sizeof a[0]), length(a), cmp )
    static_elif typeinfo(is_vector type<TT>)
        unsafe
            __builtin_sort_cblock_dim ( a, typeinfo(sizeof a[0]), length(a), cmp )
    static_elif typeinfo(is_string type<TT>)
        unsafe
            __builtin_sort_cblock_dim ( a, typeinfo(sizeof a[0]), length(a), cmp )
    else
        unsafe
            static_if typeinfo(is_ref_type type<TT>)
                __builtin_sort_dim_any_cblock ( a, typeinfo(sizeof a[0]), length(a), cmp )
            else
                __builtin_sort_dim_any_ref_cblock ( a, typeinfo(sizeof a[0]), length(a), cmp )

[builtin_array_sort]
def sort ( var a : array<auto(TT)>|#; cmp : block<(x,y:TT):bool> )
    static_if typeinfo(is_numeric_comparable type<TT>)
        unsafe
            __builtin_sort_cblock_array ( a, typeinfo(sizeof a[0]), length(a), cmp )
    static_elif typeinfo(is_vector type<TT>)
        unsafe
            __builtin_sort_cblock_array ( a, typeinfo(sizeof a[0]), length(a), cmp )
    static_elif typeinfo(is_string type<TT>)
        unsafe
            __builtin_sort_cblock_array ( a, typeinfo(sizeof a[0]), length(a), cmp )
    else
        unsafe
            static_if typeinfo(is_ref_type type<TT>)
                __builtin_sort_array_any_cblock ( a, typeinfo(sizeof a[0]), length(a), cmp )
            else
                __builtin_sort_array_any_ref_cblock ( a, typeinfo(sizeof a[0]), length(a), cmp )

def lock ( var a : array<auto(TT)> ==const|#; blk : block<(var x : array<TT>#)> )
    __builtin_array_lock(a)
    unsafe
        invoke(blk, reinterpret<array<auto(TT)> -const #> a)
    __builtin_array_unlock(a)

def lock ( a : array<auto(TT)> ==const|#; blk : block<(x : array<TT>#)> )
    __builtin_array_lock(a)
    unsafe
        invoke(blk, reinterpret<array<auto(TT)> const #> a)
    __builtin_array_unlock(a)

def lock_data ( var a : array<auto(TT)> ==const|#; blk : block<(var p:TT?#;s:int)> )
    let len = length(a)
    if len != 0
        __builtin_array_lock(a)
        unsafe
            invoke(blk, reinterpret<TT?#> addr(a[0]), len)
        __builtin_array_unlock(a)
    else
        var nullT : TT?
        unsafe
            invoke(blk, reinterpret<TT?#> nullT, 0)

def lock_data ( a : array<auto(TT)> ==const|#; blk : block<(p:TT const?#;s:int)> )
    let len = length(a)
    if len != 0
        __builtin_array_lock(a)
        unsafe
            invoke(blk, reinterpret<TT const?#> addr(a[0]), len)
        __builtin_array_unlock(a)
    else
        unsafe
            invoke(blk, reinterpret<TT const?#> null, 0)

// find_index

def find_index ( arr : array<auto(TT)>|#; key : TT )
    for i in range(length(arr))
        if arr[i]==key
            return i
    return -1

def find_index ( arr : auto(TT)[]|#; key : TT )
    for i in range(length(arr))
        if arr[i]==key
            return i
    return -1

def find_index ( arr : iterator<auto(TT)>; key : TT - & )
    for o,i in arr,count()
        if o == key
            return i
    return -1

def find_index_if ( arr : array<auto(TT)>|#; blk : block<(key:TT):bool> )
    for i in range(length(arr))
        if invoke(blk,arr[i])
            return i
    return -1

def find_index_if ( arr : auto(TT)[]|#; blk : block<(key:TT):bool> )
    for i in range(length(arr))
        if invoke(blk,arr[i])
            return i
    return -1

def find_index_if ( arr : iterator<auto(TT)>; blk : block<(key:TT - &):bool> )
    for o,i in arr,count()
        if invoke(blk,o)
            return i
    return -1

def has_value ( a; key )
    static_if typeinfo(is_iterable a)
        for t in a
            if t == key
                return true
        return false
    else
        concept_assert(false,"can't iterate over source; 'has_value' is meaningless")
        return false

def get_ptr ( src : smart_ptr<auto(TT)> ==const ) : TT? const
    unsafe
        return reinterpret<TT?> src

def get_ptr ( var src : smart_ptr<auto(TT)> ==const ) : TT?
    unsafe
        return reinterpret<TT?> src

def get_const_ptr ( src : smart_ptr<auto(TT)> ) : TT ? const
    unsafe
        return reinterpret<TT ? const> src

def add_ptr_ref ( src : smart_ptr<auto(TT)> ) : smart_ptr<TT>
    unsafe
        var dst := reinterpret<smart_ptr<TT>> src
        return <- dst

[generic]
def get_command_line_arguments : array<string>
    var args : array<string>
    builtin_get_command_line_arguments(args)
    return <- args

[unsafe_operation]
def map_to_array ( data:void?; len:int; blk:block<(var arg:array<auto(TT)>#)> )
    assert(data != null,"expecting data")
    assert(len > 0,"at least 1 element")
    let elem_size = typeinfo(sizeof type<TT>)
    let num_elem = len / elem_size
    unsafe
        _builtin_temp_array(data, num_elem, blk)

[unsafe_operation]
def map_to_ro_array ( data:void?; len:int; blk:block<(arg:array<auto(TT)>#)> )
    assert(data != null,"expecting data")
    assert(len > 0,"at least 1 element")
    let elem_size = typeinfo(sizeof type<TT>)
    let num_elem = len / elem_size
    unsafe
        _builtin_temp_array(data, num_elem, blk)

def swap ( var a,b:auto(TT)& )
    unsafe
        var t : TT - & <- a
        a <- b
        b <- t

def subarray ( a:auto(TT)[]; r:range )
    static_if typeinfo(can_copy type<TT>)
        var ra : array<TT-const-&>
        if r.x < r.y
            ra |> reserve(r.y-r.x)
            for t in r
                ra |> push(a[t])
        return <- ra
    else
        concept_assert(false,"can't get subarray of non-copyable type")

def subarray ( a:auto(TT)[]; r:urange )
    static_if typeinfo(can_copy type<TT>)
        var ra : array<TT-const-&>
        if r.x < r.y
            ra |> reserve(int(r.y-r.x))
            for t in r
                ra |> push(a[t])
        return <- ra
    else
        concept_assert(false,"can't get subarray of non-copyable type")

def subarray ( a:array<auto(TT)>; r:range )
    static_if typeinfo(can_copy type<TT>)
        var ra : array<TT-const-&>
        if r.x < r.y
            ra |> reserve(r.y-r.x)
            for t in r
                ra |> push(a[t])
        return <- ra
    else
        concept_assert(false,"can't get subarray of non-copyable type")

def subarray ( a:array<auto(TT)>; r:urange )
    static_if typeinfo(can_copy type<TT>)
        var ra : array<TT-const-&>
        if r.x < r.y
            ra |> reserve(int(r.y-r.x))
            for t in r
                ra |> push(a[t])
        return <- ra
    else
        concept_assert(false,"can't get subarray of non-copyable type")

def move_to_ref ( var a : auto &; var b )
    static_if typeinfo(is_ref_type a)
        static_if typeinfo(can_move a)
            a <- b
        else
            concept_assert(false, "type is not movable")
    else
        a = b

def clear ( var t : table<auto(KT); auto(VT)> )
    static_if typeinfo(need_lock_check type<VT>)
        _builtin_verify_locks(t)
    _builtin_table_clear(t)


def float2(a,b)
    return float2(float(a),float(b))

def float3(a,b,c)
    return float3(float(a),float(b),float(c))

def float4(a,b,c,d)
    return float4(float(a),float(b),float(c),float(d))

def int2(a,b)
    return int2(int(a),int(b))

def int3(a,b,c)
    return int3(int(a),int(b),int(c))

def int4(a,b,c,d)
    return int4(int(a),int(b),int(c),int(d))

def uint2(a,b)
    return uint2(uint(a),uint(b))

def uint3(a,b,c)
    return uint3(uint(a),uint(b),uint(c))

def uint4(a,b,c,d)
    return uint4(uint(a),uint(b),uint(c),uint(d))
