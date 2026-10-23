# CinderDE User Language Packs

CinderDE v1.0.0 ships with no built-in language packs. Users can optionally add language definition JSON files to extend syntax highlighting, completion, comments, indentation, and build/run defaults.

## Discovery and Search Locations

User language packs (`*.json`) are loaded from the application configuration directory:

- **User Configuration Directory**: `~/.config/cinderde/languages/`

There are no embedded defaults or bundled language definitions.

## JSON Format Specification

Each language pack is a JSON object with the following fields:

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

### Field Reference

- `id` (string, required): Unique language identifier chosen by the user.
- `name` (string, required): Human-readable display name chosen by the user.
- `extensions` (array of strings, optional): List of file extensions without leading dots.
- `keywords` (array of strings, optional): Language keyword tokens.
- `types` (array of strings, optional): Primitive types and built-in type identifiers.
- `builtins` (array of strings, optional): Standard library or global functions.
- `comments` (object, optional):
  - `line`: Single-line comment prefix string.
  - `block_start`: Multiline comment opening token.
  - `block_end`: Multiline comment closing token.
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
