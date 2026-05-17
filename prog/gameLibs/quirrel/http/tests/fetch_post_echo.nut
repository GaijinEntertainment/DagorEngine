from "dagor.http" import httpFetch

if (__name__ == "__analysis__")
  return

//-file:undefined-global

// POST a small payload to /echo; the fixture mirrors the body back.

let baseUrl = ::__argv[3]

async function main() {
  let resp = await httpFetch({
    url = $"{baseUrl}/echo"
    method = "POST"
    data = "hello body"
  })
  println($"http_code = {resp.http_code}")
  print($"body = {resp.body.as_string()}")
}

println("start")
main()
println("end")
