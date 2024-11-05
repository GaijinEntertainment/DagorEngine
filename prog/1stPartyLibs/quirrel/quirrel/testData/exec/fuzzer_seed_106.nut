// #disable-optimizer
// seed = 106
try{
local count = 0
let to_log = function(id, s) { let tp = typeof(s); print($"{id}: "); print(tp == "integer" || tp == "float" || tp == "string" ? s : tp); print("\n"); if (count++ > 1000) print["stop"]; }
{
  let t6 = { }
  let a7 = [ (-5), ]
  let t54 = { f0=-3317007163456823286 f1=-4 }
  let t55 = { }
  let a57 = [ ]
  let v58 = -2
  let v60 = 3
  let t59 = { f0=((v60) & (11)) }
  let a61 = [ 4, 11, ]
  let t62 = { }
  let a64 = [ (((13 > 10 - 9)) ? ((6)) : (4)), ]
  let v65 = (((7 != 8)) ? ((((-5 == -4)) ? (5) : (6))) : (2)) - 9
  let a63 = [ 5, (a64?[0] ?? v65), ]
  let t56 = { f0=-1 f1=(((((((a57?[0] ?? 7) == v58)) ? (11) : (2)) == ((-3 - (((((4) | (-4)) < ((1) & (3)))) ? (-4 - 10) : (13 - 467615902768935684))) & (10)))) ? ((t59?["f1"] ?? ((5) & (10))) + (((((((10 >= 2)) ? (1) : ((a61?[1] ?? (t62?["f1"] ?? 8))))) > 10 + 0 + (((10 == 6)) ? (9) : (2)))) ? (6) : ((-1)))) : ((a63?[1] ?? -2))) }
  let a66 = [ ]
  let t67 = { f0=2 }
  let fni68 = @() 12
  let v69 = 13
  let fni70 = @() 8
  let a71 = [ ]
  let v72 = v60
  let v74 = -2
  let a73 = [ (v74), ]
  let v76 = -3
  let v75 = v76
  let t78 = { }
  let a79 = [ ]
  let t80 = { }
  let t81 = { }
  let v82 = 8
  {
    local loop0 = 1
    while (loop0++ < -4)
    {
      let v2 = (-3)
      let v3 = 9
      let a4 = [ ]
      let v5 = (2)
      let t1 = { f0=v2 f1=(v3 + (a4?[1] ?? -5 - v5)) }
      continue
      to_log("1", t1.f1)
      to_log("2", v2)
      to_log("3", v3)
      to_log("4", a4)
      to_log("5", v5)
      to_log("6", t1)
    }
  }
  {
    local loop8 = (t6?.f0 ?? a7[0])
    while (loop8++ < -1)
    {
      let gen11 = function () { yield 14; yield 7; yield 6; }
      let t52 = { }
      let a53 = [ 12 + -5, -3, ]
      foreach (t9, t10 in gen11())
      {
          let v51 = (((t52?["f1"] ?? (13))) & ((((a53?[1] ?? -1)) & (-5))))
          break
          to_log("7", 1)
          break
          if (-2)
          {
              let v50 = v51
              let fn12 = function fn12(p28, p30, p34) {
                let fn13 = function fn13() {
                  let v16 = {m0 = -5, m1 = 6, m2 = 11, }
                  let fn17 = function fn17(p18) {
                      to_log("8", (((4 > 12)) ? (-4) : (11)))
                      to_log("9", 8)
                      to_log("10", 5 + p18)
                      try
                      {
                          let v19 = 7
                          to_log("11", v19)
                          to_log("12", -3)
                          to_log("13", v19)
                        }
                      catch(err20)
                      {
                          to_log("14", ((11) | (9)))
                        }
                    }
                  foreach (t14, t15 in v16)
                    fn17(7)
                  to_log("15", v16)
                  to_log("16", fn17)
                }
                let v22 = 6
                let fni21 = @() ((v22) | (((9) & (10))))
                let a25 = [ 1, ]
                let v27 = (((-3 <= -5)) ? (2) : (v22))
                let fni26 = @() v27
                let v29 = -1
                fn13()
                {
                  local loop23 = ((-4500801829576294150) | (fni21()))
                  while (loop23++ < (a25?[0] ?? fni26()))
                  {
                    let a24 = [ ]
                    to_log("17", 7)
                    to_log("18", (a24?[1] ?? (-1)))
                    to_log("19", a24)
                  }
                }
                to_log("20", t10)
                to_log("21", -1 - p28)
                if (v29)
                  to_log("22", 7)
                else
                {
                    let fni49 = @() 12
                    to_log("23", p30)
                    try
                    {
                        to_log("24", 14)
                        0/0
                      }
                    catch(err43)
                    {
                        let gen33 = function () { return 2; yield 13; yield 14; }
                        let fn35 = function fn35() {
                          let v37 = (((3 == -4)) ? (-1) : (3))
                          let gen42 = function () { }
                          for (local loop36 = 0; loop36 < v37; loop36++)
                          {
                              let fn38 = function fn38() {
                                let v39 = 5
                                to_log("29", v39)
                                to_log("30", 5)
                                to_log("31", v39)
                              }
                              to_log("28", -8447515169283609172)
                              fn38()
                              to_log("32", fn38)
                            }
                          to_log("33", 5)
                          foreach (t40, t41 in gen42())
                          {
                              if (!((9 <= 2)))
                              {
                                  to_log("34", -2)
                                }
                              to_log("35", -1)
                              if (!((-1 > 8)))
                              {
                                  to_log("36", -1)
                                  to_log("37", (((14 == 13)) ? (-3) : (12)))
                                  to_log("38", 2)
                                  to_log("39", 0)
                                  to_log("40", 3)
                                }
                              to_log("41", 13)
                              to_log("42", -3)
                            }
                          to_log("43", 4)
                          to_log("44", v37)
                          to_log("45", gen42)
                        }
                        foreach (t31, t32 in gen33())
                        {
                            to_log("25", p34)
                            to_log("26", 6)
                            to_log("27", ((11) | (-4)))
                          }
                        fn35()
                        to_log("46", gen33)
                        to_log("47", fn35)
                      }
                    for (local loop44 = 5; loop44 < 5; loop44++)
                    {
                        let t45 = { f0=-2 f1=14 }
                        let fni46 = @() 3
                        let a47 = [ -4, ]
                        let v48 = 13
                        to_log("48", 11)
                        to_log("49", 5)
                        to_log("50", (((t45.f1)) & (fni46())) - (((2 != 5793629195146322613)) ? (14) : (8)))
                        to_log("51", (((a47[0] == 9)) ? (((787389375239146782 - 12) | (v48))) : (10)))
                        to_log("52", t45)
                        to_log("53", fni46)
                        to_log("54", a47)
                        to_log("55", v48)
                      }
                    to_log("56", fni49())
                    to_log("57", fni49)
                  }
                to_log("58", fn13)
                to_log("59", v22)
                to_log("60", fni21)
                to_log("61", a25)
                to_log("62", v27)
                to_log("63", fni26)
                to_log("64", v29)
              }
              fn12(3, ((v50) & (v50)), (13))
              to_log("65", 1)
              to_log("66", v50)
              to_log("67", fn12)
            }
          to_log("68", v51)
        }
      to_log("69", gen11)
      to_log("70", t52)
      to_log("71", a53)
    }
  }
  {
    local loop77 = (((11 == (t54?.f0 ?? 12))) ? ((t55?["f0"] ?? 7)) : (((-1) & (((t56?["f1"] ?? ((10) & (-3))) + (a66?[0] ?? (t67?["f0"] ?? (((v65 < -4)) ? (-2) : (((13) | (6)))))) + fni68()))))) + (((v65 != v69)) ? ((((fni70() > (((-1 >= -2)) ? (0) : ((a71?[0] ?? v65))))) ? (-5) : (v72 - (a73?[1] ?? v75)))) : (-3))
    do
      to_log("72", 11)
    while (loop77++ < (t78?["f1"] ?? ((5 - (((8 <= (a79?[0] ?? 14))) ? (-5) : (12)) + (((8 <= ((((t80?.f0 ?? -4) <= (t81?["f0"] ?? -5))) ? (2) : (14)))) ? (10) : (6))) & (6))))
  }
  to_log("73", -1)
  {
    local loop83 = (((11 >= (((v82) + 10) & (-5)))) ? (-6760683509134506842) : (2))
    while (loop83++ < 11)
    {
      to_log("74", 0)
    }
  }
  to_log("75", t6)
  to_log("76", a7)
  to_log("77", t54)
  to_log("78", t55)
  to_log("79", a57)
  to_log("80", v58)
  to_log("81", v60)
  to_log("82", t59)
  to_log("83", a61)
  to_log("84", t62)
  to_log("85", a64)
  to_log("86", v65)
  to_log("87", a63)
  to_log("88", t56)
  to_log("89", a66)
  to_log("90", t67)
  to_log("91", fni68)
  to_log("92", v69)
  to_log("93", fni70)
  to_log("94", a71)
  to_log("95", v72)
  to_log("96", v74)
  to_log("97", a73)
  to_log("98", v76)
  to_log("99", v75)
  to_log("100", t78)
  to_log("101", a79)
  to_log("102", t80)
  to_log("103", t81)
  to_log("104", v82)
}} catch (q) {print(q)}

