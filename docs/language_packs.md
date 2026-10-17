# CinderDE Language Packs

CinderDE v0.7 introduces a lightweight, extensible language configuration system using JSON files.

## Discovery and Search Locations

Language packs (`*.json`) are automatically loaded on startup from the following locations:

1. **Embedded Qt Resources**: `:/resources/languages/`
2. **Local Directory**: `./languages/`
3. **Application Share Directory**: `../share/cinderde/languages/`
4. **User Configuration Directory**: `~/.config/cinderde/languages/`

User-defined language packs loaded from disk take precedence over default embedded configurations.

## JSON Format Specification

Each language pack is a JSON object with the following fields:

```json
{
  "id": "kekno",
  "name": "Kekno Source",
  "extensions": ["kek", "kekno"],
  "keywords": ["fn", "let", "mut", "if", "else", "return"],
  "types": ["int", "bool", "str"],
  "builtins": ["print", "len"],
  "comments": {
    "line": ";",
    "block_start": "/*",
    "block_end": "*/"
  },
  "indentation": {
    "tab_size": 4,
    "use_spaces": true,
    "auto_indent_on_brace": true
  },
  "build_run": {
    "build_command": "keknoc \"${file}\" -o \"${fileDir}/${fileBase}\"",
    "run_command": "\"${fileDir}/${fileBase}\""
  }
}
```

### Field Reference

- `id` (string, required): Lowercase unique language identifier (e.g. `"cpp"`, `"python"`, `"kekno"`).
- `name` (string, required): Human-readable display name (e.g. `"C++ Source"`).
- `extensions` (array of strings, optional): List of file extensions without leading dots (e.g. `["cpp", "cc", "hpp"]`).
- `keywords` (array of strings, optional): Language keyword tokens.
- `types` (array of strings, optional): Primitive types and built-in type identifiers.
- `builtins` (array of strings, optional): Standard library or global functions.
- `comments` (object, optional):
  - `line`: Single-line comment prefix string (e.g. `"//"` or `"#"` or `";"`).
  - `block_start`: Multiline comment opening token (e.g. `"/*"`).
  - `block_end`: Multiline comment closing token (e.g. `"*/"`).
- `indentation` (object, optional):
  - `tab_size` or `indent_width`: Number of spaces for indent step (default `4`).
  - `use_spaces`: `true` to insert spaces, `false` for tabs.
  - `auto_indent_on_brace`: `true` to auto-indent when opening braces `{` are typed.
- `build_run` (object, optional):
  - `build_command`: Default compile command template with macro variables.
  - `run_command`: Default execution command template.

### Supported Macro Variables
- `${file}`: Full path of current active file.
- `${fileDir}`: Directory containing current file.
- `${fileName}`: File name with extension.
- `${fileBase}`: File name without extension.
- `${fileExt}`: File extension.
- `${workspace}`: Active workspace root folder path.
- `${workingDir}`: Resolved execution working directory.
