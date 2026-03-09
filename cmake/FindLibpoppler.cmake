# FindLibpoppler.cmake
# Wraps pkg-config detection of poppler-qt6 into an imported target.
# Provides: Poppler::Qt6

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(POPPLER_QT6 QUIET poppler-qt6)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libpoppler
    REQUIRED_VARS POPPLER_QT6_LIBRARIES POPPLER_QT6_INCLUDE_DIRS
    VERSION_VAR POPPLER_QT6_VERSION
)

if(Libpoppler_FOUND AND NOT TARGET Poppler::Qt6)
    add_library(Poppler::Qt6 INTERFACE IMPORTED)
    target_include_directories(Poppler::Qt6 INTERFACE ${POPPLER_QT6_INCLUDE_DIRS})
    target_link_libraries(Poppler::Qt6 INTERFACE ${POPPLER_QT6_LIBRARIES})
    target_compile_options(Poppler::Qt6 INTERFACE ${POPPLER_QT6_CFLAGS_OTHER})
endif()
