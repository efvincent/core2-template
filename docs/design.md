# Design

How core2-template is put together and why. The decisions behind it, with dates, are in [decisions.md](decisions.md).

## Layout
```
core2-template/
  CLAUDE.md, README.md, docs/   this repo
  check.sh                      generates every variant, builds every preset, runs the binaries
  template/                     the payload (cargo-generate's subfolder)
    cargo-generate.toml         placeholders, Liquid exclusions, the hook
    post.rhai                   post hook: arranges files, deletes itself
    setup.sh                    run once in the client: submodules, then deletes itself
    CLAUDE.client.md            the client's CLAUDE.md (renamed by the hook)
    .claude/settings.json       the client's sandbox, permissions, comment-only hook
    .vscode/                    settings.json, launch.json, tasks.json
    .gitignore
    CMakeLists.txt              on core2's core2::core2 target (Liquid: the chosen UI's block only)
    CMakePresets.json           debug, release, lto, instrument
    README.md                   the client's README
    docs/                       the client's docs: core2.md, and the Doxyfile for the `docs` target
    ui/<variant>/main.cpp       plain, ncurses, sdl3; the hook moves the chosen one to src/
    ui/imgui/<backend>/main.cpp sdl3_renderer, sdl3_gpu, glfw_opengl3, glfw_vulkan
```

## Placeholders
| Name | Values | Default | Notes |
|---|---|---|---|
| `project-name` | kebab-case | (asked) | cargo-generate's built-in. Names the CMake project, the target and the binary `build/cmake/<preset>/<project-name>`. |
| `core2_version` | `vX.Y.Z`, major 2 or later (the regex) | `v2.0.0` | The core2 tag to pin; an older tag is rejected at generate time, `-d` included. The default is the tag `check.sh` last passed with. `v2.0.0` is the oldest the generated project fits: its build is CMake only, as core2's is since `v2.0.0` (`v1.1.0` has the CMake target, `v1.2.0` the link-time `NDEBUG` check the docs describe, and both still ship make); its settings name core2's CMake build, and the hook that guards the client's `CMakeLists.txt` is `v2.0.0`'s. |
| `core2_url` | git URL or path | `git@github.com:efvincent/core2.git` | Private repo, so SSH. `check.sh` uses it too, unless `CORE2_DIR` names a local clone. |
| `ui` | `plain`, `ncurses`, `sdl3`, `imgui` | `plain` | |
| `imgui_backend` | `sdl3_renderer`, `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan` | `sdl3_renderer` | Asked only when `ui == "imgui"` (`[conditional.'ui == "imgui"'.placeholders]`). |

## Generating
1. `cargo generate` copies `template/` to a temp dir and renders it with Liquid, except `ui/**` (`[template] exclude`): the C++ sources have no placeholders, and `{{` is valid C++.
2. The post hook runs in that temp dir. It renames `ui/<variant>` (`ui/imgui/<backend>` for ImGui) to `src`, deletes `ui/`, renames `CLAUDE.client.md` to `CLAUDE.md`, deletes itself, and prints the next step. It runs no system commands, so `-a` is not needed.
3. cargo-generate moves the result to the destination and runs a fresh `git init` there (no commit).
4. The author runs `sh <name>/setup.sh` once. It adds `third-party/core2` as a submodule from `core2_url`, checks out `core2_version`, stages it (the gitlink is the pin), does the same for ImGui at `v1.92.8` when `ui == "imgui"`, and deletes itself. It changes to its own directory first, so it can be run from anywhere.
5. `cmake --preset debug && cmake --build --preset debug` builds `build/cmake/debug/<project-name>`.

Nothing is committed: the author makes the first commit.

### Why setup.sh, not the hook
The plan was for the hook to run `git init`, `git submodule add` and the checkout. That cannot work with cargo-generate 0.23.13 (checked 2026-10-31):
- The hook runs in a temp dir (`/tmp/cargo-generateXXXXXX`), before the move to the destination.
- The move skips every directory named `.git`, at any depth. The superproject's `.git`, `.git/modules/...` and a plain clone's `.git` are all dropped (a `.git` *file* survives, but points at nothing). This holds with `vcs = "None"` too.
- cargo-generate then runs its own `git init` in the destination, and nothing runs after it.

