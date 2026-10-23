# Installation and Build

This guide describes building CinderDE from source on Linux.

## Requirements

- A C++17-capable compiler, such as GCC or Clang.
- CMake 3.16 or newer.
- Qt 6 development libraries: Core, Gui, Widgets, and Test.
- Git, if cloning the repository.

## Install build dependencies

On Ubuntu or Linux Mint, the following packages provide the common build dependencies:

```bash
sudo apt update
sudo apt install git build-essential cmake qt6-base-dev
```

Package availability can vary by distribution and release. Install the corresponding Qt 6 development packages if your distribution uses different package names.

## Get the source

Clone the repository and enter its directory:

```bash
git clone https://github.com/BlazeBoyZ0/CinderDE.git
cd CinderDE
```

## Configure and build

From the repository root:

```bash
cmake -B build -S .
cmake --build build
```

The generated application executable is normally located at `build/cinderde`.

## Launch

```bash
./build/cinderde
```

## Run automated tests

```bash
ctest --test-dir build --output-on-failure
```

Run the test command after a successful build. Test results depend on the source revision and build environment; these instructions do not imply that a particular run has passed.

## Language features

CinderDE does not include language servers or bundled language definition packs. Install and configure external tools separately if you need language intelligence or language-specific highlighting. See [LSP setup](lsp.md) and [language packs](language-pack.md).
