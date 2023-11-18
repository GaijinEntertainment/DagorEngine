const bytes_per_allocation = 16
const bytes_per_object = 16

let {max} = require("math")


function calculate_allocated_memory(obj) {
  local sum = 0
  local items = 0

  let typeName = type(obj)

  if (typeName == "table" || typeName == "class" || typeName == "array")
    foreach (v in obj) {
      items++
      sum += calculate_allocated_memory(v)
    }

  if (typeName == "null")
    sum += bytes_per_object
  else if (typeName == "table")
    sum += bytes_per_object + bytes_per_allocation * 2 + 32 + max(items, 4) * bytes_per_object * 4
  else if (typeName == "array")
    sum += bytes_per_object + bytes_per_allocation * 2 + (items / 2) * bytes_per_object
  else if (typeName == "closure")
    sum += bytes_per_object + 144
  else if (typeName == "string")
    sum += bytes_per_object + 32 + bytes_per_allocation + ((obj.len() | 15) + 1)
  else if (typeName == "userdata")
    sum += bytes_per_object + 32 + bytes_per_allocation
  else if (typeName == "integer")
    sum += bytes_per_object
  else if (typeName == "float")
    sum += bytes_per_object
  else if (typeName == "userpointer")
    sum += bytes_per_object + 32 + bytes_per_allocation
  else if (typeName == "function")
    sum += bytes_per_object + 32
  else if (typeName == "generator")
    sum += bytes_per_object + 64
  else if (typeName == "thread")
    sum += bytes_per_object + 32
  else if (typeName == "class")
    sum += bytes_per_object + bytes_per_allocation * 2 + 32 + max(items, 4) * bytes_per_object * 4
  else if (typeName == "instance")
    sum += bytes_per_object + 32
  else if (typeName == "bool")
    sum += bytes_per_object
  return sum
}

return {
  calculate_allocated_memory
}
