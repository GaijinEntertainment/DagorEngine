from "dagor.http" import httpFetch

if (__name__ == "__analysis__")
  return

//-file:undefined-global

// A request to a port we know nothing is listening on rejects with
// {status = "FAILED"}. The rejection surfaces as an exception inside the
// awaiting function, caught here.

async function main() {
  try {
    let resp = await httpFetch({
      url = "http://127.0.0.1:1/never"  // port 1 is reserved; nothing listens
      timeout_ms = 500
    })
    println($"unexpected resolve: http_code = {resp.http_code}")
  } catch (err) {
    println($"caught: {err.status}")
  }
}

println("start")
main()
println("end")
