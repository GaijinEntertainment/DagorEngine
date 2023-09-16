This macro convert assert_once(expr,message) to the following code::

    var __assert_once_I = true  // this is a global variable
    if __assert_once_I && !expr
        __assert_once_I = false
        assert(false,message)
