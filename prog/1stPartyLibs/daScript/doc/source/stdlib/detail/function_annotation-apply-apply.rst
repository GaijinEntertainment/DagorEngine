This macro implements the apply() pattern. The idea is that for each entry in the structure, variant, or tuple,
the block will be invoked. Both element name, and element value are passed to the block.
For example

    struct Bar
        x, y : float
    apply([[Bar x=1.,y=2.]]) <| $ ( name:string; field )
        print("{name} = {field} ")

Would print x = 1.0 y = 2.0
