//expect:w238

let function x() { //-declared-never-used
  ::a._must_be_utilized(::table2);
}
