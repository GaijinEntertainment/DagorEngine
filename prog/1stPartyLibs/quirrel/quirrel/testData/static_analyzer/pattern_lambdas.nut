local x = 10
let _y = x > 320 ? @() 20 : function() { return 30 }
