# Design

How core2-template is put together and why. The decisions behind it, with dates, are in [decisions.md](decisions.md).

## Layout
```
core2-template/
  CLAUDE.md, README.md, docs/   this repo
  check.sh                      generates every variant, builds every mode, runs the binaries
  template/                     the payload (cargo-generate's subfolder)
    cargo-generate.toml         placeholders, Liquid exclusions, the hook
    post.rhai                   post hook: arranges files, deletes itself
    setup.sh                    run once in the client: submodules, then deletes itself
    CLAUDE.client.md            the client's CLAUDE.md (renamed by the hook)
    .claude/settings.json       the client's sandbox, permissions, comment-only hook
    .vscode/                    settings.json, launch.json, tasks.json
    .gitignore
    Makefile                    on core2's core.mk
    README.md                   the client's README
    docs/                       the client's docs (starts with a README)
    ui/<variant>/main.cpp       plain, ncurses, sdl3; the hook moves the chosen one to src/
    ui/imgui/<backend>/main.cpp sdl3_renderer, sdl3_gpu, glfw_opengl3, glfw_vulkan
```

## Placeholders
| Name | Values | Default | Notes |
|---|---|---|---|
| `project-name` | kebab-case | (asked) | cargo-generate's built-in. Names the binary `build/<project-name>_<mode>`. |
| `core2_version` | `vX.Y.Z` | `v0.13.1` | The core2 tag to pin. The default is the tag `check.sh` last passed with; `v0.13.1` is the oldest that works (it ships `docs/style.md` and the hook). |
| `core2_url` | git URL or path | `git@github.com:efvincent/core2.git` | Private repo, so SSH. `check.sh` passes a local clone. |
| `ui` | `plain`, `ncurses`, `sdl3`, `imgui` | `plain` | |
| `imgui_backend` | `sdl3_renderer`, `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan` | `sdl3_renderer` | Asked only when `ui == "imgui"` (`[conditional.'ui == "imgui"'.placeholders]`). |

## Generating
1. `cargo generate` copies `template/` to a temp dir and renders it with Liquid, except `ui/**` (`[template] exclude`): the C++ sources have no placeholders, and `{{` is valid C++.
2. The post hook runs in that temp dir. It renames `ui/<variant>` (`ui/imgui/<backend>` for ImGui) to `src`, deletes `ui/`, renames `CLAUDE.client.md` to `CLAUDE.md`, deletes itself, and prints the next step. It runs no system commands, so `-a` is not needed.
3. cargo-generate moves the result to the destination and runs a fresh `git init` there (no commit).
4. The author runs `sh <name>/setup.sh` once. It adds `third-party/core2` as a submodule from `core2_url`, checks out `core2_version`, stages it (the gitlink is the pin), does the same for ImGui at `v1.92.8` when `ui == "imgui"`, and deletes itself. It changes to its own directory first, so it can be run from anywhere.
5. Plain `make` builds `build/<project-name>_debug`.

Nothing is committed: the author makes the first commit.

### Why setup.sh, not the hook
The plan was for the hook to run `git init`, `git submodule add` and the checkout. That cannot work with cargo-generate 0.23.13 (checked 2026-10-31):
- The hook runs in a temp dir (`/tmp/cargo-generateXXXXXX`), before the move to the destination.
- The move skips every directory named `.git`, at any depth. The superproject's `.git`, `.git/modules/...` and a plain clone's `.git` are all dropped (a `.git` *file* survives, but points at nothing). This holds with `vcs = "None"` too.
- cargo-generate then runs its own `git init` in the destination, and nothing runs after it.

