// Shapes the fuzzer reaches that no hand-written test does. Every block below compiles
// clean and must keep compiling: the file carries no assertions, because a regression is a
// compiler crash or a spurious error, not a wrong value.
options gen2

// A template structure's typedef holds a block in the dim expression of its own alias. The
// type expressions of a template are never inferred, so that block reaches every post-infer
// visitor with no type and outside of any function - a visitor keying off its enclosing
// function reads a null one.
struct template r {
    def v => 0
    typedef r = r<iterator<a>[ @ {0 = (0, C())}]>
}

// A computed goto inside a captured block. Its label lives in the enclosing function, which
// the block's own label table does not carry.
def label_target : int {
    return 6
}

[export]
def computed_goto_in_a_captured_block {
    invoke($ {
        goto label_target()
    })
    label 6:
    pass
}

// A type macro allocated with new, in the dim of an option result type that also holds an
// auto. The macro type carries no alias name, so looking it up as one matched the whole result
// type - the very type the new sits in - and copied it into itself.
def new_of_type_macro_in_option_result : auto(v) | int[(new<$T>())] {
    return 1
}

[export]
def main {
    new_of_type_macro_in_option_result()
}
