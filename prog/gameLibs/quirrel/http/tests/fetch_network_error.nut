from "dagor.http" import httpFetch, HTTP_FAILED

if (__name__ == "__analysis__")
  return

//-file:undefined-global

// A request to a port we know nothing is listening on rejects with
// {status = HTTP_FAILED}. The rejection surfaces as an exception inside the
// awaiting function, caught here.

async function main() {
  try {
    let resp = await httpFetch({
      url = "http://127.0.0.1:1/never"  // port 1 is reserved; nothing listens
      timeout_ms = 500
    })
    println($"unexpected resolve: http_code = {resp.http_code}")
  } catch (err) {
    println($"caught: status={err.status} HTTP_FAILED={HTTP_FAILED} match={err.status == HTTP_FAILED}")
  }
}

println("start")
main()
println("end")
