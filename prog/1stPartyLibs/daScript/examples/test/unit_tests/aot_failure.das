[export] def test()
    let test_block <- ${ print("hello world\n"); }
    invoke(test_block)
    return true
