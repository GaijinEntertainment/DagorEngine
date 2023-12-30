from "iostream" import blob
let {file} = require("io")

function readFileAsBlob(fileName, mode = "") {
  let f = file(fileName, $"r{mode}")
  let len = f.len()
  let contents = f.readblob(len)
  f.close()
  return contents
}

let readFileAsString = @(fileName, mode = "") readFileAsBlob(fileName, mode).as_string()

function writeBlobToFile(fileName, contents, mode = "") {
  let f = file(fileName, $"w{mode}")
  f.writeblob(contents)
  let len = f.len()
  f.close()
  return len
}

function writeStringToFile(fileName, contentsStr, mode = "") {
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