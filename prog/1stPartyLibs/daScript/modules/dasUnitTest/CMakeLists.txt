
IF ((NOT DAS_UNIT_TEST_INCLUDED) AND ((NOT ${DAS_UNIT_TEST_DISABLED}) OR (NOT DEFINED DAS_UNIT_TEST_DISABLED)))
    SET(DAS_UNIT_TEST_INCLUDED TRUE)
    MESSAGE(STATUS "dasUnitTest module included.")

	SET(DAS_UNIT_TEST_DIR ${PROJECT_SOURCE_DIR}/modules/dasUnitTest)

	CMAKE_TEXT_XXD(libDasModuleUnitTest modules/dasUnitTest/unit_test.das)

	SET(DAS_UNIT_TEST_MODULE_SRC
		${DAS_UNIT_TEST_DIR}/bytecode.cpp
		${DAS_UNIT_TEST_DIR}/bytecode.h
		${DAS_UNIT_TEST_DIR}/test_handles.cpp
		${DAS_UNIT_TEST_DIR}/test_enum.cpp
		${DAS_UNIT_TEST_DIR}/module_unitTest.h
		${DAS_UNIT_TEST_DIR}/unitTest.h
		${DAS_UNIT_TEST_DIR}/unit_test.das.inc
		${DAS_UNIT_TEST_DIR}/unit_test.das
	)
	SOURCE_GROUP_FILES("source" DAS_UNIT_TEST_MODULE_SRC)

	ADD_MODULE_CPP(UnitTest)
	ADD_MODULE_LIB(libDasModuleUnitTest dasModuleUnitTest ${DAS_UNIT_TEST_MODULE_SRC})
	SETUP_CPP11(libDasModuleUnitTest)
	ADD_PROJECT_XXD_DEPENDS(libDasModuleUnitTest)

ENDIF()
