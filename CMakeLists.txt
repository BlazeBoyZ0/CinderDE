cmake_minimum_required(VERSION 3.16)
project(CinderDE VERSION 0.4.0 LANGUAGES CXX)

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
    src/syntax_highlighter.cpp
    src/workspace_config.cpp
)

set(HEADERS
    include/cinderde/main_window.h
    include/cinderde/editor_tab.h
    include/cinderde/file_explorer.h
    include/cinderde/file_type_detector.h
    include/cinderde/preferences.h
    include/cinderde/syntax_theme.h
    include/cinderde/syntax_highlighter.h
    include/cinderde/workspace_config.h
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
    src/syntax_highlighter.cpp
    include/cinderde/editor_tab.h
    include/cinderde/syntax_highlighter.h
)

target_link_libraries(editor_tab_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME EditorTabTest COMMAND editor_tab_tests -platform offscreen)

add_executable(syntax_highlighter_tests
    tests/test_syntax_highlighter.cpp
    src/syntax_highlighter.cpp
    src/editor_tab.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    include/cinderde/editor_tab.h
    include/cinderde/syntax_highlighter.h
)

target_link_libraries(syntax_highlighter_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME SyntaxHighlighterTest COMMAND syntax_highlighter_tests -platform offscreen)

add_executable(file_explorer_tests
    tests/test_file_explorer.cpp
    src/file_explorer.cpp
    src/preferences.cpp
    include/cinderde/file_explorer.h
)

target_link_libraries(file_explorer_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME FileExplorerTest COMMAND file_explorer_tests -platform offscreen)

add_executable(workspace_session_tests
    tests/test_workspace_session.cpp
    src/preferences.cpp
    src/workspace_config.cpp
    src/editor_tab.cpp
    src/file_type_detector.cpp
    src/syntax_highlighter.cpp
    include/cinderde/preferences.h
    include/cinderde/workspace_config.h
    include/cinderde/editor_tab.h
    include/cinderde/syntax_highlighter.h
)

target_link_libraries(workspace_session_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME WorkspaceSessionTest COMMAND workspace_session_tests -platform offscreen)
