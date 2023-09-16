expect 30508:3

struct Foo
    bar : table<string; int>

[export]
def test
    let a <- {{ "one" => 1; "two" => 2 }}
    let b <- {{ "a" => a }}                         // 30508: can't move from a constant value
    print("a={a} b={b}\n")

    let aa <- {{ "one" => 1; "two" => 2 }}
    let bb <- [[ Foo bar <- aa ]]                   // 30508: can't move from a constant value
    print("aa={aa} bb={bb}\n")

    let aaa <- {{ "one" => 1; "two" => 2 }}
    let bbb <- [[ table<string;int>[1] aaa ]]       // 30508: can't move from a constant value
    print("aaa={aaa} bbb={bbb}\n")

    return true
