options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/rtti
require daslib/ast

// Test typeinfo trait queries on various types

struct SimpleStruct {
    x : int
    y : float
}

class SimpleClass {
    value : int
}

[test]
def test_typeinfo_traits_value_types(t : T?) {
    t |> run("int is local") @(t : T?) {
        t |> equal(typeinfo is_local(type<int>), true)
    }
    t |> run("int can copy") @(t : T?) {
        t |> equal(typeinfo can_copy(type<int>), true)
    }
    t |> run("int can be placed in container") @(t : T?) {
        t |> equal(typeinfo can_be_placed_in_container(type<int>), true)
    }
    t |> run("float3 is local") @(t : T?) {
        t |> equal(typeinfo is_local(type<float3>), true)
    }
    t |> run("string can copy") @(t : T?) {
        t |> equal(typeinfo can_copy(type<string>), true)
    }
    t |> run("string can clone") @(t : T?) {
        t |> equal(typeinfo can_clone(type<string>), true)
    }
}

[test]
def test_typeinfo_traits_structs(t : T?) {
    t |> run("struct is local") @(t : T?) {
        t |> equal(typeinfo is_local(type<SimpleStruct>), true)
    }
    t |> run("struct can copy") @(t : T?) {
        t |> equal(typeinfo can_copy(type<SimpleStruct>), true)
    }
    t |> run("struct can move") @(t : T?) {
        t |> equal(typeinfo can_move(type<SimpleStruct>), true)
    }
    t |> run("struct can new") @(t : T?) {
        t |> equal(typeinfo can_new(type<SimpleStruct>), true)
    }
    t |> run("struct has no nontrivial ctor") @(t : T?) {
        t |> equal(typeinfo has_nontrivial_ctor(type<SimpleStruct>), false)
    }
    t |> run("struct has no nontrivial dtor") @(t : T?) {
        t |> equal(typeinfo has_nontrivial_dtor(type<SimpleStruct>), false)
    }
}

[test]
def test_typeinfo_traits_handled_types(t : T?) {
    // FileAccess is a C++ handled type from the ast module
    t |> run("FileAccess? is local") @(t : T?) {
        t |> equal(typeinfo is_local(type<FileAccess?>), true)
    }
    t |> run("FileAccess? can copy") @(t : T?) {
        t |> equal(typeinfo can_copy(type<FileAccess?>), true)
    }
    t |> run("FileAccess? can move") @(t : T?) {
        t |> equal(typeinfo can_move(type<FileAccess?>), true)
    }
    t |> run("FileAccess? cannot delete ptr") @(t : T?) {
        t |> equal(typeinfo can_delete_ptr(type<FileAccess?>), false)
    }
}

[test]
def test_typeinfo_traits_containers(t : T?) {
    t |> run("array is local") @(t : T?) {
        t |> equal(typeinfo is_local(type<array<int>>), true)
    }
    t |> run("array can move") @(t : T?) {
        t |> equal(typeinfo can_move(type<array<int>>), true)
    }
    t |> run("array can clone") @(t : T?) {
        t |> equal(typeinfo can_clone(type<array<int>>), true)
    }
    t |> run("table can move") @(t : T?) {
        t |> equal(typeinfo can_move(type<table<string; int>>), true)
    }
}
