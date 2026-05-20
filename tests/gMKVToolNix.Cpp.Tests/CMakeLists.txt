find_package(Qt6 6.2 REQUIRED COMPONENTS Test)

qt_add_executable(gMKVToolNixCppTests
    tst_Core.cpp
)

target_link_libraries(gMKVToolNixCppTests
    PRIVATE
        Qt6::Test
        gMKVToolNixCpp
)

add_test(NAME gMKVToolNixCppTests COMMAND gMKVToolNixCppTests)
