from "async" import Future

// Future.all rejects with the bare reason (the thrown value), not the faulted
// input future. Post-adoption-removal the combinator uses done.reject(e).

async function boom() { throw "the-reason" }
async function ok() { return 1 }

async function main() {
    try {
        let _ = await Future.all([ok(), boom(), ok()])
        print("BUG: all did not reject\n")
    }
    catch (e) {
        print("all rejected with: " + e + "\n")              // the-reason
        print("reason is string: " + (typeof e == "string") + "\n")  // true (bare value, not a Future)
    }
}
main()
print("script done\n")
