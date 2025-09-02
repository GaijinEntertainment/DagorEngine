options gen2
// options log = true
// options log_aot = true

require daslib/regex
require daslib/regex_boost
require strings
require fio

let {
    test_and_log = true
    only_find_one_result = true
}

[export]
def test {
    static_if (test_and_log) {
        test_regex()
    }

    var r1 <- %regex~[\w\.+-]+@[\w\.-]+\.[\w\.-]+%%
    static_if (test_and_log) {
        regex_debug(r1)
    }

    let test = "first.last@learnxinyminutes.com"
    let res = regex_match(r1, test)
    assert(res != -1)

    let test2 = "first"
    let res2 = regex_match(r1, test2)
    assert(res2 == -1)

    // regex_debug(r1)
    let root = get_das_root()
    let input = root + "/../REGEX/input-text.txt"
    fopen(input, "rb") <| $(f) {
        if (f != null) {
            var dict : table<string; int>
            let data = fread(f)
            profile(5, "regex email") <| $() {
                regex_foreach(r1, data) <| $(r) {
                    static_if (test_and_log) {
                        print("match {slice(data,r.x,r.y)}\n")
                    }
                    return !only_find_one_result
                }
            }
        } else {
            print("can't open ")
        }
    }
    return true
}

def test_re(var regex : Regex; matches : array<string>; nonmatches : array<string>) {
    print("re is:\n")
    regex_debug(regex)
    var failed = false
    for (m in matches) {
        let ma = regex_match(regex, m)
        print("matching `{m}` -> {ma} {ma==-1 ? "failed" : "ok"}\n")
        failed ||= ma == -1
    }
    for (m in nonmatches) {
        let ma = regex_match(regex, m)
        print("^matching `{m}` -> {ma} {ma!=-1 ? "failed" : "ok"}\n")
        failed ||= ma != -1
    }
    print("{failed ? "failed" : "ok"}\n\n")
    assert(!failed)
    delete regex
}

def test_group(var regex : Regex; match : string; groups : array<string>) {
    print("re is:\n")
    regex_debug(regex)
    if (length(regex.groups) == 0) {
        print("has no groups\n")
        assert(false)
    }
    let ma = regex_match(regex, match)
    assert(ma != -1, "failed to match\n")
    let tot = length(regex.groups) - 1
    assert(tot == length(groups), "group count mismatch")
    var failed = false
    for (x in range(tot)) {
        let grp = regex_group(regex, x + 1, match)
        let cgrp = groups[x]
        if (grp != cgrp) {
            print("failed, expecting {x}={cgrp}, got {grp}")
            failed = true
        }
    }
    assert(!failed)
    delete regex
}

def test_replace(var regex : Regex; input : string; expected : string) {
    let res = regex_replace(regex, input, $(a) => "")
    print("replacing `{input}` -> `{res}`\n")
    print("expected `{expected}`\n")
    assert(res == expected)
    delete regex
}

def test_regex {
    // character
    test_re(%regex~a%%, [ "a"], array<string>("", "b", "ba"))
    // union
    test_re(%regex~cat|dog%%, array<string>("cat", "dog", "cats"), array<string>("at", " cat", "doog"))
    test_re(%regex~cat|dog|bat%%, array<string>("cat", "dog", "cats", "bats"), array<string>("at", " cat", "doog", "at"))
    // set & negatvie set
    test_re(%regex~[0-9a-zA-Z_]%%, array<string>("0", "3", "9", "a", "e", "z", "A", "Q", "Z", "_"), array<string>("#", "*"))
    test_re(%regex~[\w]%%, array<string>("0", "3", "9", "a", "e", "z", "A", "Q", "Z", "_"), array<string>("#", "*"))
    test_re(%regex~\w%%, array<string>("0", "3", "9", "a", "e", "z", "A", "Q", "Z", "_"), array<string>("#", "*"))
    test_re(%regex~[^0-9a-zA-Z_]%%, array<string>("#", "*"), array<string>("0", "3", "9", "a", "e", "z", "A", "Q", "Z", "_"))
    // any, concat
    test_re(%regex~cat.%%, array<string>("catt", "cats"), array<string>("cat", " cat"))
    test_re(%regex~(cat)(dog)(bat)%%, [ "catdogbat"], array<string>("cat", " cat", "catdog", "dogbat"))
    // eos
    test_re(%regex~cat$%%, [ "cat"], array<string>("cattt", " cat"))
    // +
    test_re(%regex~ab+%%, array<string>("ab", "abb", "abbbc"), array<string>("a", " ab", "bbb"))
    test_re(%regex~(cat)+%%, array<string>("cat", "catcat"), array<string>("dog", "caat"))
    // *
    test_re(%regex~ab*%%, array<string>("a", "ab", "abb", "abbbc"), array<string>(" ab", "bbb"))
    test_re(%regex~a(cat)*%%, array<string>("a", "acat", "acatcat"), array<string>("dog", "caat", "cat"))
    test_re(%regex~a*(cat)%%, array<string>("cat", "acat", "aaacat"), array<string>("dog", "caat"))
    // ?
    test_re(%regex~ab?%%, array<string>("a", "ab", "abb"), array<string>("b", " ab", "bbb"))
    test_re(%regex~(cat)?x%%, array<string>("catx", "x"), array<string>("dog", "caat"))
    // match2
    test_re(%regex~[a-z.]+.com%%, [ "abra.com"], array<string>("1", "2"))

    // gropus
    test_group( %regex~i have a (cat|dog)%%, "i have a cat.", [ "cat"])
    test_group( %regex~(this|that) is a (book|table|car)%%, "that is a table", array<string>("that", "table"))

    // replace
    test_replace( %regex~[^a-z]*%%, "ch!ar#l@y123be&%rt!", "charlybert")
}

