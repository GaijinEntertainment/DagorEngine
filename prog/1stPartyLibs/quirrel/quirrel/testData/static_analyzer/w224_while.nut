//-file:statement-on-same-line
//-file:invalid-indentation

function foo(x){ //-declared-never-used
  while (x) {
    x++;
    if (x > 5)
      break;
  } while (x < 6) ;
}