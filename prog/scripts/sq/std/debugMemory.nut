const bytes_per_allocation = 16
const bytes_per_object = 16

let function calculate_allocated_memory(obj) {
  local sum = 0
  local items = 0

  let typeName = type(obj)

  if (typeName == "table" || typeName == "class" || typeName == "array")
    foreach (v in obj) {
      items++
      sum += calculate_allocated_memory(v)
    }

  switch(typeName) {
    case "null": sum += bytes_per_object; break
    case "table": sum += bytes_per_object + bytes_per_allocation * 2 + 32 + max(items, 4) * bytes_per_object * 4; break
    case "array": sum += bytes_per_object + bytes_per_allocation * 2 + (items / 2) * bytes_per_object; break
    case "closure": sum += bytes_per_object + 144; break
    case "string": sum += bytes_per_object + 32 + bytes_per_allocation + ((obj.len() | 15) + 1); break
    case "userdata": sum += bytes_per_object + 32 + bytes_per_allocation; break
    case "integer": sum += bytes_per_object; break
    case "float": sum += bytes_per_object; break
    case "userpointer": sum += bytes_per_object + 32 + bytes_per_allocation; break
    case "function": sum += bytes_per_object + 32; break
    case "generator": sum += bytes_per_object + 64; break
    case "thread": sum += bytes_per_object + 32; break
    case "class": sum += bytes_per_object + bytes_per_allocation * 2 + 32 + max(items, 4) * bytes_per_object * 4; break
    case "instance": sum += bytes_per_object + 32; break
    case "bool": sum += bytes_per_object; break
    default: break
  }

  return sum
}

return {
  calculate_allocated_memory
}