So the hook would leave a checkout with no git data and no gitlink. The author chose a one-time `setup.sh` (2026-10-31) over a wrapper script (would need a local clone of this repo) and a Makefile bootstrap (git logic and a second copy of the pin in the author's Makefile).

If the submodule is missing (a fresh clone without `--recurse-submodules`, or `setup.sh` not run yet), the Makefile stops with a message naming both fixes, and the comment-only hook, which lives in the submodule, blocks every `Edit`/`Write` (fail closed).

## The client's Makefile
Built on core2's `core.mk` (its header comment lists the inputs and outputs; core2's README has "Using core2 in another project" and "The flags contract").
- `CXX := clang++`. `CXXFLAGS` holds `-std=c++26 -fno-exceptions -fno-rtti -nostdinc++ -nostdlib++`, the warnings `-Wall -Wextra -Wpedantic -Wglobal-constructors -Wexit-time-destructors`, and the mode flags; it is set before `include third-party/core2/core.mk`, which builds core2 with the same flags (so `-DNDEBUG` always agrees).
- Modes, as in core2: `debug` (`-O0 -g -DBUILD_DEBUG`), `release` (`-O3 -march=native -DNDEBUG`), `lto` (release plus `-flto=full`), `instrument` (`-O1 -g -DBUILD_DEBUG -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -fno-sanitize-link-c++-runtime`; the last flag is needed with `-nostdlib++`).
- `CORE_BUILD := build/$(MODE)/core`. The client's modules (`src/**/*.cppm`) go through `$(call core_modules,APP,src,build/$(MODE)/app)`, guarded: `core_modules` stops make on a directory with no modules, and a new client has only `main.cpp`.
- Every `src/**/*.cpp` is compiled with `CORE_FLAGS` (so any of them can `import core;`) and linked with core2's and the client's module objects.
- `make bear` rebuilds with clang's `-MJ` (`CORE_EXTRA_FLAGS = $(MJFLAGS)` for the modules) and merges the fragments into `compile_commands.json`, as core2's `bear` target does.
- UI libraries come from `pkg-config`.
- Binary: `build/<project-name>_<mode>`.

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

The ImGui variants follow Dear ImGui's own `examples/example_<backend>/main.cpp` (v1.92.8, MIT), rewritten in core2's style: errors go to `console::eprintln`, Vulkan errors stop through `trap::panic`, and the state the example keeps in globals is a struct on `main`'s stack (globals with constructors would trip `-Wglobal-constructors`).

The UI libraries' compile flags (`pkg-config --cflags`) go on the project's own `.cpp` compile lines only, never into `CXXFLAGS`: core2 is built with `CXXFLAGS`, and `ncursesw` alone adds `-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600`. The Makefile stops with a message when `pkg-config` cannot find the packages.

ImGui and its backends are C++ that does not import core2. The Makefile compiles them with `CXXFLAGS` minus `-nostdinc++`, the warnings and `-I./src`, plus `-fno-threadsafe-statics` (no `__cxa_guard_*`) and `-I third-party/imgui`; the project's sources get `-isystem third-party/imgui`, so ImGui's headers raise no warnings there. All four backends link without the C++ runtime (checked 2026-10-31).

### Vulkan headers
The Vulkan loader's `vulkan.pc` (Arch: `vulkan-icd-loader`) brings no headers; they are a separate package (`vulkan-headers`). `glfw_vulkan` builds warning-free in all four modes with Vulkan-Headers 1.4.350 (first, through `CPATH`) and with the system's 1.4.363. The project uses the system package (decision 10), not a pinned `Vulkan-Headers` submodule as the old template did: the headers then match the loader `pkg-config` finds, and there is no third pin. The Makefile checks for `vulkan/vulkan.h` and stops with the package names when it is missing.

The old template (`~/code/cpp/project-template`, for the earlier `cpp-core` library) had one Liquid `if/elsif` `main.cpp`; one file per variant lets clangd parse each one, and keeps Liquid away from C++ braces.

