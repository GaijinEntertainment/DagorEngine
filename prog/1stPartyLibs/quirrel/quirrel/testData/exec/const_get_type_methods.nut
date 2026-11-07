// A constant read finds a type method only where the same run-time read finds it
foreach (src in [
  "const L = \"abc\"[\"len\"]\nreturn L",
  "const L = \"abc\"[\"len\"]()\nreturn L",
  "const L = (\"abc\"[\"len\"])()\nreturn L",
  "const function [pure] make() { return \"abc\" }\nreturn typeof make()[\"len\"]",
  "#forbid-implicit-type-methods\nconst L = \"abc\".len\nreturn L",
  "#forbid-implicit-type-methods\nconst L = \"abc\"[\"len\"]()\nreturn L",
  "#forbid-implicit-type-methods\nconst L = \"abc\".$len()\nreturn L",
]) {
  try {
    println(compilestring(src, "reads")())
  }
  catch (e) {
    println(e)
  }
}
