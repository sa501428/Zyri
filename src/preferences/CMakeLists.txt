add_library(ZyriPreferences STATIC
    PreferencesStore.cpp
    PreferencesDialog.cpp
    KeyboardShortcuts.cpp
)

target_include_directories(ZyriPreferences PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ZyriPreferences PUBLIC
    ZyriCore
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
