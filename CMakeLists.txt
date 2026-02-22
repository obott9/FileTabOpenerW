cmake_minimum_required(VERSION 3.20)
project(FileTabOpenerW LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FileTabOpenerW WIN32
    src/main.cpp
    src/app.cpp
    src/config.cpp
    src/i18n.cpp
    src/logger.cpp
    src/utils.cpp
    src/explorer_opener.cpp
    src/main_window.cpp
    src/history_section.cpp
    src/tab_group_section.cpp
    src/tab_view.cpp
    src/input_dialog.cpp
    res/app.rc
)

target_include_directories(FileTabOpenerW PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(FileTabOpenerW PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(FileTabOpenerW PRIVATE
    comctl32
    shlwapi
    ole32
    oleaut32
    uuid
    uiautomationcore
    comdlg32
    shell32
)

# MSVC specific settings
if(MSVC)
    target_compile_options(FileTabOpenerW PRIVATE /W4 /utf-8)
    # Link the manifest
    set_target_properties(FileTabOpenerW PROPERTIES
        LINK_FLAGS "/MANIFEST:NO"
    )
endif()
