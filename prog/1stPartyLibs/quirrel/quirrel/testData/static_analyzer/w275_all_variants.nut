

function foo() {}

let x = foo()

switch (x) {
    case 1:
        foo();   // OK
    case 2: {
            {
                foo();
                {
                    foo();
                    break;   // WRONG
                }
            }
        }
    case 3:
        foo()    // OK
    default:
        foo()
        break;   // WRONG
}


switch (x) {
    case 2:
        foo();
        break;    // WRONG
    case 3:
        foo();
        ;         // WRONG -missed-break
    default:
        foo()     // WRONG
}


switch (x) {
    case 1:         //WRONG
    case 2:
        foo();
        break;      // WRONG
    case 3:
        foo()
        ;           // WRONG -missed-break
}


switch (x) {
    case 1:
        foo()
        break       // WRONG
    case 2:
    case 3:         // WRONG
}