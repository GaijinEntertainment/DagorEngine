// Test: Regex matching with deeply nested repetitions
// Tries to overflow stack during match execution
let { regexp } = require("string")

// Deep alternation chains that create recursive match paths
print("Test 1: nested groups with +\n")
local depths = [1, 10, 100, 300, 600, 900, 1900, 2800, 5000, 10000, 50000]
foreach (depth in depths) {
  local pattern = ""
  for (local i = 0; i < depth; i++)
    pattern += "("
  pattern += "a+"
  for (local i = 0; i < depth; i++)
    pattern += ")"
  print("  depth " + depth + "... ")
  try {
    local r = regexp(pattern)
    local res = r.search("aaa")
    print("OK\n")
  } catch(e) {
    print("ERROR: " + e + "\n")
    break
  }
}

// Test 2: Long alternation creating wide regex tree
print("Test 2: long alternation\n")
local alt_counts = [1, 10, 100, 300, 600, 900, 1900, 2800, 5000, 10000, 50000]
foreach (count in alt_counts) {
  local pattern = ""
  for (local i = 0; i < count; i++) {
    if (i > 0) pattern += "|"
    pattern += "a"
  }
  print("  alternatives " + count + "... ")
  try {
    local r = regexp(pattern)
    local res = r.search("a")
    print("OK\n")
  } catch(e) {
    print("ERROR: " + e + "\n")
    break
  }
}

// Test 3: Repetition operator with long input
print("Test 3: long match\n")
local lens = [1, 10, 100, 300, 600, 900, 1900, 2800, 5000, 10000, 50000]
foreach (len in lens) {
  local str = ""
  for (local i = 0; i < len; i++)
    str += "a"
  print("  len " + len + "... ")
  try {
    local r = regexp(@"a+")
    local res = r.search(str)
    print("OK\n")
  } catch(e) {
    print("ERROR: " + e + "\n")
    break
  }
}

print("ALL DONE\n")
