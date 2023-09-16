from "%darg/ui_imports.nut" import *
from "base64" import encodeBlob
from "iostream" import blob

let function mkBitmapPicture(w, h, fillcb, prefix="") {
  const HEADER_SIZE = 18
  let BITMAP_SIZE = w*h*4

  let b = blob()
  b.resize(HEADER_SIZE+BITMAP_SIZE)

  b.seek(0)
  b.writen(0, 'c') // id
  b.writen(0, 'c') // colormap
  b.writen(2, 'c') // true color
  b.writen(0, 's') // colormap start
  b.writen(0, 's') // colormap len
  b.writen(0, 'c') // colormap entry size
  b.writen(0, 's') // x origin
  b.writen(0, 's') // y origin
  b.writen(w, 's') // width
  b.writen(h, 's') // height
  b.writen(32,'c') // pixel depth
  b.writen(8, 'c') // alpha depth & dir

  for (local i=0;i<BITMAP_SIZE; ++i)
    b.writen(0, 'c')

  let function setPixel(x_, y_, c) {
    let x = x_.tointeger()
    let y = y_.tointeger()
    if (x<0 || x>=w || y<0 || y>=h)
      throw $"Invalid coordinates {x},{y} for bitmap of size {w}x{h}"
    b.seek(HEADER_SIZE+(y*w+x)*4)
    b.writen(c, 'i')
  }

  let function bufSetAt(idx, byte) {
    if (idx<0 || idx>BITMAP_SIZE)
      throw $"Invalid index {idx} for bitmap of size {w}x{h}"
    b.seek(HEADER_SIZE+idx)
    b.writen(byte, 'b')
  }

  let function bufGetAt(idx) {
    if (idx<0 || idx>BITMAP_SIZE)
      throw $"Invalid index {idx} for bitmap of size {w}x{h}"
    b.seek(HEADER_SIZE+idx)
    return b.readn('b')
  }

  fillcb({w, h}, {setPixel, bufSetAt, bufGetAt})

  return Picture($"{prefix}b64://{encodeBlob(b)}.tga?Ac")
}

return {
  mkBitmapPicture
}
