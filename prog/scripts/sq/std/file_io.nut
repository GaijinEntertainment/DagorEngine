from "iostream" import blob
let {file} = require("io")

let function readFileAsBlob(fileName, mode = "") {
  let f = file(fileName, $"r{mode}")
  let len = f.len()
  let contents = f.readblob(len)
  f.close()
  return contents
}

let readFileAsString = @(fileName, mode = "") readFileAsBlob(fileName, mode).as_string()

let function writeBlobToFile(fileName, contents, mode = "") {
  let f = file(fileName, $"w{mode}")
  f.writeblob(contents)
  let len = f.len()
  f.close()
  return len
}

let function writeStringToFile(fileName, contentsStr, mode = "") {
  let contents = blob()
  contents.writestring(contentsStr)
  return writeBlobToFile(fileName, contents, mode)
}

return {
  readFileAsBlob
  readFileAsString
  writeBlobToFile
  writeStringToFile
}