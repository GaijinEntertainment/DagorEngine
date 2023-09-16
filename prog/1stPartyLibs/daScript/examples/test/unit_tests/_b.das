module _b

require _c

[sideeffects]
def b_func(arg)
    c_func(arg)
    return c_func(arg)
