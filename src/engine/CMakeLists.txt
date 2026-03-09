add_library(ZyriEngine STATIC
    raster/PaintEngine.cpp
    raster/BrushStamp.cpp
    raster/FillEngine.cpp
    raster/SelectionMask.cpp
    raster/TransformEngine.cpp
    vector/VectorScene.cpp
    vector/VectorObject.cpp
    vector/PathEditor.cpp
    vector/VectorRasterizer.cpp
)

target_include_directories(ZyriEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriEngine PUBLIC
    ZyriCore
    Qt6::Core
    Qt6::Gui
)
