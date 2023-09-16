#include "daScript/misc/platform.h"

#include "unitTest.h"
#include "module_unitTest.h"

using namespace das;

DAS_BASE_BIND_ENUM(SomeEnum, SomeEnum, zero, one, two)

DAS_BASE_BIND_ENUM(Goo::GooEnum, GooEnum, regular, hazardous)
DAS_BASE_BIND_ENUM_98(Goo::GooEnum98, GooEnum98, soft, hard)

Goo::GooEnum efn_flip ( Goo::GooEnum goo ) {
    return (goo == Goo::GooEnum::regular) ? Goo::GooEnum::hazardous : Goo::GooEnum::regular;
}

SomeEnum efn_takeOne_giveTwo ( SomeEnum one ) {
    return (one == SomeEnum::one) ? SomeEnum::two : SomeEnum::zero;
}

DAS_BASE_BIND_ENUM_98(SomeEnum98, SomeEnum98, SomeEnum98_zero, SomeEnum98_one, SomeEnum98_two)

SomeEnum98 efn_takeOne_giveTwo_98 ( SomeEnum98 one ) {
    return (one == SomeEnum98_one) ? SomeEnum98_two : SomeEnum98_zero;
}

SomeEnum98_DasProxy efn_takeOne_giveTwo_98_DasProxy ( SomeEnum98_DasProxy two) {
    return (SomeEnum98_DasProxy) efn_takeOne_giveTwo_98( (SomeEnum98)two);
}

DAS_BASE_BIND_ENUM_98(SomeEnum_16, SomeEnum_16, SomeEnum_16_zero, SomeEnum_16_one, SomeEnum_16_two)

void Module_UnitTest::addEnumTest(ModuleLibrary &lib)
{
    // enum
    addEnumeration(make_smart<EnumerationGooEnum>());
    addEnumeration(make_smart<EnumerationSomeEnum>());
    addExtern<DAS_BIND_FUN(efn_takeOne_giveTwo)>(*this, lib, "free_takeOne_giveTwo", SideEffects::none, "efn_takeOne_giveTwo");
    addExtern<DAS_BIND_FUN(efn_takeOne_giveTwo)>(*this, lib, "efn_takeOne_giveTwo", SideEffects::modifyExternal, "efn_takeOne_giveTwo");
    addExtern<DAS_BIND_FUN(efn_flip)>(*this, lib, "efn_flip", SideEffects::modifyExternal, "efn_flip");
    // enum98
    addEnumeration(make_smart<EnumerationSomeEnum98>());
    addExtern<DAS_BIND_FUN(efn_takeOne_giveTwo_98_DasProxy)>(*this, lib, "efn_takeOne_giveTwo_98",
        SideEffects::modifyExternal, "efn_takeOne_giveTwo_98");
    // enum16
    addEnumeration(make_smart<EnumerationSomeEnum_16>());
};

