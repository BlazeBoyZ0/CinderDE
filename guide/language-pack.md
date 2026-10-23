# User Language Packs

CinderDE v1.0.0 ships with no built-in language definition packs. Optional JSON packs can be installed by the user to supply language-specific syntax highlighting, word-completion vocabulary, comment tokens, indentation settings, and default build/run commands.

## Pack location

Place user language-pack JSON files in:

```text
~/.config/cinderde/languages/
```

CinderDE does not load embedded language-pack resources or ship example packs.

## JSON format

A language pack is a JSON object. The following is a generic template; replace the placeholder values with settings for your own language.

```json
{
  "id": "your-language-id",
  "name": "Your Language",
  "extensions": ["ext"],
  "keywords": ["keyword_one", "keyword_two"],
  "types": ["type_one", "type_two"],
  "builtins": ["builtin_one"],
  "comments": {
    "line": "//",
    "block_start": "/*",
    "block_end": "*/"
  },
  "indentation": {
    "tab_size": 4,
    "use_spaces": true,
    "auto_indent_on_brace": true
  },
  "build_run": {
    "build_command": "",
    "run_command": ""
  }
}
```

## Fields

- `id` (required string): Unique identifier you choose for the language.
- `name` (required string): Display name for the language.
- `extensions` (array of strings): File extensions without leading dots.
- `keywords` (array of strings): Tokens to highlight and use for completion.
- `types` (array of strings): Type names to highlight.
- `builtins` (array of strings): Built-in names available for completion/highlighting.
- `comments.line`: Single-line comment prefix.
- `comments.block_start` and `comments.block_end`: Multiline comment delimiters.
- `indentation.tab_size` (or `indent_width`): Indentation width; defaults to 4 when omitted.
- `indentation.use_spaces`: Use spaces rather than tabs.
- `indentation.auto_indent_on_brace`: Whether opening braces trigger the configured indentation behavior.
- `build_run.build_command`: Optional default build command template.
- `build_run.run_command`: Optional default run command template.

The parser accepts optional fields; unsupported or omitted optional settings do not need to be added to the JSON file. Use valid JSON syntax.

## Command macros

Build/run command templates may use these macros:

- `${file}`: Full path of the active file.
- `${fileDir}`: Directory containing the active file.
- `${fileName}`: File name including extension.
- `${fileBase}`: File name without its extension.
- `${fileExt}`: File extension.
- `${workspace}`: Workspace root path.
- `${workingDir}`: Resolved working directory.

Quote paths appropriately when writing shell commands, especially if paths may contain spaces or shell metacharacters. CinderDE substitutes these macros before executing the configured command.

## Notes

- Language IDs and names are user-defined; they do not populate a predefined language dropdown.
- A language pack provides editor metadata, not a compiler, interpreter, debugger, or language server.
- Install external language servers separately and configure them in the [LSP panel](lsp.md).
