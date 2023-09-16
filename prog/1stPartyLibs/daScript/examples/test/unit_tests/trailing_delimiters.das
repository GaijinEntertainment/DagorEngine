require dastest/testing_boost public


struct AAA
    x : int = 1
    y : int = 1


def test_int_array()
    var i0 <- [[auto 1; 2; 1;]]
    print("{i0}\n")

    var i1 <- [[int 1; 2; 1;]]
    print("{i1}\n")

    var i2 <- [[int[3] 1; 2; 1;]]
    print("{i2}\n")

    var i3 <- [{auto 1; 2;}]
    print("{i3}\n")

    var i4 <- [{auto 1;}]
    print("{i4}\n")

    var i5 <- [[auto[] 1;]]
    print("{i5}\n")

    let x = i0[i1[i2[0]]]
    print("{x}\n")


def test_structure_init()
    var a <- [[AAA
      x=1, y=2;
      x=3, y=4;  //
    ]]
    print("{a}\n")


    var b <- [[AAA()
      x=1, y=2;
      x=3, y=4;
    ]]
    print("{b}\n")


    var ad <- [{AAA
      x=1, y=2;
      x=3, y=4;  /* */ /* // 1234 */
    }]
    print("{ad}\n")


    var bd <- [{AAA()
      x=1, y=2;
      x=3, y=4;
    }]
    print("{bd}\n")


    var c <- [[AAA
      x=1,
      y=2,
    ]]
    print("{c}\n")


    var d <- [[AAA()
      x=1,
      y=2,  // /*
    ]]
    print("{d}\n")


    var e <- [[ AAA x=111, y=222; /* */ x=333, y=444;  /* */ ]]  /*
    */
    print("{e}\n")
    assert(e[1].y == 444)


    var f <- [[ AAA x=111, y=222 /*
      new line
    */; x=333, y=444;  /*
      new line
    */ ]]  /*
    */
    print("{f}\n")
    assert(f[1].y == 444)



def test_table_init()
    var t0 <- {{
      1 => 111;
      2 => 222; //
    }}
    print("{t0}\n")


def test_blocks()
    {}
    {
    }
    {;}
    { {}; }
    { {} /**/; };
    { {} ; /**/ }
    { { {}; };
    }
    {print("1"); print("2\n");}

    if true {print("12\n");}
    if true {if true {print("12\n");} else {print("34\n"); }; }
    if true {if true {print("12\n");} else {print("34\n"); } }


def test_mixed_blocks()
    let a0 <- {{ 1 => @(a : string) { print(a); } }}
    print("{a0}\n")

    if true {let a1 <- {{ 1 => @(a : string) { print(a); } }}; }

    let a2 <- {{ 1 => @(a : string) { let a3 <- {{ 1 => @(a : string) { print(a); } }}; } }}
    print("{a2}\n")

    let a3 <- {{ 1 => @(a : string) { let a3 <- {{ 1 => @(a : string) { print(a); }; }}; }; }};
    print("{a2}\n")


def test_string_interpolation()
    print("{[[auto[] 1;]]}\n")
    print("{ @(a : string) { print(a);} }\n")
    // print("{@(a : string) { print(a);}}\n") <<< broken :(

[export]
def test
    test_int_array()
    test_structure_init()
    test_table_init()
    test_blocks()
    test_mixed_blocks()
    test_string_interpolation()
    return true

















