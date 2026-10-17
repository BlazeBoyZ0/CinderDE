# CinderDE v0.8

CinderDE (Cinder Development Environment) is a lightweight desktop code editor built with C++17 and Qt 6 Widgets. It is designed for Linux desktops where RAM and CPU resources are limited, while still providing a comfortable coding workflow.

CinderDE keeps its core editor lightweight and makes language intelligence optional: users can configure external Language Server Protocol (LSP) programs instead of having language servers bundled into the application.

## Features

### Editing and navigation
- Dark charcoal UI with ember-orange accents.
- Tabbed editing with unsaved-change indicators and prompts before discarding changes.
- Create, open, save, and Save As files.
- Line numbers, current-line highlighting, matching-bracket highlighting, smart indentation, bracket pairing, and configurable tab size.
- Find and replace, line/column status, font-size controls, and optional word wrap.
- File explorer for browsing a workspace and creating, renaming, or deleting files and folders.
- Session and workspace restoration, including the option to restore the previous workspace and tabs.
- Keyboard shortcuts for common editing actions, completion, go-to-definition, references, and rename.

### Syntax highlighting and built-in completion
- Built-in syntax highlighting for C, C++, and Python.
- Lightweight word completion based on language keywords, built-ins, and identifiers found in the current document.
- Built-in word completion remains available without an LSP.

### Optional LSP support
CinderDE v0.8 includes an optional LSP client. No language server is bundled or required to use CinderDE. Install a compatible server separately and configure its executable, arguments, language ID, and file extensions in CinderDE's LSP settings.

When a configured server supports the relevant capabilities, CinderDE can provide:
- Language-aware completion, including member completion after trigger characters such as `.`.
- Diagnostics and editor underlines.
- Hover information, including automatic hover after the mouse pauses over code.
- Go to definition.
- Find references.
- Rename symbol.

Support for individual operations depends on the configured language server and the capabilities it advertises. Automatic LSP completion is debounced while typing to avoid sending a request for every keystroke; trigger characters such as `.` request completion immediately with LSP trigger context. Hover information appears after the mouse pauses over code, and `Ctrl+Space` remains available to request completion manually.

Language servers run as external processes. Their memory use depends on the server and project. For the lightest experience, leave LSP disabled when language intelligence is not needed.

### Build and run
- Build/run actions with an integrated output panel.
- Optional external-terminal execution mode, where supported by the configured run workflow.

## Requirements

- C++17 compiler (GCC or Clang).
- CMake 3.16 or newer.
- Qt 6 development packages: Core, Gui, Widgets, and Test.

On Ubuntu or Linux Mint, install the common dependencies with:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

## Build

From the repository root:

```bash
cmake -B build -S .
cmake --build build
```

Launch the editor:

```bash
./build/cinderde
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

## Optional language server setup

1. Install a language server for the language you use. CinderDE does not download or bundle servers.
2. Open CinderDE's LSP configuration and add a server entry.
3. Set the server executable and any required command-line arguments. For stdio-based servers, use the server's documented stdio option.
4. Set the language ID and file extensions the server handles, then enable the entry.
5. Open a matching source file in a workspace and check the LSP status/log panel.

For example, Pyright can be configured as an external server for Python files with language ID `python`, extension `py`, and the executable/arguments recommended by Pyright's installation instructions.

## Scope

CinderDE is a lightweight editor and extensible coding environment, not a full replacement for every feature of a heavyweight IDE. It does not bundle language servers or a debugger. Language intelligence and its resource usage depend on which optional external servers the user chooses to install and run.
