options gen2
options indenting = 4
options rtti

module suite_result shared

require daslib/json_boost

struct SuiteResult {
    passed : int// number of passed tests
    failed : int// number of failed tests
    errors : int// compile, runtime error
    total : int// total number of tests
    skipped : int// number of skipped tests
}

struct SuiteTestResult {
    name : string
    passed : bool
    skipped : bool
    @rename = "time" time_usec : int  // microseconds
    messages : array<string>          // collected log/error messages
    location : string                 // "file:line" of first failure (empty if passed)
}


def operator +(var a : SuiteResult; b : SuiteResult | #) : SuiteResult {
    var res := a
    res += b
    return res
}


def operator +=(var a : SuiteResult; b : SuiteResult | #) : void {
    a.passed += b.passed
    a.failed += b.failed
    a.total += b.total
    a.errors += b.errors
    a.skipped += b.skipped
}


def operator ==(a : SuiteResult | #; b : SuiteResult | #) : bool {
    return (
        a.passed == b.passed &&
        a.failed == b.failed &&
        a.total == b.total &&
        a.errors == b.errors &&
        a.skipped == b.skipped
    )
}


def operator !=(a : SuiteResult | #; b : SuiteResult | #) : bool {
    return !(a == b)
}


def SuiteResult(val : JsonValue?) {
    let obj & = unsafe(val as _object)
    return SuiteResult(
        passed = from_JV(obj?["passed"] ?? JV(0), type<int>),
        failed = from_JV(obj?["failed"] ?? JV(0), type<int>),
        total = from_JV(obj?["total"] ?? JV(0), type<int>),
        errors = from_JV(obj?["errors"] ?? JV(0), type<int>),
        skipped = from_JV(obj?["skipped"] ?? JV(0), type<int>)
    )
}


def SuiteTestResult(val : JsonValue?) {
    let obj & = unsafe(val as _object)
    var msgs : array<string>
    unsafe {
        let msgsJv = obj?["messages"] ?? JVNull()
        if (msgsJv != null && msgsJv.value is _array) {
            for (m in msgsJv.value as _array) {
                msgs |> push(from_JV(m, type<string>))
            }
        }
    }
    return SuiteTestResult(
        name = from_JV(unsafe(obj?["name"]) ?? JV(""), type<string>),
        passed = from_JV(unsafe(obj?["passed"]) ?? JV(false), type<bool>),
        skipped = from_JV(unsafe(obj?["skipped"]) ?? JV(false), type<bool>),
        time_usec = from_JV(unsafe(obj?["time"]) ?? JV(0), type<int>),
        messages <- msgs,
        location = from_JV(unsafe(obj?["location"]) ?? JV(""), type<string>)
    )
}


def parse_test_results(val : JsonValue?; var results : array<SuiteTestResult>) {
    let obj & = unsafe(val as _object)
    unsafe {
        let testsJv = obj?["tests"] ?? JVNull()
        if (testsJv != null && testsJv.value is _array) {
            for (t in testsJv.value as _array) {
                results |> emplace(SuiteTestResult(t))
            }
        }
    }
}


def to_string(res : SuiteResult) : string {
    return "{res.total} tests, {res.passed} passed, {res.failed} failed, {res.errors} errors, {res.skipped} skipped"
}
