import "DataBlock" as DataBlock
from "dagor.fs" import file_exists, can_read_text_from_file, read_text_from_file

function readPackedText(path: string): string|null {
  if (can_read_text_from_file(path))
    return read_text_from_file(path)
  let blk = DataBlock()
  return blk.tryLoad($"{path}.blk", true) ? "".join(blk % "text") : null
}

let packedTextExists = @(path: string): bool can_read_text_from_file(path) || file_exists($"{path}.blk")

return freeze({
  readPackedText
  packedTextExists
})
