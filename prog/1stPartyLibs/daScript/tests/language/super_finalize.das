// Tests `delete super.self` — calling the base class/struct finalizer from a derived finalizer.
options gen2
require dastest/testing_boost public

var class2_del_base = 0
var class2_del_derived = 0

class ClassBase {
    def operator delete {
        class2_del_base ++
    }
}

class ClassDerived : ClassBase {
    def operator delete {
        delete super.self
        class2_del_derived ++
    }
}

var class3_del_a = 0
var class3_del_b = 0
var class3_del_c = 0

class ClassA {
    def operator delete {
        class3_del_a ++
    }
}

class ClassB : ClassA {
    def operator delete {
        delete super.self
        class3_del_b ++
    }
}

class ClassC : ClassB {
    def operator delete {
        delete super.self
        class3_del_c ++
    }
}

var struct_del_base = 0
var struct_del_derived = 0

struct StructBase {
    a : int
}

def operator delete(var self : StructBase) {
    struct_del_base ++
}

struct StructDerived : StructBase {
    b : int
}

def operator delete(var self : StructDerived) {
    delete super.self
    struct_del_derived ++
}

[test]
def test_delete_super_self(t : T?) {
    t |> run("class derived operator delete chains to base via super.self") @(t : T?) {
        class2_del_base = 0
        class2_del_derived = 0
        var d = new ClassDerived()
        unsafe {
            delete d
        }
        t |> equal(class2_del_base, 1)
        t |> equal(class2_del_derived, 1)
    }
    t |> run("3-level class hierarchy chains all finalizers") @(t : T?) {
        class3_del_a = 0
        class3_del_b = 0
        class3_del_c = 0
        var c = new ClassC()
        unsafe {
            delete c
        }
        t |> equal(class3_del_a, 1)
        t |> equal(class3_del_b, 1)
        t |> equal(class3_del_c, 1)
    }
    t |> run("struct free-function finalizer chains via super.self") @(t : T?) {
        struct_del_base = 0
        struct_del_derived = 0
        var d = new StructDerived()
        unsafe {
            delete d
        }
        t |> equal(struct_del_base, 1)
        t |> equal(struct_del_derived, 1)
    }
}
