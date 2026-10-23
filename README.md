# CinderDE

**CinderDE (Cinder Development Environment)** is a lightweight desktop code editor built with C++17 and Qt 6 Widgets. It focuses on a practical editing and build/run workflow with a low-overhead approach, customizable language support, and optional external development tools.

## At a glance

- Lightweight desktop editor with tabs, file explorer, workspace/session restoration, and common editing tools.
- Configurable build and run commands.
- Optional language intelligence through external Language Server Protocol (LSP) servers.
- User-installed JSON language packs for language-specific editor metadata.
- No bundled language definition packs or language servers.

CinderDE may suit developers who want a straightforward editor and prefer choosing their own language tools. External servers and toolchains must be installed separately.

## Documentation

- **[Project overview](guide/info.md)** — features, goals, and intended audience.
- **[Installation and build](guide/installation.md)** — dependencies, building, launching, and tests.
- **[Language Server Protocol (LSP)](guide/lsp.md)** — configure external language servers.
- **[Language packs](guide/language-pack.md)** — create and install user-defined JSON language packs.
- **[Limitations and scope](guide/limitations.md)** — current boundaries and dependencies.

## Project details

- **Language:** C++17
- **GUI framework:** Qt 6 Widgets
- **Build system:** CMake
- **Version:** 1.0.0 (finalization in progress)

For detailed instructions and caveats, start with the guides above.
