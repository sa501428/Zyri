add_library(ZyriUI STATIC
    MainWindow.cpp
    DocumentTab.cpp
    toolbar/TopToolBar.cpp
    toolbar/ToolOptionsBar.cpp
    toolpanel/ToolPanel.cpp
    panels/LayerPanel.cpp
    panels/LayerItemDelegate.cpp
    panels/ColorPanel.cpp
    panels/ColorWheel.cpp
    panels/SwatchGrid.cpp
    panels/PropertiesPanel.cpp
    dialogs/NewDocumentDialog.cpp
    dialogs/ResizeCanvasDialog.cpp
    dialogs/ResizeImageDialog.cpp
    dialogs/ExportDialog.cpp
    dialogs/TextEditDialog.cpp
    dialogs/CrashRecoveryDialog.cpp
    statusbar/StatusBar.cpp
    theme/ThemeManager.cpp
)

target_include_directories(ZyriUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriUI PUBLIC
    ZyriCore
    ZyriCanvas
    ZyriTools
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
