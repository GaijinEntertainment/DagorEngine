//expect:w217

let function foo(x,y,z){ //-declared-never-used
  for (;;) {
    x++;
    y--;
    break;
    z--;
  }
}
