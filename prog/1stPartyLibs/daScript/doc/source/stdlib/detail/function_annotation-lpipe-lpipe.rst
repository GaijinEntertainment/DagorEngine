This macro will implement the lpipe function. It allows piping blocks the previous line call. For example::

    def take2(a,b:block)
        invoke(a)
        invoke(b)
    ...
    take2 <|
        print("block1\n")
    lpipe <|    // this block will pipe into take2
        print("block2\n")
