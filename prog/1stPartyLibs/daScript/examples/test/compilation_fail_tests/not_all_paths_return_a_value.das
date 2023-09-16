expect 40200

def test_return_int_vs_void(arg:bool)   // 40200: not all control paths return value
    if arg
        return 13

