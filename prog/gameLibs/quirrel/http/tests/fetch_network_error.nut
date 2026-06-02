from "dagor.http" import httpFetch, HTTP_FAILED

if (__name__ == "__analysis__")
  return

//-file:undefined-global

// A request to a port we know nothing is listening on resolves with
// status = HTTP_FAILED. Documented failures travel as values, not throws.

async function main() {
  let resp = await httpFetch({
    url = "http://127.0.0.1:1/never"  // port 1 is reserved; nothing listens
    timeout_ms = 500
  })
  println($"resolved: status={resp.status} HTTP_FAILED={HTTP_FAILED} match={resp.status == HTTP_FAILED}")
}

println("start")
main()
println("end")
