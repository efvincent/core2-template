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

Without the submodules, `make` stops with a message, and Claude Code's comment-only hook (which lives in `third-party/core2`) blocks every edit.

## Building
Needs `clang++` with C++26 modules and GNU make 4.2 or later{% if ui == "ncurses" %}, and the ncursesw development files (`pkg-config ncursesw`){% elsif ui == "sdl3" %}, and SDL3 (`pkg-config sdl3`){% elsif ui == "imgui" %}{% if imgui_backend == "sdl3_renderer" or imgui_backend == "sdl3_gpu" %}, and SDL3 (`pkg-config sdl3`){% elsif imgui_backend == "glfw_opengl3" %}, GLFW and OpenGL (`pkg-config glfw3 gl`){% else %}, GLFW, the Vulkan loader and the Vulkan headers (`pkg-config glfw3 vulkan`, and `vulkan/vulkan.h`){% endif %}{% endif %}.

| Command | Builds |
|---|---|
| `make` | `build/{{project-name}}_debug` (`-O0 -g`) |
| `make release` | `build/{{project-name}}_release` (`-O3 -march=native -DNDEBUG`) |
| `make lto` | `build/{{project-name}}_lto` (release plus full LTO) |
| `make instrument` | `build/{{project-name}}_instrument` (AddressSanitizer, UndefinedBehaviorSanitizer) |
| `make bear` | `compile_commands.json`, for clangd |
| `make clean`, `make clean_all` | removes this mode's build, or all of `build/` |

`make MODE=release` is the same as `make release`.

Sources: `src/**/*.cpp` and the project's modules `src/**/*.cppm`. core2's `third-party/core2/core.mk` builds core2 with the same flags and scans the project's modules for their imports.

{% if ui == "imgui" -%}
Dear ImGui (`third-party/imgui`, pinned to `v1.92.8`) is compiled by the Makefile with its own flags; bump it the same way as core2 (below). It saves window positions to `imgui.ini` in the working directory (git-ignored).

{% endif -%}
## Editor
VS Code with clangd and CodeLLDB: run `make bear` once (and again when files are added or removed), then F5 builds and debugs `build/{{project-name}}_debug`.

## core2
core2 is a git submodule at `third-party/core2`, pinned to a release tag. How to bump it, and how a core2 change gets here: [docs/core2.md](docs/core2.md).
