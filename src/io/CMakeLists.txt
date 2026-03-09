add_library(ZyriIO STATIC
    IOManager.cpp
    ZyriFormat.cpp
    PNGHandler.cpp
    JPEGHandler.cpp
    PDFImporter.cpp
    PDFExporter.cpp
    ClipboardHandler.cpp
)

target_include_directories(ZyriIO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriIO PUBLIC
    ZyriCore
    Qt6::Core
    Qt6::Gui
    Qt6::PrintSupport
    Qt6::Pdf
    Qt6::Concurrent
    PkgConfig::LCMS2
    ZLIB::ZLIB
)

if(ZYRI_ENABLE_POPPLER AND POPPLER_FOUND)
    target_link_libraries(ZyriIO PRIVATE ${POPPLER_LIBRARIES})
    target_include_directories(ZyriIO PRIVATE ${POPPLER_INCLUDE_DIRS})
endif()
