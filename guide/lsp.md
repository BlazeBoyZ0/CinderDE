# Language Server Protocol (LSP)

CinderDE can launch external Language Server Protocol servers. Servers are optional and are not bundled with CinderDE.

## Before you begin

1. Install a language server that supports your language and platform.
2. Read that server's official documentation for its executable, startup arguments, language ID, and supported capabilities.
3. Make sure the executable can be launched in your environment.

CinderDE does not download, install, or manage language-server packages for you.

## Add a server

Open CinderDE's Language Servers (LSP) panel and choose **Add**. Enter the following fields:

- **Display Name:** A label you choose to identify this server in CinderDE.
- **Language ID:** The language identifier expected by the server. Type it manually; CinderDE does not offer a predefined language-name list.
- **File Extensions:** Comma-separated extensions used to match files to this server. You may type extensions with or without a leading dot.
- **Executable:** An executable name available on `PATH`, or a path to an executable file.
- **Arguments:** Startup arguments required by the server, if any.
- **Environment:** Optional environment entries written as `KEY=VALUE`, separated by semicolons.
- **Enable server:** Controls whether the configuration is enabled.

Save the configuration, open a matching file, and check the LSP panel's status and logs. You can edit, enable/disable, start, stop, or remove configurations from the panel.

## Available capabilities

The server determines which features it supports. Depending on the server and configuration, CinderDE may provide:

- Completion
- Diagnostics
- Hover information
- Go to definition
- Find references
- Rename

Not every server supports every feature.

## Resource use and troubleshooting

LSP servers run as separate processes and consume their own memory and CPU. CinderDE can be used without starting a language server; enabling a server may increase total resource usage.

If a server fails to start:

1. Confirm the executable exists and is executable.
2. Check the arguments against the server's official instructions.
3. Confirm the manually entered language ID and file extensions match the server's expectations.
4. Inspect the LSP status and log panel for errors.
5. Try launching the server from a terminal to identify missing dependencies or environment requirements.

See [Limitations](limitations.md) for the current scope of CinderDE's LSP integration.
