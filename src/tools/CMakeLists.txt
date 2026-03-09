add_library(ZyriTools STATIC
    ToolBase.cpp
    ToolRegistry.cpp
    painting/PencilTool.cpp
    painting/BrushTool.cpp
    painting/EraserTool.cpp
    fill/FillBucketTool.cpp
    fill/EyedropperTool.cpp
    shapes/LineTool.cpp
    shapes/RectTool.cpp
    shapes/EllipseTool.cpp
    shapes/RoundedRectTool.cpp
    shapes/PolygonTool.cpp
    text/TextTool.cpp
    transform/MoveTool.cpp
    transform/CropTool.cpp
    transform/TransformTool.cpp
    selection/RectSelectTool.cpp
    selection/EllipseSelectTool.cpp
    selection/LassoTool.cpp
    selection/MagicWandTool.cpp
)

target_include_directories(ZyriTools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriTools PUBLIC
    ZyriCore
    ZyriEngine
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
