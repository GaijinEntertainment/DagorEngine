module _a

require _b

[sideeffects]
def a_func(arg)
    return b_func(arg)
