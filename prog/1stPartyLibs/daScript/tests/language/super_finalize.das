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

[unused_argument(self)]
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

// Skip-level inheritance: empty SkipMid in the middle of the chain.
// `delete super.self` in SkipLeaf must walk past SkipMid and call SkipBase's finalizer.
var skip_class_base = 0
var skip_class_leaf = 0

class SkipBase {
    def operator delete {
        skip_class_base ++
    }
}

class SkipMid : SkipBase {
}

class SkipLeaf : SkipMid {
    def operator delete {
        delete super.self
        skip_class_leaf ++
    }
}

// 4-level skip with two empty intermediates.
var deep_class_base = 0
var deep_class_leaf = 0

class DeepBase {
    def operator delete {
        deep_class_base ++
    }
}

class DeepMid1 : DeepBase {
}

class DeepMid2 : DeepMid1 {
}

class DeepLeaf : DeepMid2 {
    def operator delete {
        delete super.self
        deep_class_leaf ++
    }
}

// Mid-chain with finalizer: only DeepLeaf and one ancestor have finalizers,
// the immediate parent does not. Ensures walk-up still chains transitively.
var mid_class_base = 0
var mid_class_b = 0
var mid_class_leaf = 0

class MidBase {
    def operator delete {
        mid_class_base ++
    }
}

class MidB : MidBase {
    def operator delete {
        delete super.self
        mid_class_b ++
    }
}

class MidEmpty : MidB {
}

class MidLeaf : MidEmpty {
    def operator delete {
        delete super.self
        mid_class_leaf ++
    }
}

// Finalizer inheritance: a derived class with NO finalizer of its own chains to the
// ancestor's automatically — the generated finalizer cleans the derived slice and
// runs `delete super.self` on the immediate parent, recursing past empty levels.
var inh_base = 0

class InhBase {
    def operator delete {
        inh_base ++
    }
}

class InhDerived : InhBase {
}

class InhMid : InhBase {
}

class InhLeaf : InhMid {
}

// Slice cleanup + ordering: TrB has no finalizer, but its generated chain finalizer
// still finalizes TrB's own fields (Tracer observes it), in dtor order — derived
// body first, then each slice on the way up, base finalizer last.
var fin_order = ""

struct Tracer {
    id : int
}

def operator delete(var self : Tracer) {
    fin_order += "tracer{self.id};"
}

class TrA {
    def operator delete {
        fin_order += "A;"
    }
}

class TrB : TrA {
    tr_b : Tracer = Tracer(id = 2)
}

class TrC : TrB {
    tr_c : Tracer = Tracer(id = 3)
    def operator delete {
        fin_order += "C;"
        delete tr_c
        delete super.self
    }
}

// Struct skip-level: SkipStructMid has no finalizer (struct case relies on
// substitution at the immediate parent — we still verify it works).
var skip_struct_base = 0
var skip_struct_leaf = 0

struct SkipStructBase {
    a : int
}

[unused_argument(self)]
def operator delete(var self : SkipStructBase) {
    skip_struct_base ++
}

struct SkipStructMid : SkipStructBase {
    m : int
}

struct SkipStructLeaf : SkipStructMid {
    z : int
}

def operator delete(var self : SkipStructLeaf) {
    delete super.self
    skip_struct_leaf ++
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
    t |> run("delete super.self walks past empty intermediate class") @(t : T?) {
        skip_class_base = 0
        skip_class_leaf = 0
        var s = new SkipLeaf()
        unsafe {
            delete s
        }
        t |> equal(skip_class_base, 1)
        t |> equal(skip_class_leaf, 1)
    }
    t |> run("delete super.self walks past two empty intermediate classes") @(t : T?) {
        deep_class_base = 0
        deep_class_leaf = 0
        var d = new DeepLeaf()
        unsafe {
            delete d
        }
        t |> equal(deep_class_base, 1)
        t |> equal(deep_class_leaf, 1)
    }
    t |> run("delete super.self walks past empty mid-chain to nearest finalizer") @(t : T?) {
        mid_class_base = 0
        mid_class_b = 0
        mid_class_leaf = 0
        var m = new MidLeaf()
        unsafe {
            delete m
        }
        // MidLeaf -> (skip MidEmpty) -> MidB -> MidBase
        t |> equal(mid_class_base, 1)
        t |> equal(mid_class_b, 1)
        t |> equal(mid_class_leaf, 1)
    }
    t |> run("struct delete super.self walks past empty intermediate struct") @(t : T?) {
        skip_struct_base = 0
        skip_struct_leaf = 0
        var s = new SkipStructLeaf()
        unsafe {
            delete s
        }
        t |> equal(skip_struct_base, 1)
        t |> equal(skip_struct_leaf, 1)
    }
}

[test]
def test_finalizer_inheritance(t : T?) {
    t |> run("derived class without finalizer inherits the base finalizer") @(t : T?) {
        inh_base = 0
        var d = new InhDerived()
        unsafe {
            delete d
        }
        t |> equal(inh_base, 1)
    }
    t |> run("finalizer inheritance chains past an empty intermediate class") @(t : T?) {
        inh_base = 0
        var l = new InhLeaf()
        unsafe {
            delete l
        }
        t |> equal(inh_base, 1)
    }
    t |> run("generated chain finalizer cleans the intermediate slice in dtor order") @(t : T?) {
        fin_order = ""
        var c = new TrC()
        unsafe {
            delete c
        }
        // TrC body (logs C, deletes its own tracer), then TrB's generated chain
        // finalizer (deletes TrB's tracer), then TrA's finalizer.
        t |> equal(fin_order, "C;tracer3;tracer2;A;")
    }
}
