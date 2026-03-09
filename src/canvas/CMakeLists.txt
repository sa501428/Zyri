add_library(ZyriCanvas STATIC
    CanvasViewport.cpp
    TileCache.cpp
    TileRenderer.cpp
    GLCompositor.cpp
    ViewTransform.cpp
    CheckerboardBackground.cpp
    CanvasOverlay.cpp
)

target_include_directories(ZyriCanvas PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriCanvas PUBLIC
    ZyriCore
    ZyriEngine
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Embed GLSL shaders as Qt resources
target_sources(ZyriCanvas PRIVATE shaders.qrc)
