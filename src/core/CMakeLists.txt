add_library(ZyriCore STATIC
    Document.cpp
    DocumentController.cpp
    Layer.cpp
    RasterLayer.cpp
    VectorLayer.cpp
    TextLayer.cpp
    LayerStack.cpp
    BlendMode.cpp
    ColorProfile.cpp
)

target_include_directories(ZyriCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriCore PUBLIC
    Qt6::Core
    Qt6::Gui
    PkgConfig::LCMS2
    ZLIB::ZLIB
)
