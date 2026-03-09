cmake_minimum_required(VERSION 3.25)

project(Zyri VERSION 0.1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(ZyriVersion)
include(CompilerOptions)

# ── Qt 6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
    Pdf
    PrintSupport
    Concurrent
    Test
)
qt_standard_project_setup()

# ── Third-party ───────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)

# littlecms — ICC color profiles
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)

# poppler-qt6 — PDF import (optional; Phase 6 feature)
# Homebrew's poppler formula does not ship poppler-qt6.pc on all configurations,
# so we make this optional and fall back to Qt6::Pdf for rasterized PDF rendering.
option(ZYRI_ENABLE_POPPLER "Use poppler-qt6 for enhanced PDF import" OFF)
if(ZYRI_ENABLE_POPPLER)
    pkg_check_modules(POPPLER REQUIRED poppler-qt6)
    if(POPPLER_FOUND)
        message(STATUS "poppler-qt6 found — enhanced PDF import enabled")
        add_compile_definitions(ZYRI_HAS_POPPLER)
    endif()
else()
    message(STATUS "poppler-qt6 disabled — using Qt6::Pdf for PDF rendering")
endif()

# zlib — tile delta compression (bundled with Qt on most platforms)
find_package(ZLIB REQUIRED)

# ── Submodules ────────────────────────────────────────────────────────────────
add_subdirectory(src/core)
add_subdirectory(src/canvas)
add_subdirectory(src/engine)
add_subdirectory(src/tools)
add_subdirectory(src/history)
add_subdirectory(src/io)
add_subdirectory(src/preferences)
add_subdirectory(src/ui)
add_subdirectory(src/app)
add_subdirectory(tests)

# ── Main executable ───────────────────────────────────────────────────────────
if(APPLE)
    set(MACOSX_SOURCES src/platform/macos/BookmarkManager.mm src/platform/macos/MacOSAppearance.mm)
else()
    set(MACOSX_SOURCES "")
endif()

add_executable(Zyri
    src/main.cpp
    ${MACOSX_SOURCES}
    resources/resources.qrc
)

target_link_libraries(Zyri PRIVATE
    ZyriApp
    ZyriCore
    ZyriCanvas
    ZyriEngine
    ZyriTools
    ZyriHistory
    ZyriIO
    ZyriPreferences
    ZyriUI
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Concurrent
)

# ── macOS bundle ──────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(Zyri PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Zyri"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zyri.app"
        MACOSX_BUNDLE_ICON_FILE "icon"
    )
    # Copy icon.icns into the bundle's Resources folder
    set(APP_ICON "${CMAKE_SOURCE_DIR}/packaging/macos/icon.icns")
    set_source_files_properties("${APP_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(Zyri PRIVATE "${APP_ICON}")
    target_link_libraries(Zyri PRIVATE "-framework AppKit" "-framework Foundation")
endif()

# ── Install / packaging ───────────────────────────────────────────────────────
include(Packaging)
