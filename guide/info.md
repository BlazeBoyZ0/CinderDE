# CinderDE — Project Information

## What is CinderDE?

CinderDE (Cinder Development Environment) is a lightweight desktop code editor built with C++17 and Qt 6 Widgets. It is designed for developers who want a practical, customizable coding environment without the overhead of a large IDE.

CinderDE focuses on editing, workspace navigation, build/run workflows, and optional external language tooling. Its interface uses a dark charcoal theme with ember-orange accents.

## What it offers

- Tabbed editor with unsaved-change protection, safe file saving, line numbers, bracket matching, indentation, find/replace, and configurable editor preferences.
- Workspace file explorer and workspace/session restoration.
- Basic word completion and syntax highlighting that can be extended with user-installed language packs.
- Optional external Language Server Protocol (LSP) servers for capabilities such as completion, diagnostics, hover, definitions, references, and rename when supported by the configured server.
- Configurable build/run commands, workspace-specific settings, output/diagnostics, and optional external-terminal execution.
- Optional external debugging through the debugger adapter configuration supported by the application.

## Language tooling philosophy

CinderDE does not bundle language servers or ship built-in language definition packs. Users choose and install the tools they want. See [Language servers](lsp.md) and [Language packs](language-pack.md).

## Who is it for?

CinderDE may suit developers who prefer a small desktop editor, want to configure their own toolchain, or work on machines where resource use matters. It is not intended to reproduce every feature of a full-scale IDE.

## More documentation

- [Installation and build instructions](installation.md)
- [Language Server Protocol setup](lsp.md)
- [User language-pack format](language-pack.md)
- [Known limitations and scope](limitations.md)
