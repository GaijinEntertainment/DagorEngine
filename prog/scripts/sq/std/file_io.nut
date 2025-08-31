from "iostream" import blob
from "io" import file
from "underscore.nut" import do_in_scope

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

function doInFile(filePath, flags, action){
  do_in_scope({
    __enter__ = @() file(filePath, flags)
    __exit__ = @(file_handle) file_handle.close()
  }, action)
}

return freeze({
  readFileAsBlob
  readFileAsString
  writeBlobToFile
  writeStringToFile
  doInFile
})