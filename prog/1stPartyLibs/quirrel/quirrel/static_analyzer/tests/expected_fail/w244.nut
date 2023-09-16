//expect:w244

local class A { //-declared-never-used
  function ns() {
  }

  static function fn() {
    print(ns())
  }
}
