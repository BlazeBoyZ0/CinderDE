# CinderDE — Agent Instructions

## 1. Project Overview

CinderDE (Cinder Development Environment) is a lightweight, VS Code-inspired code editor built primarily for Linux desktop systems with limited RAM.

- Language: C++
- GUI framework: Qt 6 Widgets
- Editor component: QPlainTextEdit
- Build system: CMake
- Design: dark charcoal with ember-orange accents
- Primary goal: a capable, responsive coding environment without unnecessary resource consumption.

CinderDE is intended to grow incrementally. Prefer small, maintainable improvements over unnecessary architectural complexity.

## 2. Mandatory Rules

- Do not update README.md.
- Inspect relevant existing code before modifying it.
- Preserve working functionality unless the task explicitly requires changing it.
- Do not rewrite or refactor unrelated modules.
- Avoid adding dependencies unless they provide a clear, necessary benefit.
- Keep resource usage low. Avoid unnecessary background services, processes, and repeated full-document processing.
- Follow the existing architecture and coding conventions whenever practical.
- Do not assume a bug's root cause. Investigate the relevant execution path before implementing a fix.
- Never claim tests passed unless they were actually run successfully.
- Do not silently ignore errors that should be reported to the user.

## 3. Repository Exploration

Do not read the entire repository in detail for every task.

1. Locate the implementation responsible for the requested feature.
2. Inspect its directly related headers, source files, callers, configuration, and tests.
3. Expand the investigation only when dependencies or cross-module behavior require it.
4. Reuse existing utilities and abstractions rather than creating duplicate implementations.
5. Before changing shared behavior, identify the existing functionality that could be affected.

Use the repository's actual structure as the source of truth. Do not assume filenames or class names without checking.

## 4. Architecture Awareness

CinderDE contains or may contain modules responsible for:

- Main window, menus, actions, and keyboard shortcuts.
- Editor widgets, tabs, and text-editing behavior.
- Syntax highlighting and language recognition.
- File explorer and workspace operations.
- Workspace and session persistence.
- Preferences and configuration.
- Build/run orchestration and external processes.
- Output panel and compiler diagnostics.
- Automated tests.

These are functional areas, not guaranteed filenames. Locate the actual implementations before editing.

Keep responsibilities separated. For example, terminal-launch fixes should primarily affect the build/run process rather than the editor widget or workspace persistence.

## 5. Build and Testing

The project uses CMake. The known configuration and build commands are:

`cmake -B build -S .`

`cmake --build build`

Use the existing build directory when appropriate. Inspect the project's CMake configuration to discover the available test targets and execution commands.

After making changes:

- Build the project.
- Run the relevant tests.
- Add focused regression tests when practical.
- Check that related existing functionality remains intact.
- Report commands executed, results, and any tests that could not be run.

Avoid unnecessary clean rebuilds unless needed to diagnose a problem.

## 6. Feature and Bug-Fix Workflow

For bug fixes:

1. Reproduce or trace the reported behavior.
2. Identify the most likely responsible component and confirm the root cause.
3. Make the smallest reliable change.
4. Test both the fix and relevant regression scenarios.
5. Report the root cause, modified files, and verification results.

For new features:

1. Inspect existing patterns and interfaces.
2. Integrate with current settings and keyboard shortcuts where appropriate.
3. Avoid conflicting shortcuts or duplicating existing functionality.
4. Keep new functionality modular and extensible.
5. Respect the current release's scope.

## 7. Language and Extensibility

CinderDE is intended to support C, C++, Python, Kotlin, Java, JavaScript, and Kekno over time.

Avoid hardcoding assumptions that make future language support unnecessarily difficult. Prefer reusable interfaces and configurable behavior where justified, without prematurely building a complicated plugin framework.

## 8. Completion Report

At the end of each task, summarize:

- What changed and why.
- Which files were modified.
- Which build commands and tests were run.
- Whether verification succeeded.
- Any remaining limitations or known issues.

Do not include unrelated changes or claim completion of unverified behavior.
