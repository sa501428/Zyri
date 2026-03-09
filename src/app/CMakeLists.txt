add_library(ZyriApp STATIC
    Application.cpp
    AppController.cpp
    SessionManager.cpp
)

target_include_directories(ZyriApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriApp PUBLIC
    ZyriCore
    ZyriUI
    ZyriIO
    ZyriHistory
    ZyriTools
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)
