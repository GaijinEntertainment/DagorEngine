from "dagor.http" import httpFetch

if (__name__ == "__analysis__")
  return

//-file:undefined-global

let baseUrl = ::__argv[3]

async function main() {
  let resp = await httpFetch({ url = $"{baseUrl}/ok", method = "GET" })
  println($"http_code = {resp.http_code}")
  println($"body = {resp.body.as_string()}")
}

println("start")
main()
println("end")