## The client's Claude Code setup
Mirrors core2's (core2's `docs/claude.md`, "In client projects"):
- `CLAUDE.md`: the author writes `src/` and the `Makefile`; Claude writes `docs/` and, when asked for code, `proposals/`. Nobody edits `third-party/`. The style is imported with `@third-party/core2/docs/style.md`, so it follows the pin.
- `CLAUDE.md` also protects core2's sensitive parts as core2's own `CLAUDE.md` does (decision 9): `third-party/core2/src/`, `examples/`, `Makefile`, `core.mk` never, not even comments, and no grant in the client covers them; `tests/` is written only in core2's repo; core2's `CLAUDE.md`, `docs/style.md` and `.claude/` are read, never changed.
- `.claude/settings.json`: sandbox enabled, `failIfUnavailable`, no unsandboxed commands, `filesystem.denyWrite` on `./src`, `./Makefile` and `./third-party`, plus core2's own three by name (`./third-party/core2/src`, `tests`, `Makefile`), for the Bash route; `deny` on `Edit(/third-party/**)`, plus core2's protected paths by name (`src/**`, `tests/**`, `examples/**`, `Makefile`, `core.mk`, `CLAUDE.md`, `.claude/**`), for the file-tool route. The named entries repeat what `third-party` already covers, so they stay protected if that broad entry is ever narrowed; the `PreToolUse` comment-only hook on `Edit|Write|MultiEdit`, run from the submodule (`python3 "$CLAUDE_PROJECT_DIR"/third-party/core2/.claude/hooks/comment_only_guard.py`); `allow` for `Edit(/docs/**)`, `Edit(/proposals/**)`, `Edit(/README.md)` and the build targets. `.claude/settings.local.json` is git-ignored.
- `.vscode/`: clangd (`"C_Cpp.intelliSenseEngine": "disabled"`, `"makefile.configureOnOpen": false`, `*.cppm` as C++); `launch.json` runs `build/<project-name>_debug` under CodeLLDB (`"type": "lldb"`) after `build-debug`; `tasks.json` has `build-debug` (default), `build-release`, `bear`.
- `.gitignore`: `/build`, `/.cache`, `compile_commands.json`, `/.claude/settings.local.json`, `/proposals`.
- No tests in generated clients for now (decision 4).

## check.sh
For each variant: `cargo generate --path . template` into a temp dir with `--silent` and every placeholder defined (`core2_url` is the local clone, so core2 needs no network; `core2_version` is not passed, so the default in `cargo-generate.toml` is what gets checked), then `setup.sh` (with `protocol.file.allow=always` in the environment, which git needs to clone a local path as a submodule), then sanity checks on the result (no leftover template files or Liquid tags, the JSON files parse, the hook's self-test passes from the submodule), then `make -j` in all four modes (any `warning:` fails the check) plus `make bear` (the database must have entries for `src/main.cpp` and for core2). Then, in every mode:
- `plain`, and `ncurses` without a terminal: run with two arguments; the output is checked.
- `ncurses` on a pseudo-terminal (`script`, when installed): the screen must show the arguments and the prompt; a key press ends it.
- `sdl3` and `imgui` + `sdl3_renderer`: run for 2 seconds on SDL's `offscreen` video driver (no window on the desktop); they must still be running and have printed the line that follows their window setup.
- `imgui` + `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan`: built only. They need a surface that can present, which the offscreen driver lacks (SDL_GPU fails with "This surface does not support presenting!"). `WINDOWED=1 ./check.sh` runs them for 2 seconds each on the real display.

The binaries run in the project's directory, where ImGui writes `imgui.ini`. ImGui comes from GitHub over HTTPS (`setup.sh`), so the ImGui variants need the network.

Checked by hand for `plain` (2026-10-31), not by `check.sh`: the comment-only hook, run from the submodule with the client as `CLAUDE_PROJECT_DIR`, blocks a code edit in `src/` and allows a comment; `make` without the submodule stops with the message; a project module (`src/greet.cppm`, imported by `main.cpp`) builds in all four modes and lands in `compile_commands.json`; editing a header under `src/` rebuilds `main.cpp`.
