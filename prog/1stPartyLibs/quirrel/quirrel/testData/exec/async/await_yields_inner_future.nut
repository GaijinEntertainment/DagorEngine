from "async" import Future

// await unwraps exactly ONE level (no adoption). A Future resolved with another
// Future yields that inner Future; a second await reaches the value. An async
// function that returns a Future fulfils with it verbatim; `return await`
// forwards the value instead.

async function returnsFuture() { let f = Future(); f.resolve(9); return f }
async function returnsValue()  { let f = Future(); f.resolve(9); return await f }

async function main() {
    let inner = Future(); inner.resolve(7)
    let outer = Future(); outer.resolve(inner)   // stored verbatim
    let got = await outer
    print("got is inner: " + (got == inner) + "\n")   // true
    print("got.getValue: " + got.getValue() + "\n")   // 7
    print("deep: " + (await got) + "\n")              // 7

    let a = await returnsFuture()                     // a is the inner Future
    print("a.getState: " + a.getState() + "\n")       // fulfilled
    print("a deep: " + (await a) + "\n")              // 9
    print("return await: " + (await returnsValue()) + "\n")  // 9
}
main()
print("script done\n")
