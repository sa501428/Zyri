# FindLittleCMS.cmake
# Wraps pkg-config detection of lcms2 into an imported target.
# Provides: LittleCMS::LittleCMS

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LCMS2 QUIET lcms2)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LittleCMS
    REQUIRED_VARS LCMS2_LIBRARIES LCMS2_INCLUDE_DIRS
    VERSION_VAR LCMS2_VERSION
)

if(LittleCMS_FOUND AND NOT TARGET LittleCMS::LittleCMS)
    add_library(LittleCMS::LittleCMS INTERFACE IMPORTED)
    target_include_directories(LittleCMS::LittleCMS INTERFACE ${LCMS2_INCLUDE_DIRS})
    target_link_libraries(LittleCMS::LittleCMS INTERFACE ${LCMS2_LIBRARIES})
    target_compile_options(LittleCMS::LittleCMS INTERFACE ${LCMS2_CFLAGS_OTHER})
endif()
