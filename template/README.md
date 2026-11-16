# {{project-name}}

A C++26 project on [core2](https://github.com/efvincent/core2) (Orthodox / Handmade C++: modules, no C++ standard library, no exceptions or RTTI), generated from core2-template{% if ui == "imgui" %} with Dear ImGui ({{imgui_backend}}){% elsif ui != "plain" %} with {{ui}}{% endif %}.

## Getting started
Once, right after generating: `sh setup.sh` (adds the pinned submodules, then deletes itself).

After a fresh clone:
```sh
git clone --recurse-submodules <url>
# or, in an existing clone:
git submodule update --init
```

Without the submodules, CMake stops with a message, and Claude Code's comment-only hook (which lives in `third-party/core2`) blocks every edit.

## Building
CMake, with presets. Needs `clang++` with C++26 modules, CMake 3.30 or later, Ninja, and the `clang-scan-deps` of the same LLVM as `clang++` (plus `llvm-ar` and `llvm-ranlib` for `lto`){% if ui == "ncurses" %}, and the ncursesw development files (`pkg-config ncursesw`){% elsif ui == "sdl3" %}, and SDL3 (`pkg-config sdl3`){% elsif ui == "imgui" %}{% if imgui_backend == "sdl3_renderer" or imgui_backend == "sdl3_gpu" %}, and SDL3 (`pkg-config sdl3`){% elsif imgui_backend == "glfw_opengl3" %}, GLFW and OpenGL (`pkg-config glfw3 gl`){% else %}, GLFW, the Vulkan loader and the Vulkan headers (`pkg-config glfw3 vulkan`, and `vulkan/vulkan.h`){% endif %}{% endif %}.

| Command | Builds |
|---|---|
| `cmake --preset debug` | configures `build/cmake/debug/` (once per preset) |
| `cmake --build --preset debug` | `build/cmake/debug/{{project-name}}` (`-O0 -g`) |
| `cmake --build --preset release` | `build/cmake/release/{{project-name}}` (`-O3 -march=native -DNDEBUG`, plus `-g` for profiling) |
| `cmake --build --preset lto` | `build/cmake/lto/{{project-name}}` (release plus full LTO, also with `-g`) |
| `cmake --build --preset instrument` | `build/cmake/instrument/{{project-name}}` (AddressSanitizer, UndefinedBehaviorSanitizer) |
| `cmake --build --preset debug --target docs` | the reference, `docs/html/index.html` (Doxygen; fails on any warning) |
| `cmake --build --preset debug --target clean` | removes that preset's build output |

Each preset needs its `cmake --preset <name>` once before its first build; after that, a build reconfigures by itself when needed (a source added or removed under `src/` included).

core2 comes in through `add_subdirectory(third-party/core2)` and the target `core2::core2`, which brings core2's required flags with it; core2 is built from the submodule's working tree, inside this build. Module dependencies come from `clang-scan-deps`. Sources: `src/**/*.cpp` and the project's modules `src/**/*.cppm`.

`-DNDEBUG` comes only from the presets' build types (`CMAKE_CXX_FLAGS_<CONFIG>`), which core2 shares; never put it on a target. A mismatch fails to link (see [docs/core2.md](docs/core2.md), "Flags and core2").

{% if ui == "imgui" -%}
Dear ImGui (`third-party/imgui`, pinned to `v1.92.8`) is the `imgui` target in `CMakeLists.txt`, compiled with its own flags; bump it the same way as core2 (below). It saves window positions to `imgui.ini` in the working directory (git-ignored).

{% endif -%}
## Editor
VS Code with clangd and CodeLLDB. clangd reads `compile_commands.json` at the root. Every configure writes one for its preset; link the debug one there once:
```sh
ln -sf build/cmake/debug/compile_commands.json compile_commands.json
```
Its module flags point at files the build writes, so build once before opening the editor. F5 builds and debugs `build/cmake/debug/{{project-name}}`.

To pass arguments under F5, set `"args"` in `.vscode/launch.json`, one string per argument: `"args": ["one", "two words"]`. To be asked each time instead, give each argument an input:
```jsonc
"args": ["${input:name}"],
// next to "configurations":
"inputs": [
  { "id": "name", "type": "promptString", "description": "Name:", "default": "world" }
]
```

The debugger shows some core2 values as `<incomplete type>` (CodeLLDB: `<not available>`): `Result`, `Array`, `string::Split` and other templates defined in one core2 module and instantiated in another. clang 23 writes no debug information for such an instance anywhere, so neither lldb nor gdb can show it; `-fstandalone-debug` does not help. Until a fixed clang, copy the fields you need into plain locals (`u64 v = r.val;`), or use `memory read &r` in the lldb console.

## Docs
`docs/` holds the project's own pages (Markdown) and `Doxyfile`. `cmake --build --preset debug --target docs` (needs `doxygen`; the target exists only when CMake finds it) builds the reference from the Doxygen comments in `src/`, with this README as the main page, into `docs/html/` (git-ignored). Any warning fails it, an undocumented public name included; names in a `detail` namespace are left out.

## core2
core2 is a git submodule at `third-party/core2`, pinned to a release tag. How to bump it, and how to fix core2 from here (a core2 session in `third-party/core2`, through core2's own gate): [docs/core2.md](docs/core2.md).
