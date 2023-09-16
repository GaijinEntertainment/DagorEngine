options indenting = 4

module testing shared

require rtti
require strings // to compare strings/das_strings
require daslib/math_bits

class T
    name: string
    startTick: int64
    verbose: bool
    onLog: lambda<(msg: string; at: LineInfo): void>
    onFail: lambda<(now: bool): void>
    onSkipNow: lambda<(): void>
    onRun: lambda<(name: string; fn: RunT): void>

    failed: bool = false
    skipped: bool = false

    def T(name_: string; start_tick: int64; verbose_:bool)
        name = name_
        startTick = start_tick
        verbose = verbose_

    def const failNow()
        onFail |> invoke(true)
        panic("test failed")

    def const fail()
        onFail |> invoke(false)

    def const error(msg: string)
        errorAt(msg, get_line_info(1))

    def const errorAt(msg: string; at: LineInfo)
        logAt(msg, at)
        fail()

    def const fatal(msg: string)
        fatalAt(msg, get_line_info(1))

    def const fatalAt(msg: string; at: LineInfo)
        logAt(msg, at)
        failNow()

    def const log(msg: string)
        logAt(msg, get_line_info(1))

    def const logAt(msg: string; at: LineInfo)
        onLog |> invoke(msg, at)

    def const skipNow()
        onSkipNow |> invoke()
        panic("test skipped")

    def const skip(msg: string)
        skipAt(msg, get_line_info(1))

    def const skipAt(msg: string; at: LineInfo)
        logAt(msg, at)
        skipNow()

    def const getTimeUsec(): int
        return get_time_usec(startTick)

    def const getTimeSec(): double
        return double(getTimeUsec())/1000000.0lf


def equal(t: T?; a; b; msg = ""): bool
    if a == b
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->errorAt("\tgot: {b}", get_line_info(1))
    return false

[sideeffects]
def accept(t: T?; value)
    pass

def strictEqual(t: T?; a; b; msg = ""): bool
    if a == b
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false

def strictEqual(t: T?; a,b:float; msg = ""): bool
    if float_bits_to_int(a) == float_bits_to_int(b)
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false

def strictEqual(t: T?; a,b:float2; msg = ""): bool
    if float_bits_to_int(a) == float_bits_to_int(b)
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false

def strictEqual(t: T?; a,b:float3; msg = ""): bool
    if float_bits_to_int(a) == float_bits_to_int(b)
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false

def strictEqual(t: T?; a,b:float4; msg = ""): bool
    if float_bits_to_int(a) == float_bits_to_int(b)
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false


def strictEqual(t: T?; a,b:double; msg = ""): bool
    if double_bits_to_int64(a) == double_bits_to_int64(b)
        return true
    t->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    t->errorAt("\texpected: {a}", get_line_info(1))
    t->fatalAt("\tgot: {b}", get_line_info(1))
    return false

def success(t: T?; a; msg = ""): bool
    if a
        return true
    t->errorAt(!empty(msg) ? msg : "expected success, got failure", get_line_info(1))
    return false


typedef RunT = variant<lmd1: lambda<(t: T?): void>; func1: function<(t: T?): void>>


def run(t: T?; name: string; var cb: lambda<(t: T?): void>)
    t.onRun |> invoke(name, [[RunT lmd1<-cb]])


def run(t: T?; name: string; func: function<(t: T?): void>)
    t.onRun |> invoke(name, [[RunT func1=func]])
