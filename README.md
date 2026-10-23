# CinderDE v1.0.0

CinderDE (Cinder Development Environment) is a lightweight desktop code editor built with C++17 and Qt 6 Widgets. It targets Linux desktops with limited RAM while providing a practical coding workflow.

CinderDE keeps language intelligence optional: users install and configure external Language Server Protocol (LSP) programs instead of having language servers bundled into the application.

## Features

### Editing and navigation
- Dark charcoal UI with ember-orange accents.
- Tabbed editing, unsaved-change indicators, and prompts before discarding changes.
- Create, open, save, and Save As files, with safer file-write handling.
- Line numbers, current-line highlighting, matching-bracket highlighting, smart indentation, bracket pairing, and configurable tab size.
- Find and replace, line/column status, font-size controls, and optional word wrap.
- Workspace file explorer for browsing, creating, renaming, and deleting files and folders.
- Session and workspace restoration, including an option to restore the previous workspace and tabs.
- Keyboard shortcuts for common editing actions and navigation.

### Syntax highlighting and completion
- Optional syntax highlighting driven by user-installed language definition packs; no language packs are bundled.
- Lightweight word completion can use keywords and built-ins from user-installed language definition packs, plus identifiers in the current document.
- Built-in word completion remains available without an LSP.

### Optional Language Server Protocol (LSP)
- Configure external language servers with executable, arguments, a manually entered language ID, file extensions, and environment settings.
- Optional language-aware completion, diagnostics, hover information, go to definition, find references, and rename, depending on server capabilities.
- Debounced completion requests to reduce unnecessary background traffic.
- Server status and logs to help diagnose startup and runtime problems.
- LSP can remain disabled for a lighter memory footprint. Resource usage depends on the external server and workspace.

CinderDE does not bundle language servers. Install compatible servers separately and configure them in the LSP settings.

### Build and run
- Configurable build and run commands, with macro substitution for file, workspace, and working-directory paths.
- Build/run output panel with diagnostics navigation and process controls.
- Optional external-terminal execution where supported by the configured workflow.
- Save failures prevent build/run actions from proceeding with unsaved editor content.
- Workspace build/run settings are persisted, with safer handling of configuration-file writes.

## Requirements

- C++17 compiler (GCC or Clang).
- CMake 3.16 or newer.
- Qt 6 development packages: Core, Gui, Widgets, and Test.

On Ubuntu or Linux Mint:

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

Launch CinderDE:

```bash
./build/cinderde
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

## Optional language server setup

1. Install a compatible language server for your language. CinderDE does not download or bundle servers.
2. Open CinderDE's Language Servers settings.
3. Configure the executable and any required arguments using the server's documented invocation.
4. Set the language ID and file extensions the server handles, then enable the configuration.
5. Open a matching source file in a workspace and check the LSP status/log panel.

Language IDs and file extensions are entered manually to match the external server's own documentation. CinderDE does not provide a predefined language list.

## Scope

CinderDE is a lightweight, extensible coding environment, not a full replacement for every feature of a heavyweight IDE. It prioritizes low overhead and lets users choose which optional external language tools to install and run. Language intelligence and its resource usage depend on those external tools.

## Release status

Version 1.0.0 is being finalized. CinderDE ships without built-in language support packs; users may add their own JSON language definitions and configure external language servers. The repository's build and test commands above are provided for release verification; a successful build or test run should not be assumed until those commands have been executed successfully on the target environment.
