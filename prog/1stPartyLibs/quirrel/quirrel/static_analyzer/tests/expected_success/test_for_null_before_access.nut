::x <- require("sq3_sa_test").x


local skinDecorator = ::x?.y
local canScaleAndRotate = skinDecorator == null || skinDecorator.getCouponItemdefId() == null
return canScaleAndRotate