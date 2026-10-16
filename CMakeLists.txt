cmake_minimum_required(VERSION 3.16)
project(CinderDE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/editor_tab.cpp
    src/file_explorer.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
)

set(HEADERS
    include/cinderde/main_window.h
    include/cinderde/editor_tab.h
    include/cinderde/file_explorer.h
    include/cinderde/file_type_detector.h
    include/cinderde/preferences.h
)

add_executable(cinderde ${SOURCES} ${HEADERS})

target_link_libraries(cinderde PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

enable_testing()

add_executable(cinderde_tests
    tests/test_file_type_detector.cpp
    src/file_type_detector.cpp
)

target_link_libraries(cinderde_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME FileTypeDetectorTest COMMAND cinderde_tests)

add_executable(editor_tab_tests
    tests/test_editor_tab.cpp
    src/editor_tab.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    include/cinderde/editor_tab.h
)

target_link_libraries(editor_tab_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME EditorTabTest COMMAND editor_tab_tests -platform offscreen)
