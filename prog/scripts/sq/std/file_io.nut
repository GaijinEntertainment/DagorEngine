from "iostream" import blob
let {file} = require("io")

let function readFileAsString(fileName, mode="") {
  let f = file(fileName, $"r{mode}")
  let len = f.len()
  let contents = f.readblob(len)
  f.close()
  return contents.as_string()
}


let function writeStringToFile(fileName, contentsStr, mode="") {
  let contents = blob()
  contents.writestring(contentsStr)
  let f = file(fileName, $"w{mode}")
  f.writeblob(contents)
  let len = f.len()
  f.close()
  return len
}

return {
  readFileAsString
  writeStringToFile
}