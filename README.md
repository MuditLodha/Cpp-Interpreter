# C++ Lox Interpreter

A tree-walk interpreter for the Lox language, implemented in C++. The project
follows the language and interpreter design presented in *Crafting
Interpreters*.

## Project layout

```
include/lox/  Interpreter headers and implementation
src/          Program entry points
examples/     Sample Lox programs
docs/         Project reference material
```

## Build

### With CMake

```powershell
cmake -S . -B build
cmake --build build
```

### With g++

```powershell
g++ -std=c++17 -Iinclude/lox src/main.cpp -o lox.exe
```

## Run

After building, run a Lox program such as:

```powershell
.\build\lox.exe .\examples\sample.lox
```

Or, for the direct g++ build:

```powershell
.\lox.exe .\examples\sample.lox
```

## Included files

Source code, sample programs, build configuration, and project documentation
are version-controlled. Generated executables and IDE/build output are ignored.
