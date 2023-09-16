require bar.bar

[export]
def test
    let btv = bar::get_test_value()
    assert(btv==13)
    return true
