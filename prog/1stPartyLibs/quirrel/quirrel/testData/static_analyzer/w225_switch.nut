#allow-switch-statement

function foo(_p) {}
function _dataToBlk(data) {
    let blk = {}
    let dataType = foo(data) ? "DataBlock" : foo(data)
    switch (dataType) {
      case "null":
        return "__null"
      case "bool":
      case "integer":
      case "float":
      case "string":
        return data
      case "array":
      case "table":
        foreach (key, value in data) {
          foo(key)
          foo(value)
        }
        return "04"
      case "DataBlock":
        blk.setFrom(data)
        blk.__datablock <- true
        return "999"
      default:
        return "__unsupported "
    }
  }