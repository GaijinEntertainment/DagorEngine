from "dagor.http" import httpFetch

if (__name__ == "__analysis__")
  return

//-file:undefined-global

// 4xx is not a network failure: the Promise resolves with the response.
// The script decides what to do with the http_code.

let baseUrl = ::__argv[3]

async function main() {
  let resp = await httpFetch({ url = $"{baseUrl}/notfound", method = "GET" })
  println($"http_code = {resp.http_code}")
  println($"body = {resp.body.as_string()}")
}

println("start")
main()
println("end")
