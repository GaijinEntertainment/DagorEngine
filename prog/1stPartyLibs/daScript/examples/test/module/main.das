require foo
require bar.bar

[export]
def test
    let ftv = foo::get_test_value()
    let btv = bar::get_test_value()
    assert(ftv==btv)
    assert(ftv==13)
    return true
