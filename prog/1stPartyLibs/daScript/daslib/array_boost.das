options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module array_boost shared public

require daslib/templates
require daslib/contracts

[unsafe_operation, template (a), unused_argument(a)]
def private array_helper ( var arr : auto implicit ==const; a : auto(TT) ) : array<TT -const-#>
    var res : array<TT -const-#>
    let lenA = _::length(arr)
    if lenA>=1
        unsafe
            _builtin_make_temp_array(res, addr(arr[0]), lenA)
    return <- res

[unsafe_operation, template (a), unused_argument(a)]
def private array_helper ( arr : auto implicit ==const; a : auto(TT) ) : array<TT -const-#>
    var res : array<TT -const-#>
    let lenA = _::length(arr)
    if lenA>=1
        unsafe
            _builtin_make_temp_array(res, addr(arr[0]), lenA)
    return <- res

[unsafe_operation]
def public temp_array ( var arr : auto implicit ==const )
    //! Creates temporary array from the given object.
    //! Important requirements are:
    //!     * object memory is linear
    //!     * each element follows the next one directly, with the stride equal to size of the element
    //!     * object memory does not change within the lifetime of the returned array
    unsafe
        return <- array_helper ( arr, decltype(arr[0]) )

[unsafe_operation]
def public temp_array ( arr : auto implicit ==const)
    unsafe
        return <- array_helper ( arr, decltype(arr[0]) )

[expect_any_array(v)]
def empty(v:auto(VecT))
    //! returns true if 'v' has 0 elements. this also implies that `length(v)` is defined.
    return _::length(v)==0

[unsafe_operation, template (a), unused_argument(a)]
def public temp_array ( var data : auto? ==const; lenA : int; a : auto(TT) ) : array<TT -const-#>
    var res : array<TT -const-#>
    if lenA>=1
        unsafe
            _builtin_make_temp_array(res, data, lenA)
    return <- res

[unsafe_operation, template (a), unused_argument(a)]
def public temp_array ( data : auto? ==const; lenA : int; a : auto(TT) ) : array<TT -const-#> const
    var res : array<TT -const-#>
    if lenA>=1
        unsafe
            _builtin_make_temp_array(res, data, lenA)
    return <- res

def array_view ( bytes:array<auto(TT)> == const; offset,length:int; blk:block<(view:array<TT>#):void> )
    //! creates a view of the array, which is a temporary array that is valid only within the block
    unsafe
        if offset<0 || (offset+length)>length(bytes)
            panic("array_view: out of range")
        __builtin_array_lock(bytes)
        var res : array<TT>#
        _builtin_make_temp_array(res, addr(bytes[offset]), length)
        invoke(blk, res)
        __builtin_array_unlock(bytes)

def array_view ( var bytes:array<auto(TT)> == const; offset,length:int; blk:block<(var view:array<TT>#):void> )
    //! creates a view of the array, which is a temporary array that is valid only within the block
    unsafe
        if offset<0 || (offset+length)>length(bytes)
            panic("array_view: out of range")
        __builtin_array_lock(bytes)
        var res : array<TT>#
        _builtin_make_temp_array(res, addr(bytes[offset]), length)
        invoke(blk, res)
        __builtin_array_unlock(bytes)