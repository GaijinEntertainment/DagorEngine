This macro transforms::

    ptr |> if_not_null <| call(...)

to::

    var _ptr_var = ptr
    if _ptr_var
        call(*_ptr_var,...)

