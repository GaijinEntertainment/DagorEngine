options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

enum MdEnum {
    foo = 0
    bar
}

struct MdStruct {
    x : int
    y : float
}

[test]
def test_make_default(t : T?) {
    t |> run("default int") @(t : T?) {
        let t_int = int()
        t |> equal(t_int, 0)
    }
    t |> run("default string") @(t : T?) {
        let t_string = string("")
        t |> equal(t_string, "")
    }
    t |> run("default tuple") @(t : T?) {
        let t_tuple = tuple<int; float>()
        t |> equal(t_tuple._0, 0)
        t |> equal(t_tuple._1, 0.0)
    }
    t |> run("default variant") @(t : T?) {
        let t_variant = variant<a : int; b : float>()
        t |> equal(t_variant is a, true)
        t |> equal(t_variant as a, 0)
    }
    t |> run("default struct") @(t : T?) {
        let t_struct = MdStruct()
        t |> equal(t_struct.x, 0)
        t |> equal(t_struct.y, 0.0)
    }
    t |> run("default enum") @(t : T?) {
        let t_enum = default<MdEnum>
        t |> equal(t_enum, MdEnum.foo)
    }
    t |> run("default array") @(t : T?) {
        let t_array <- array<int>()
        t |> equal(length(t_array), 0)
    }
    t |> run("array struct syntax") @(t : T?) {
        let t_array_struct = array struct<MdStruct>((x = 1), (x = 2))
        t |> equal(length(t_array_struct), 2)
        t |> equal(t_array_struct[0].x, 1)
        t |> equal(t_array_struct[1].x, 2)
        t |> equal(t_array_struct[0].y, 0.0)
        t |> equal(t_array_struct[1].y, 0.0)
    }
}
