from "types" import *

class ApiError { msg = "api" }
class NetError { msg = "net" }
class Sub(ApiError) { }

// typed match
try { throw ApiError() } catch (ApiError e) { println("match: " + e.msg) }

// typed miss falls through to the catch-all
try { throw NetError() }
catch (ApiError e) { println("BUG api") }
catch (e) { println("catchall: " + e.msg) }

// first-declared clause checked first; here the second matches
try { throw NetError() }
catch (ApiError e) { println("BUG api") }
catch (NetError e) { println("second: " + e.msg) }

// built-in type classes as catch types
try { throw "boom" } catch (String e) { println("string: " + e) }
try { throw 42 }
catch (String e) { println("BUG string") }
catch (Integer e) { println("integer: " + e) }

// subclass-inclusive matching
try { throw Sub() } catch (ApiError e) { println("subclass: " + e.msg) }

// a miss with no catch-all propagates to the outer try
try {
  try { throw NetError() } catch (ApiError e) { println("BUG inner") }
}
catch (e) { println("propagated: " + e.msg) }

// a throw in a catch body reaches the outer try, not a sibling clause
try {
  try { throw ApiError() }
  catch (ApiError e) { throw NetError() }
  catch (NetError e) { println("BUG sibling") }
}
catch (e) { println("rethrow-to-outer: " + e.msg) }

// Integer must not catch a Bool
try { throw true }
catch (Integer e) { println("BUG integer") }
catch (Bool e) { println("bool: " + e) }
