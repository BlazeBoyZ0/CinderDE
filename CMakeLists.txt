cmake_minimum_required(VERSION 3.16)
project(CinderDE VERSION 0.6.0 LANGUAGES CXX)

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
    src/code_completion.cpp
    src/file_explorer.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    src/syntax_highlighter.cpp
    src/workspace_config.cpp
    src/diagnostic_parser.cpp
    src/output_panel.cpp
    src/build_run_manager.cpp
    src/build_run_dialog.cpp
)

set(HEADERS
    include/cinderde/main_window.h
    include/cinderde/editor_tab.h
    include/cinderde/code_completion.h
    include/cinderde/file_explorer.h
    include/cinderde/file_type_detector.h
    include/cinderde/preferences.h
    include/cinderde/syntax_theme.h
    include/cinderde/syntax_highlighter.h
    include/cinderde/workspace_config.h
    include/cinderde/diagnostic_parser.h
    include/cinderde/output_panel.h
    include/cinderde/build_run_manager.h
    include/cinderde/build_run_dialog.h
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
    src/code_completion.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    src/syntax_highlighter.cpp
    include/cinderde/editor_tab.h
    include/cinderde/code_completion.h
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
    src/code_completion.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    include/cinderde/editor_tab.h
    include/cinderde/code_completion.h
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
    src/code_completion.cpp
    src/file_type_detector.cpp
    src/syntax_highlighter.cpp
    include/cinderde/preferences.h
    include/cinderde/workspace_config.h
    include/cinderde/editor_tab.h
    include/cinderde/code_completion.h
    include/cinderde/syntax_highlighter.h
)

target_link_libraries(workspace_session_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME WorkspaceSessionTest COMMAND workspace_session_tests -platform offscreen)

add_executable(diagnostic_parser_tests
    tests/test_diagnostic_parser.cpp
    src/diagnostic_parser.cpp
)

target_link_libraries(diagnostic_parser_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME DiagnosticParserTest COMMAND diagnostic_parser_tests)

add_executable(build_run_manager_tests
    tests/test_build_run_manager.cpp
    src/build_run_manager.cpp
    src/preferences.cpp
    src/workspace_config.cpp
    include/cinderde/build_run_manager.h
)

target_link_libraries(build_run_manager_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME BuildRunManagerTest COMMAND build_run_manager_tests)

add_executable(output_panel_tests
    tests/test_output_panel.cpp
    src/output_panel.cpp
    src/diagnostic_parser.cpp
    include/cinderde/output_panel.h
)

target_link_libraries(output_panel_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME OutputPanelTest COMMAND output_panel_tests -platform offscreen)

add_executable(v05_features_tests
    tests/test_v05_features.cpp
    src/build_run_manager.cpp
    src/preferences.cpp
    src/workspace_config.cpp
    include/cinderde/build_run_manager.h
)

target_link_libraries(v05_features_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME V05FeaturesTest COMMAND v05_features_tests)

add_executable(code_completion_tests
    tests/test_code_completion.cpp
    src/editor_tab.cpp
    src/code_completion.cpp
    src/file_type_detector.cpp
    src/preferences.cpp
    src/syntax_highlighter.cpp
    include/cinderde/editor_tab.h
    include/cinderde/code_completion.h
    include/cinderde/syntax_highlighter.h
)

target_link_libraries(code_completion_tests PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME CodeCompletionTest COMMAND code_completion_tests -platform offscreen)
