require fio
require strings
require daslib/async_boost
require daslib/coroutines


let begin = ref_time_ticks()

def log(msg)
    print("{fmt(":0.3f", float(get_time_usec(begin)) / 1000000.)}: {msg}")

struct Data
    a : int
    b : string
    arr : array<int>

[async] def get_plus_1(a : int) : int
    log("get_plus_1 1\n")
    await_next_frame() // frame pause
    log("get_plus_1 2\n")
    yield a + 1
    // return false // force exit

[async] def get_data(a : int) : Data
    if a < 0
        return false // returns empty data: [[Data]]
    log("get_data 1\n")
    await_next_frame()
    log("get_data 2\n")
    var data : Data
    data.a = a
    data.b = "hello"
    data.arr <- [{int 1; }]
    yield <- data

[async] def async_range() : int
    yield 1
    yield 2
    yield 3

[async] def bool_range() : bool
    yield true
    yield false
    yield true

[coroutine] def just_coroutine(n)
    for i in 0..n
        yield true

[async] def getall() : void
    var a = await <| get_plus_1(100)
    debug(a, "1+100")
    assert(a == 101)

    a <- await <| get_plus_1(200)
    debug(a, "1+200")
    assert(a == 201)

    log("pass frame\n")
    await_next_frame()

    // await <| get_data(1) // 40102: call annotated by AwaitMacro failed to transform, result should be assigned to a variable, use 'let res <- await(get_data(1))'
    let res <- await <| get_data(1)
    debug(res, "res")
    assert(res.a == 1)
    assert(res.b == "hello")
    assert(res.arr[0] == 1)
    // res = await <| get_data(1) // 30507: this type can't be copied, Data& -const = Data

    let noRes <- await <| get_data(-1) // empty result
    debug(noRes, "noRes")
    assert(noRes.a == 0)
    assert(noRes.b == "")
    assert(length(noRes.arr) == 0)


// coroutine
[async] def getrealall() : void
    log("getrealall 1\n")
    await_next_frame()
    log("getrealall 2\n")
    await_next_frame()
    log("getrealall 3\n")
	await <| getall() // ok, async function with void result

    log("async_range\n")
    var ints = 0
    for idx, i in count(), async_range()
        print("int {i}\n")
        assert(i is res)
        assert(idx + 1 == (i as res))
        ints++
        await_next_frame()
    assert(ints == 3)

    log("bool_range\n")
    var bools = 0
    for idx, b in count(), bool_range()
        print("bool {b}\n")
        assert(b is res)
        assert(b as res == (idx % 2 == 0))
        bools++
        await_next_frame()
    assert(bools == 3)

    let n = 5
    log("wait coroutine ({n} frames)\n")
    await <| just_coroutine(n)
    log("coroutine done\n")


[export]
def main()
    log("----\n")
    var all <- getrealall()
    var foo : bool
    while next(all, foo)
        log("step...\n")
        sleep(500u)
    log("----\n")
