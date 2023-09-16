options indenting = 4

module suite_result shared

require daslib/json_boost

struct SuiteResult
    passed: int // number of passed tests
    failed: int // number of failed tests
    errors: int // compile, runtime error
    total: int // total number of tests
    skipped: int // number of skipped tests


def operator +(var a: SuiteResult; b: SuiteResult): SuiteResult
    var res := a
    res += b
    return res


def operator +=(var a: SuiteResult; b: SuiteResult): void
    a.passed += b.passed
    a.failed += b.failed
    a.total += b.total
    a.errors += b.errors
    a.skipped += b.skipped


def operator ==(a: SuiteResult; b: SuiteResult): bool
    return (
        a.passed == b.passed &&
        a.failed == b.failed &&
        a.total == b.total &&
        a.errors == b.errors &&
        a.skipped == b.skipped
    )


def operator !=(a: SuiteResult; b: SuiteResult): bool
    return !(a == b)


def SuiteResult(val: JsonValue?)
    let obj & = unsafe(val as _object)
    return [[SuiteResult
        passed=int((obj?["passed"] ?? JV(0lf)) as _number),
        failed=int((obj?["failed"] ?? JV(0lf)) as _number),
        total=int((obj?["total"] ?? JV(0lf)) as _number),
        errors=int((obj?["errors"] ?? JV(0lf)) as _number),
        skipped=int((obj?["skipped"] ?? JV(0lf)) as _number)
    ]]


def to_string(res: SuiteResult): string
    return "{res.total} tests, {res.passed} passed, {res.failed} failed, {res.errors} errors, {res.skipped} skipped"
