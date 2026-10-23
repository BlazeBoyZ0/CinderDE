# Limitations and Scope

CinderDE is a lightweight, extensible coding environment. It is not a full replacement for every feature found in larger IDEs.

## Current limitations

- **Platform focus:** The documented build workflow targets Linux. Other platforms may require additional setup and have not been established by this guide.
- **No bundled language packs:** CinderDE ships without built-in language definition JSON packs. Language-specific highlighting and pack-provided completion vocabulary require a user-installed pack.
- **No bundled language servers:** LSP servers must be installed and configured separately. Feature availability depends on each server's implementation and configuration.
- **External tools use resources:** Language servers, compilers, debuggers, and terminal applications run outside or alongside the editor and may add memory and CPU usage.
- **Manual configuration:** Users are responsible for entering the correct LSP language ID, extensions, executable, arguments, and environment for their selected server.
- **Build/run behavior varies:** Default commands depend on the file type and available tools. For less common languages or custom workflows, configure build/run commands or provide a language pack with defaults.
- **Not every IDE feature is included:** Advanced refactoring, project-system integration, extension marketplaces, and other features of large IDEs are outside the current scope unless explicitly implemented in CinderDE.
- **Hardware and workspace size matter:** Performance depends on the machine, open files, workspace, and optional tools running alongside CinderDE.

## What CinderDE aims to do

CinderDE prioritizes a straightforward editor, workspace navigation, configurable build/run workflows, and optional external language tooling. It aims to let users choose their own toolchain rather than bundling a large set of language-specific dependencies.

These limitations describe the intended current scope, not a promise that every configuration has been tested on every platform.
