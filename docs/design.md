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
    ui/<variant>/main.cpp       one per UI; the hook moves the chosen one to src/
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

| Variant | Libraries (pkg-config) | Installed here (2026-10-31) |
|---|---|---|
| `plain` | none | |
| `ncurses` | `ncursesw` | 6.6 |
| `sdl3` | `sdl3` | 3.4.18 |
| `imgui` + `sdl3_renderer` | `sdl3`, ImGui | |
| `imgui` + `sdl3_gpu` | `sdl3`, ImGui | |
| `imgui` + `glfw_opengl3` | `glfw3`, `gl` | glfw3 3.5.1, gl 1.2 |
| `imgui` + `glfw_vulkan` | `glfw3`, `vulkan` | vulkan 1.4.363 |

ImGui and its backends are C++ that does not import core2: they are compiled with their own flags (no `-nostdinc++` needed), plus `-fno-threadsafe-statics` so they do not need `__cxa_guard_*` (checked with ImGui 1.92.8, per core2's README). They link without the C++ runtime; what each backend pulls in is checked when it is added. The old template pinned `Vulkan-Headers` for `glfw_vulkan`; whether the system headers suffice is checked then too.

The old template (`~/code/cpp/project-template`, for the earlier `cpp-core` library) had one Liquid `if/elsif` `main.cpp`; one file per variant lets clangd parse each one, and keeps Liquid away from C++ braces.

## The client's Claude Code setup
Mirrors core2's (core2's `docs/claude.md`, "In client projects"):
- `CLAUDE.md`: the author writes `src/` and the `Makefile`; Claude writes `docs/` and, when asked for code, `proposals/`. Nobody edits `third-party/`. The style is imported with `@third-party/core2/docs/style.md`, so it follows the pin.
- `.claude/settings.json`: sandbox enabled, `failIfUnavailable`, no unsandboxed commands, `filesystem.denyWrite` on `./src`, `./Makefile` and `./third-party` (the Bash route); `deny` on `Edit(/third-party/**)` (the file-tool route); the `PreToolUse` comment-only hook on `Edit|Write|MultiEdit`, run from the submodule (`python3 "$CLAUDE_PROJECT_DIR"/third-party/core2/.claude/hooks/comment_only_guard.py`); `allow` for `Edit(/docs/**)`, `Edit(/proposals/**)`, `Edit(/README.md)` and the build targets. `.claude/settings.local.json` is git-ignored.
- `.vscode/`: clangd (`"C_Cpp.intelliSenseEngine": "disabled"`, `"makefile.configureOnOpen": false`, `*.cppm` as C++); `launch.json` runs `build/<project-name>_debug` under CodeLLDB (`"type": "lldb"`) after `build-debug`; `tasks.json` has `build-debug` (default), `build-release`, `bear`.
- `.gitignore`: `/build`, `/.cache`, `compile_commands.json`, `/.claude/settings.local.json`, `/proposals`.
- No tests in generated clients for now (decision 4).

## check.sh
For each variant: `cargo generate --path . template` into a temp dir with `--silent` and every placeholder defined (`core2_url` is the local clone, so core2 needs no network; `core2_version` is not passed, so the default in `cargo-generate.toml` is what gets checked), then `setup.sh` (with `protocol.file.allow=always` in the environment, which git needs to clone a local path as a submodule), then sanity checks on the result (no leftover template files or Liquid tags, the JSON files parse, the hook's self-test passes from the submodule), then `make` in all four modes (any `warning:` fails the check) plus `make bear` (the database must have entries for `src/main.cpp` and for core2). The `plain` binary runs in every mode and its output is checked. How each other variant is exercised is decided when it is added; the windowed ones can at least be built.

Checked by hand for `plain` (2026-10-31), not by `check.sh`: the comment-only hook, run from the submodule with the client as `CLAUDE_PROJECT_DIR`, blocks a code edit in `src/` and allows a comment; `make` without the submodule stops with the message; a project module (`src/greet.cppm`, imported by `main.cpp`) builds in all four modes and lands in `compile_commands.json`; editing a header under `src/` rebuilds `main.cpp`.
