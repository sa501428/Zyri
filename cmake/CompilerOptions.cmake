add_library(ZyriCompilerOptions INTERFACE)

target_compile_features(ZyriCompilerOptions INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(ZyriCompilerOptions INTERFACE
        /W4
        /WX
        /utf-8
        /permissive-
    )
else()
    target_compile_options(ZyriCompilerOptions INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wno-unused-parameter
    )
endif()

# AddressSanitizer in Debug builds (non-MSVC)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    target_compile_options(ZyriCompilerOptions INTERFACE -fsanitize=address,undefined)
    target_link_options(ZyriCompilerOptions INTERFACE -fsanitize=address,undefined)
endif()