So the hook would leave a checkout with no git data and no gitlink. The author chose a one-time `setup.sh` (2026-10-31) over a wrapper script (would need a local clone of this repo) and a Makefile bootstrap (git logic and a second copy of the pin in the author's Makefile).

If the submodule is missing (a fresh clone without `--recurse-submodules`, or `setup.sh` not run yet), `CMakeLists.txt` stops the configure with a message naming both fixes, and the comment-only hook, which lives in the submodule, blocks every `Edit`/`Write` (fail closed).

## The client's CMake build
Since 2026-11-15 (decision 12; core2 `v2.0.0` removed `core.mk`, its D45). c2app1, the first client moved to CMake, is the model. Liquid renders `CMakeLists.txt` with only the chosen UI's block; `CMakePresets.json` is the same for every variant.
- `CMakeLists.txt`: `cmake_minimum_required(VERSION 3.30...4.4)`, `project(<project-name> LANGUAGES CXX)`; a `FATAL_ERROR` when `third-party/core2/CMakeLists.txt` is missing, before `add_subdirectory(third-party/core2)`; `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` over `src/*.cppm` (a `CXX_MODULES` file set, `BASE_DIRS src`) and `src/*.cpp`, so every `.cpp` can `import core;`; `target_include_directories(PRIVATE src)`; the warnings `-Wall -Wextra -Wpedantic -Wglobal-constructors -Wexit-time-destructors` on the app target only; `target_link_libraries(PRIVATE core2::core2 ...)`.
- No required flags are repeated: `core2::core2` carries `cxx_std_26`, `-fno-exceptions -fno-rtti -nostdinc++` and `-nostdlib++` at link time. The app compiles as `gnu++26`, CMake's default (core2's D43, as in c2app1 and core2's `examples/hello`); CMake then compiles core2's interfaces a second time with the app's flags. The client's `docs/core2.md` says that `set(CMAKE_CXX_EXTENSIONS OFF)` skips that.
- `CMakePresets.json` (c2app1's): a hidden `base` (Ninja, `clang++`, `binaryDir` `build/cmake/${presetName}`, `CMAKE_EXPORT_COMPILE_COMMANDS`), then configure and build presets `debug` (`-O0 -g`), `release` (`-O3 -march=native -DNDEBUG -g`), `lto` (release plus `-flto=full`, with `CMAKE_AR`/`CMAKE_RANLIB` set to `llvm-ar`/`llvm-ranlib`) and `instrument` (`-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -fno-sanitize-link-c++-runtime`; the last flag is needed with `-nostdlib++`). The flags sit in `CMAKE_CXX_FLAGS_<CONFIG>`, which core2 shares in the same build, so `-DNDEBUG` agrees; a target-only `-DNDEBUG` fails to link on core2's mode check (D44). `-DBUILD_DEBUG`, which the Makefile set and nothing used, is gone.
- `release` and `lto` keep `-g` so profiles have symbols (the author's choice in c2app1).
- The `docs` target, defined when `find_program` finds `doxygen`, runs `doxygen docs/Doxyfile` from the root. The Doxyfile lists only non-default settings: `INPUT` README, `docs/` and `src/` (`*.cppm *.cpp *.h *.md`, `cppm=C++`), README as the main page, `detail` namespaces excluded, HTML only, `WARN_AS_ERROR = FAIL_ON_WARNINGS` (as core2). Output `docs/html/`, git-ignored with `docs/latex/`. It lives in `docs/`, which Claude writes in a client.
- Every configure writes `build/cmake/<preset>/compile_commands.json`; the client's README has the `ln -sf` to the root that clangd reads. There is no `make bear`.
- Binary: `build/cmake/<preset>/<project-name>`.

## UI variants
Each `ui/<variant>/main.cpp` uses core2 (`import core;`, `console::println`, `os::args` from `main`'s `argc`/`argv`) in the style of core2's `docs/style.md`. C headers for ncurses, SDL3 and GLFW are fine. While curses owns the screen, nothing goes through `console::print`.

| Variant | What `main.cpp` does | pkg-config packages | Installed here (2026-10-31) |
|---|---|---|---|
| `plain` | prints the arguments | none | |
| `ncurses` | shows the arguments on a curses screen until a key; without a terminal on stdin and stdout, prints them through core2 instead | `ncursesw` | 6.6 |
| `sdl3` | a window drawn with SDL_Renderer (a square moving back and forth) until Esc or close | `sdl3` | 3.4.18 |
| `imgui` + `sdl3_renderer` | ImGui window listing the arguments, a demo-window toggle, a background color, the frame rate | `sdl3` | |
| `imgui` + `sdl3_gpu` | the same, drawn with SDL_GPU | `sdl3` | |
| `imgui` + `glfw_opengl3` | the same, GLFW and OpenGL 3 (`main.cpp` calls `glClear` and friends, hence `gl`) | `glfw3 gl` | glfw3 3.5.1, gl 1.2 |
| `imgui` + `glfw_vulkan` | the same, GLFW and Vulkan, swapchain through ImGui's `ImGui_ImplVulkanH_*` helpers | `glfw3 vulkan`, plus the Vulkan headers | loader and headers 1.4.363 |

Each `main` has a Doxygen comment (`@brief`, `@param`, `@return`), so a new client's `docs` target is warning-free.

The ImGui variants follow Dear ImGui's own `examples/example_<backend>/main.cpp` (v1.92.8, MIT), rewritten in core2's style: errors go to `console::eprintln`, Vulkan errors stop through `trap::panic`, and the state the example keeps in globals is a struct on `main`'s stack (globals with constructors would trip `-Wglobal-constructors`).

The UI libraries come through `pkg_check_modules(ui QUIET IMPORTED_TARGET <packages>)`, and the app links `PkgConfig::ui`. Their compile flags then reach only targets that link it, never core2 (`ncursesw` alone adds `-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600`), and their include directories are system includes. One exception: CMake's second copy of core2's interfaces (D43's synthesized BMIs) is compiled with the importer's flags, so in `ncurses` it gets ncursesw's defines. Harmless here: with clang++'s `_GNU_SOURCE`, the C headers core2 includes preprocess to the same text and the same macros with and without them (checked 2026-11-15); `CMAKE_CXX_EXTENSIONS OFF` removes those BMIs altogether. `CMakeLists.txt` stops with a message naming the packages when `pkg-config` or the packages are missing.

ImGui and its backends are C++ that does not import core2. `CMakeLists.txt` builds them as a static `imgui` target (the five core files and the chosen backend's two) that does not link `core2::core2`, so it gets none of core2's flags (no `-nostdinc++`) and none of the app's warnings, plus `-fno-exceptions -fno-rtti -fno-threadsafe-statics` (no `__cxa_guard_*`) and `CXX_SCAN_FOR_MODULES OFF`. The build type's flags (optimization, LTO, sanitizers) reach it as they reach everything. `target_include_directories(imgui SYSTEM PUBLIC third-party/imgui)` gives the app ImGui's headers as system headers; `imgui` links `PkgConfig::ui` PUBLIC, and the app links `imgui`. `-nostdlib++` comes from `core2::core2` on the app's link line, as before: all four backends link without the C++ runtime (checked 2026-10-31 under make, 2026-11-15 under CMake).

### Vulkan headers
The Vulkan loader's `vulkan.pc` (Arch: `vulkan-icd-loader`) brings no headers; they are a separate package (`vulkan-headers`). `glfw_vulkan` builds warning-free in all four modes with Vulkan-Headers 1.4.350 (first, through `CPATH`) and with the system's 1.4.363. The project uses the system package (decision 10), not a pinned `Vulkan-Headers` submodule as the old template did: the headers then match the loader `pkg-config` finds, and there is no third pin. `CMakeLists.txt` looks for `vulkan/vulkan.h` (`find_path`, with the `vulkan` package's include directories as hints) and stops with the package names when it is missing.

The old template (`~/code/cpp/project-template`, for the earlier `cpp-core` library) had one Liquid `if/elsif` `main.cpp`; one file per variant lets clangd parse each one, and keeps Liquid away from C++ braces.

## The client's Claude Code setup
Mirrors core2's (core2's `docs/claude.md`, "In client projects"):
- `CLAUDE.md`: the author writes `src/` and the CMake build (`CMakeLists.txt`, `CMakePresets.json`); Claude writes `docs/` and, when asked for code, `proposals/`. Nobody edits `third-party/`. The style is imported with `@third-party/core2/docs/style.md`, so it follows the pin.
- `CLAUDE.md` scopes its `third-party/` rules to sessions started at the client's root (decision 11). Claude Code also loads it in a session started in `third-party/core2`, because it is in a parent directory; without the scope line it would forbid the core2 work that session is for.
- `CLAUDE.md` also protects core2's sensitive parts as core2's own `CLAUDE.md` does (decision 9): `third-party/core2/src/`, `examples/`, `tools/`, `bench/support/` and its CMake build (`CMakeLists.txt`, `CMakePresets.json`, `cmake/`) never, not even comments, and no grant in the client covers them; `tests/` is written only in core2's repo; core2's `CLAUDE.md`, `docs/style.md` and `.claude/` are read, never changed.
- `.claude/settings.json`: sandbox enabled, `failIfUnavailable`, no unsandboxed commands, `filesystem.denyWrite` on `./src`, `./CMakeLists.txt`, `./CMakePresets.json` and `./third-party`, plus core2's own by name, as core2 `v2.0.0`'s `denyWrite` lists them (`./third-party/core2/src`, `tests`, `CMakeLists.txt`, `CMakePresets.json`, `cmake`, `bench/support`), for the Bash route; `deny` on `Edit(/third-party/**)`, plus core2's protected paths by name (`src/**`, `tests/**`, `examples/**`, `tools/**`, `bench/support/**`, `cmake/**`, `CMakeLists.txt`, `CMakePresets.json`, `CLAUDE.md`, `.claude/**`), for the file-tool route. The named entries repeat what `third-party` already covers, so they stay protected if that broad entry is ever narrowed; the `PreToolUse` comment-only hook on `Edit|Write|MultiEdit`, run from the submodule (`python3 "$CLAUDE_PROJECT_DIR"/third-party/core2/.claude/hooks/comment_only_guard.py`); `allow` for `Edit(/docs/**)`, `Edit(/proposals/**)`, `Edit(/README.md)`, `cmake --preset <p>` and `cmake --build --preset <p>` for the four presets, and `cmake --build --preset debug --target docs`. core2 `v2.0.0`'s hook guards the client's root `CMakeLists.txt` (whole-line `#` comments only) and `CMakePresets.json` (no change), as it guarded the `Makefile`. `.claude/settings.local.json` is git-ignored.
- core2's `.claude/settings.local.json`, written by `setup.sh` into the submodule (core2 git-ignores it, so the submodule stays clean): `sandbox.filesystem.allowWrite` on the submodule's git directory (`git rev-parse --absolute-git-dir`, under the client's `.git/modules/`) with `denyWrite` on its `hooks/` and `config`, so Claude can commit in a core2 session (decision 11). The path is absolute; the client's `docs/core2.md` has the command to recreate it after a clone or a move. `check.sh` checks its contents and that the submodule stays clean.
- The client's `CLAUDE.md` and `docs/core2.md` have the author run a pin bump's `fetch` and `checkout`: from Claude's sandbox the fetch needs `github.com:22`, and the checkout moves the submodule's `HEAD` and index but cannot write under `./third-party`. Claude checks the result (`status --short`, `describe --tags`). Found in c2app1's bump to `v2.0.0`.
- `.vscode/`: clangd (`"C_Cpp.intelliSenseEngine": "disabled"`, `--compile-commands-dir=${workspaceFolder}`, `--header-insertion=never` so completion adds no standard-library `#include`s, `--background-index`, `--completion-style=detailed`, `*.cppm` as C++); `launch.json` runs `build/cmake/debug/<project-name>` under CodeLLDB (`"type": "lldb"`) after `build-debug`, with `args: []` (the client's README shows fixed `args` and `promptString` inputs); `tasks.json` has `build-debug` (default) and `build-release`, each `cmake --build --preset <p>`.
- `.gitignore`: `/build`, `/.cache`, `compile_commands.json`, `vgcore*`, `/.claude/settings.local.json`, `/proposals`, `/docs/html`, `/docs/latex`, `.idea` (the sandbox mounts `/dev/null` there).
- The client's README also notes clang 23's missing debug info for core2 templates instantiated across modules (`<incomplete type>` in the debugger), with the workaround; found in c2app1 (its `proposals/clang-modules-incomplete-debug-types.md`).
- No tests in generated clients for now (decision 4).

## check.sh
For each variant: `cargo generate --path . template` into a temp dir with `--silent` and every placeholder defined (`core2_url` and `core2_version` are the defaults read from `cargo-generate.toml`, so what users get is what gets checked; `CORE2_DIR` swaps in a local clone, and `CORE2_VERSION` another tag), then `setup.sh` (with a local clone, also `protocol.file.allow=always` in the environment, which git needs to clone a local path as a submodule), then sanity checks on the result (no leftover template files or Liquid tags, the JSON files parse, `CMakePresets.json` included, the hook's self-test passes from the submodule, core2's `settings.local.json` and a clean submodule). Then:
- Without the submodule: `CMakeLists.txt`, `CMakePresets.json` and `src/` copied into an empty directory; `cmake --preset debug` there must fail with the `git submodule update --init` message.
- Every preset: `cmake --preset <p>`, then `cmake --build --preset <p>`; a `warning:` or `CMake Warning` in either fails the check. Then the runs below.
- `build/cmake/debug/compile_commands.json` must have entries for `src/main.cpp` and for core2, and `third-party/core2` must still be clean after all the builds (CMake writes only under the client's `build/`).
- With `doxygen` installed: the `docs` target must pass with no warning, write `docs/html/index.html`, and `docs/html` must be git-ignored.

The runs, in every preset:
- `plain`, and `ncurses` without a terminal: run with two arguments; the output is checked.
- `ncurses` on a pseudo-terminal (`script`, when installed): the screen must show the arguments and the prompt; a key press ends it.
- `sdl3` and `imgui` + `sdl3_renderer`: run on SDL's `offscreen` video driver (no window on the desktop). `check.sh` waits up to 20 seconds for the line each prints once its window is set up, lets it draw for one more second, then stops it; exiting on its own, or no line in time, fails. It reports how long setup took. It replaced a fixed 2-second limit, which `glfw_vulkan`'s instrument build missed on the author's desktop: under the sanitizers its window is up after 2.6 seconds, against 0.3 in the other modes (2026-10-31), most likely the Vulkan loader loading this machine's three drivers (Intel, Intel hasvk, NVIDIA).
- `imgui` + `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan`: built only. They need a surface that can present, which the offscreen driver lacks (SDL_GPU fails with "This surface does not support presenting!"). `WINDOWED=1 ./check.sh` runs them the same way on the real display.

The binaries run in the project's directory, where ImGui writes `imgui.ini`. ImGui comes from GitHub over HTTPS (`setup.sh`), so the ImGui variants need the network.

Checked by hand for `plain` under make (2026-10-31), not by `check.sh`: the comment-only hook, run from the submodule with the client as `CLAUDE_PROJECT_DIR`, blocks a code edit in `src/` and allows a comment; a project module (`src/greet.cppm`, imported by `main.cpp`) builds in all four modes and lands in `compile_commands.json`; editing a header under `src/` rebuilds `main.cpp`. Under CMake (2026-11-15), on a generated `plain`: the hook blocks a code edit to `CMakeLists.txt`, `CMakePresets.json` and `src/main.cpp`, and allows a comment line in `CMakeLists.txt`; a project module in a subdirectory (`src/util/greet.cppm`, imported by `main.cpp`) builds without a new `cmake --preset` (the globs are `CONFIGURE_DEPENDS`) and lands in `compile_commands.json`; editing a header under `src/` rebuilds `main.cpp`. Also checked: no binary of any variant links a C++ runtime (`ldd`, debug, lto, instrument); ImGui's compile lines have no `-nostdinc++`; the app sees ImGui as `-isystem`; 24 synthesized BMIs per build directory, none with `CMAKE_CXX_EXTENSIONS OFF`.
