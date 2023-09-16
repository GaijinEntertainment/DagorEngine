require something_unsafe

// uncomment implicit for failed compilation
def implicit_print_failed(s:string /*implicit*/)
    print(s)

[export]
def test
    implicit_print("hello, world!\n")
    implicit_print_failed("this can fail if implicit is uncommented.\n")
    return true

