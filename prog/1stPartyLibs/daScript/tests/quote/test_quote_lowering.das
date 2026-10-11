options gen2
options rtti
options indenting = 4

// A/B equivalence for the daslib/quote lowering (QuotePass, gated on aot_macros):
// every shape is quoted twice — through the interpreter SimNode path (_quote_shapes)
// and through the lowered reconstruction path (_quote_shapes_lowered) — and the
// describe output must match exactly.

require dastest/testing_boost public
require _quote_shapes
require _quote_shapes_lowered

[test]
def test_quote_lowering_ab(t : T?) {
    t |> run("consts (escapes, non-ASCII, double, hex)") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_consts(), _quote_shapes::shape_consts())
    }
    t |> run("call") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_call(), _quote_shapes::shape_call())
    }
    t |> run("$c/$v splices + array literal") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_splices(), _quote_shapes::shape_splices())
    }
    t |> run("$i ident splice") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_ident_splice(), _quote_shapes::shape_ident_splice())
    }
    t |> run("block (var/for/print)") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_block(), _quote_shapes::shape_block())
    }
    t |> run("block with finally") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_finally_block(), _quote_shapes::shape_finally_block())
    }
    t |> run("make-struct named args") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_make_struct(), _quote_shapes::shape_make_struct())
    }
    t |> run("named-module enum type") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_named_enum_type(), _quote_shapes::shape_named_enum_type())
    }
    t |> run("named-module struct type") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_named_struct_type(), _quote_shapes::shape_named_struct_type())
    }
    t |> run("nested container type") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_nested_type(), _quote_shapes::shape_nested_type())
    }
    t |> run("table literal") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_table_literal(), _quote_shapes::shape_table_literal())
    }
    t |> run("tuple literal") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_tuple_literal(), _quote_shapes::shape_tuple_literal())
    }
    t |> run("lambda with capture clause") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_lambda_capture(), _quote_shapes::shape_lambda_capture())
    }
    t |> run("generator") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_generator(), _quote_shapes::shape_generator())
    }
    t |> run("qmacro_block_to_array") @(t : T?) {
        t |> equal(_quote_shapes_lowered::shape_block_to_array(), _quote_shapes::shape_block_to_array())
    }
}
