# CinderDE v0.1

CinderDE (Cinder Development Environment) is a lightweight desktop text and source-code editor built with C++ and Qt 6, designed for low-end Linux systems (such as Linux Mint XFCE with limited RAM and older CPUs). Inspired by the core editing capabilities of modern editors, CinderDE provides a fast, responsive, and comfortable code editing experience without heavyweight IDE bloat.

## Features

- **Visual Design**: Dark charcoal theme (`#1e1e1e`/`#252526`) with ember-orange accents (`#ff6b35`).
- **Tabbed Interface**: Manage multiple documents simultaneously with tab switching, unsaved change indicators, and close controls.
- **Line Numbers & Line Highlighting**: Custom sidebar displaying line numbers alongside active cursor line highlighting.
- **File Explorer Sidebar**: Browse directory structure, expand/collapse folders, create files/directories, rename, and delete items with confirmation dialogs.
- **Find and Replace Bar**: Embedded non-modal find and replace tool supporting case matching and match counts.
- **File-Type Recognition**: Extension-based file type classification (C++, C, Python, Headers, CMake, Markdown, JSON, XML, HTML, CSS, JavaScript, etc.) displayed in the status bar.
- **Binary Content Protection**: Detects likely binary files before opening and warns the user to avoid corrupting text or displaying unprintable characters.
- **Persistence & Preferences**: Uses `QSettings` to save and restore font size, word-wrap preference, explorer panel visibility, window geometry, and last workspace directory.
- **Keyboard Shortcuts**: Standard desktop editor shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Z, Ctrl+Y, Ctrl+F, Ctrl+H, Ctrl+B, Ctrl+Shift+O).

## Dependencies

- **C++ Compiler**: GCC or Clang supporting C++17.
- **Build System**: CMake 3.16 or newer.
- **GUI Framework**: Qt 6 (Components: `Widgets`, `Core`, `Gui`, `Test`).

### On Ubuntu / Linux Mint / Debian:

```bash
sudo apt-get update
sudo apt-get install -y qt6-base-dev qt6-tools-dev cmake g++
```

## How to Build

1. Clone or navigate to the CinderDE repository root.
2. Create a build directory and configure CMake:

```bash
mkdir -p build && cd build
cmake ..
```

3. Build the project:

```bash
make -j$(nproc)
```

## How to Launch

After building, run the application executable from the `build` directory:

```bash
./cinderde
```

To run unit tests:

```bash
ctest --output-on-failure
```

## Basic Usage

- **File Operations**: Use the **File** menu or Toolbar to create new files, open existing files, open a workspace directory, or save documents.
- **Editing**: Standard undo, redo, cut, copy, paste, select all, find, and replace functions are accessible via the **Edit** menu or standard keyboard shortcuts.
- **Sidebar**: Toggle the File Explorer sidebar via **View -> Toggle Explorer** or `Ctrl+B`. Right-click within the file explorer tree to create, rename, or delete files and folders.
- **Font Size & Word Wrap**: Adjust font size (`Ctrl++`, `Ctrl+-`, `Ctrl+0`) or toggle word wrap (`Alt+Z`) from the **View** menu.

## Scope & Limitations (v0.1)

CinderDE v0.1 is strictly a lightweight code editor and intentionally excludes heavyweight IDE features:
- No syntax highlighting (planned for future lightweight implementation).
- No build/run buttons or integrated terminal.
- No language servers, autocomplete, or debugger integration.
