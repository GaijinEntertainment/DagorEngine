//expect:w217

function foo(x,y,z){ //-declared-never-used
  for (;;) {
    x++;
    y--;
    break;
    z--;
  }
}
