
local x = 10
local y = 20

if (x)
  if (y)
    print(1)
else    // EXPECTED 1
  print(2)

if (x)
    print(3)
    print(4) // EXPECTED 2

while (false)
    print(x)
    print(y) // EXPECTED 3

while (false)
    print(x)
print(y) // FP 1

while (false)
print(x)
    print(y) // EXPECTED 4

for (;false;)
    print(x)
    print(y) // EXPECTED 5

for (;false;)
    print(x)
print(y) // FP 2


foreach (_z in [])
    print(x)
    print(y) // EXPECTED 6

foreach (_z in [])
    print(x)
print(y) // FP 3


if (x) {
    print(10)
} else // FP 4
    print(20)

if (x) {
    print(10)
} else if (y) { // FP 5
    print(20)
}

if (x) // EXPECTED 7
if (y)
  print(1)
else
print(2)


function _f1() {
print(1)
print(2)
}


function _f2() {
print(1)
print(2)
}


if (x); // -empty-body
print(3)


while (x > 100); // -empty-body
print(3)


for (;false;); // -empty-body
print(3)

while (x > 100)
print(3) // EXPECTED 8


for (;false;)
print(3) // EXPECTED 9



local e = 10, d = 20, _z = null, aD = 2, eD = 3

if (e){
    _z = { s = "e", ss = "p" }

} else if (d > 0) { // FP 6
    if (aD > eD)
        _z = { s = "a", ss = "p" }
    else
        _z = { s = "e", ss = "m" }

}