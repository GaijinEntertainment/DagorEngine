require left
require right
require mid.dle.middle

def get_test_value
    return left::get_test_value() + middle::get_test_value() + right::get_test_value()
