add_library(ZyriHistory STATIC
    UndoStack.cpp
    DeltaCompressor.cpp
    commands/PaintTileCommand.cpp
    commands/AddLayerCommand.cpp
    commands/DeleteLayerCommand.cpp
    commands/ReorderLayerCommand.cpp
    commands/LayerPropertyCommand.cpp
    commands/VectorObjectCommand.cpp
    commands/TextCommand.cpp
    commands/TransformCommand.cpp
    commands/DocumentPropertyCommand.cpp
)

target_include_directories(ZyriHistory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriHistory PUBLIC
    ZyriCore
    Qt6::Core
    ZLIB::ZLIB
)
