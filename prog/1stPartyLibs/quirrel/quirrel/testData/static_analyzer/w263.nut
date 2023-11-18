for (local i = 0; i < 5; i++) // EXPECTED
{
  ::print(i)
}

function _foo(_p) // EXPECTED
{

}


function bar() { // FP
    return false
}

class _A { // FP

}

class _B // EXPECTED
{

}

enum _E1 { // FP

}

enum _E2    // EXPECTED
{

}

try { // FP
    print(6)
} catch (_e) { // FP
    print(7)
}

try // EXPECTED
{
    print(8)
} catch (_e) // EXPECTED
{
    print(9)
}

local t = bar()

if (t) {    // FP
    print(10)
} else { // FP
    print(11)
}

if (!t) // EXPECTED
{
    print(12)
} else  // EXPECTED
{
    print(13)
}

while (t) { // FP
    print(14)
}

while (t)   // EXPECTED
{
    print(15)
}

do {    // FP
    print(16)
} while (t)

do  // EXPECTED
{
    print(17)
} while (t)

switch (t) {    // FP
    case 1:
        break;
    case 2: { // FP
        break;
    }
    default:    // EXPECTED
    {
        break;
    }
}

switch (t)  // EXPECTED
{
    case 2:
        break;
    default:
        break;
}