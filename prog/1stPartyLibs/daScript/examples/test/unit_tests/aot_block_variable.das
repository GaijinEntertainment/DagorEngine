[export]
def test
    let scoreFunc = $() {   // this needs to compile, thats it
        return 42; }
    verify(invoke(scoreFunc)==42)
    return true

