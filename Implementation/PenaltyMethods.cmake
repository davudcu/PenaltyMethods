set(PM_NAME PenaltyMethods)
set(PM_TESTS_NAME PenaltyMethodsTests)

file(GLOB PM_SOURCES   ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB PM_INCS      ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
file(GLOB PM_CORE_INCS ${CMAKE_CURRENT_LIST_DIR}/src/core/*.h)
file(GLOB PM_UI_INCS   ${CMAKE_CURRENT_LIST_DIR}/src/ui/*.h)
set(PM_PLIST           ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/AppIcon.plist)

file(GLOB PM_INC_TD    ${NATID_SDK_INC}/td/*.h)
file(GLOB PM_INC_GUI   ${NATID_SDK_INC}/gui/*.h)
file(GLOB PM_INC_DENSE ${NATID_SDK_INC}/dense/*.h)
file(GLOB PM_INC_DP    ${NATID_SDK_INC}/dp/*.h)

if(WIN32)
    set(PM_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
else()
    set(PM_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
endif()

# ------------------------------------------------------------
# GUI application
# ------------------------------------------------------------
add_executable(${PM_NAME}
    ${PM_INCS}
    ${PM_SOURCES}
    ${PM_CORE_INCS}
    ${PM_UI_INCS}
    ${PM_INC_TD}
    ${PM_INC_GUI}
    ${PM_INC_DENSE}
    ${PM_INC_DP}
    ${PM_WINAPP_ICON})

source_group("inc"        FILES ${PM_INCS})
source_group("core"       FILES ${PM_CORE_INCS})
source_group("ui"         FILES ${PM_UI_INCS})
source_group("inc\\td"    FILES ${PM_INC_TD})
source_group("inc\\gui"   FILES ${PM_INC_GUI})
source_group("inc\\dense" FILES ${PM_INC_DENSE})
source_group("inc\\dp"    FILES ${PM_INC_DP})
source_group("src"        FILES ${PM_SOURCES})

target_link_libraries(${PM_NAME}
    debug   ${MU_LIB_DEBUG}     optimized ${MU_LIB_RELEASE}
    debug   ${NATGUI_LIB_DEBUG} optimized ${NATGUI_LIB_RELEASE}
    debug   ${MATRIX_LIB_DEBUG} optimized ${MATRIX_LIB_RELEASE}
    debug   ${DP_LIB_DEBUG}     optimized ${DP_LIB_RELEASE})

setTargetPropertiesForGUIApp(${PM_NAME} ${PM_PLIST})
setAppIcon(${PM_NAME} ${CMAKE_CURRENT_LIST_DIR})
setIDEPropertiesForGUIExecutable(${PM_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${PM_NAME})

# ------------------------------------------------------------
# Console verification of the numerical core
# (finite-difference derivative checks, closed-form solutions,
#  conditioning trend)
# ------------------------------------------------------------
file(GLOB PM_TEST_SOURCES ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)

add_executable(${PM_TESTS_NAME} ${PM_TEST_SOURCES} ${PM_CORE_INCS})
source_group("tests" FILES ${PM_TEST_SOURCES})
source_group("core"  FILES ${PM_CORE_INCS})

target_link_libraries(${PM_TESTS_NAME}
    debug   ${MU_LIB_DEBUG}     optimized ${MU_LIB_RELEASE}
    debug   ${MATRIX_LIB_DEBUG} optimized ${MATRIX_LIB_RELEASE})

setIDEPropertiesForExecutable(${PM_TESTS_NAME})

# F5 in Visual Studio starts the GUI, not the test runner
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PM_NAME})
