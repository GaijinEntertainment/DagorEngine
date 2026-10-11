options gen2
options no_aot

require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost
require daslib/ast_print
require strings

def private fixture_path() : string {
    return "{get_das_root()}/tests/ast/_string_null_byte_fixture.das"
}

def private returned_constant(fn : FunctionPtr) : ExprConstString? {
    let blk = fn.body ?as ExprBlock
    return null if (blk == null || empty(blk.list))
    let ret = blk.list[0] ?as ExprReturn
    return ret != null ? (ret.subexpr ?as ExprConstString) : null
}

def private check_escape(t : T?; mod : Module?) {
    var blob = returned_constant(find_unique_function(mod, "returned_a"))
    t |> success(blob != null, "returned_a does not return a string constant")
    return if (blob == null)
    t |> equal(escape(blob.value), "x\\u0000a")
    t |> equal(describe(blob), "\"x\\x00a\"")
    var expr : ExpressionPtr = blob
    let printed = build_string() $(var writer) {
        printExpr(expr, unsafe(addr(writer)))
    }
    t |> equal(printed, "\"x\\u0000a\"")
}

def private check_hash(t : T?; mod : Module?) {
    let hash_of = @(name : string) : uint64 => find_unique_function(mod, name).hash
    t |> success(hash_of("returned_a") != hash_of("returned_b"), "returned blobs hash alike")
    t |> success(hash_of("compared_a") != hash_of("compared_b"), "compared blobs hash alike")
    t |> equal(hash_of("plain_a"), hash_of("plain_b"))
}

[test]
def test_string_null_byte(t : T?) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                t |> success(ok, "the fixture did not compile: {issues}")
                return if (!ok || program == null)
                let mod = get_this_module(program)
                check_escape(t, mod)
                simulate(program) $(sok; _context; serrors) {
                    t |> success(sok, "the fixture did not simulate: {serrors}")
                    return if (!sok)
                    check_hash(t, mod)
                }
            }
        }
    }
}
