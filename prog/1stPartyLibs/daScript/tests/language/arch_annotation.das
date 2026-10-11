options gen2
require dastest/testing_boost public

// [arch(at="<doc>.md#<anchor>")] binds code to a section of an architecture document. It is an
// inert marker registered in the builtin module, so citing a document costs a build nothing and
// needs no require; whether the citation RESOLVES is LINT026's question, not the compiler's.
// This file is the compile-side contract: the shapes that must be accepted, everywhere.

[arch(at="_arch_annotation_doc.md#tokenizer")]
def cited_function : int {
    return 17
}

// one function may answer for more than one section
[arch(at="_arch_annotation_doc.md#tokenizer"), arch(at="_arch_annotation_doc.md#buffers")]
def twice_cited_function : int {
    return 4
}

// the marker composes with the annotations it sits beside
[never_inline, arch(at="_arch_annotation_doc.md#tokenizer")]
def cited_beside_another : int {
    return 9
}

[arch(at="_arch_annotation_doc.md#token")]
struct CitedStruct {
    value : int
}

[arch(at="_arch_annotation_doc.md#lexer")]
class CitedClass {
    count : int
    def bump {
        count ++
    }
}


[test]
def test_arch_annotation_compiles(t : T?) {
    t |> run("a cited function is an ordinary function") @(t : T?) {
        t |> equal(cited_function(), 17, "the marker changes no behavior")
        t |> equal(twice_cited_function(), 4, "two citations are legal on one function")
        t |> equal(cited_beside_another(), 9, "the marker composes with other annotations")
    }
    t |> run("a cited structure is an ordinary structure") @(t : T?) {
        var s = CitedStruct(value = 5)
        s.value ++
        t |> equal(s.value, 6, "fields behave as declared")
    }
    t |> run("a cited class is an ordinary class") @(t : T?) {
        var c = new CitedClass()
        c->bump()
        c->bump()
        t |> equal(c.count, 2, "methods behave as declared")
        unsafe {
            delete c
        }
    }
}
