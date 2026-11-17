# core2-template

A [Copier](https://copier.readthedocs.io) template for C++26 projects built on [core2](https://github.com/efvincent/core2) (Orthodox / Handmade C++: modules, no C++ standard library, no exceptions or RTTI). One `copier copy` gives a finished project: core2 as a git submodule pinned to a release tag, a CMake build with presets that links core2's `core2::core2` target, already built in `debug`, core2's HTML reference built and linked, VS Code / VSCodium set up for clangd, CMake Tools and CodeLLDB, and a Claude Code setup that reuses core2's.

## Generating a project

```sh
copier copy --trust gh:efvincent/core2-template my-app
```

Copier asks the questions below, copies the files, then runs the new project's `./dev setup` in it. When it finishes, `my-app/` has its submodules added and staged, `build/cmake/debug/my-app` built, and core2's reference at `docs/core2/html/index.html`. `cd my-app && ./dev run` runs it, and `./dev` alone lists the commands. Open the folder in VSCodium and install the recommended extensions when asked. Nothing is committed: make the first commit yourself, `.copier-answers.yml` included.

`--trust` is needed because the template runs `./dev setup`. To trust it once for good, add `gh:efvincent/core2-template` under `trust:` in Copier's settings file (`~/.config/copier/settings.yml`). Without `--trust`, Copier stops before copying anything.

| Question | Choices | Default |
|---|---|---|
| Project name | lowercase letters, digits and `-` | the destination folder's name |
| core2 release tag to pin | `vX.Y.Z`, `v2.0.0` or later (core2 is CMake only since `v2.0.0`) | `v2.1.8`, the latest tag `check.sh` passed with |
| core2 repository URL | | `git@github.com:efvincent/core2.git` |
| UI | `plain`, `ncurses`, `sdl3`, `imgui` | `plain` |
| ImGui backend (only for `imgui`) | `sdl3_renderer`, `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan` | `sdl3_renderer` |

Answer on the command line instead with `--data ui=sdl3` and the like, and `--defaults` for the rest. From a local clone: `copier copy --trust ~/code/cpp/core2-template my-app` (uncommitted changes in the clone are included).

`./dev` is the project's one command: `build [preset]`, `run [preset] [-- args]`, `docs`, `clean`, `setup`, `help`, asking for what is missing at a terminal and never elsewhere. `./dev setup` is the step to rerun after a fresh clone of the project (`build`, `run` and `docs` notice and offer it), and it is safe to rerun: it checks for the tools it needs first, adds missing submodules (core2, and for `imgui` Dear ImGui `v1.92.8`) at their recorded pins and never moves one, writes core2's git-ignored `.claude/settings.local.json` (so a Claude Code session in the submodule can commit), builds the debug preset, and builds core2's reference.

### Later template changes
`copier update` in the project merges what changed in the template since the project was generated (it needs a clean, committed work tree). It does not rerun `./dev setup` and does not move the core2 pin; bumping core2 is in the project's `docs/core2.md`.

## What a project gets
| Path | What |
|---|---|
| `src/main.cpp` | the chosen UI's starting program; the build takes every `src/**/*.cpp` and module `src/**/*.cppm` |
| `CMakeLists.txt` | `add_subdirectory(third-party/core2)`, the UI library through pkg-config, ImGui as its own static target, the `docs` and `core2_docs` targets |
| `CMakePresets.json` | `debug`, `release`, `lto`, `instrument`, each into `build/cmake/<preset>/` |
| `third-party/` | the core2 (and ImGui) submodules |
| `dev` | the one command above (executable as generated) |
| `README.md`, `docs/core2.md` | building, the editor, bumping the core2 pin, fixing core2 from the submodule |
| `docs/Doxyfile` | the project's reference (`docs/html/`), which fails on any warning and links core2's names into `docs/core2/html/` |
| `CLAUDE.md`, `.claude/settings.json` | the working agreement (imports core2's style) and the sandbox, with core2's comment-only hook run from the submodule. Claude cannot change either, nor `src/`, the CMake build or `dev`: it offers patches in `proposals/` |
| `.vscode/` | clangd (reading `build/cmake/debug/compile_commands.json`), CMake Tools on the presets, CodeLLDB (F5), build tasks, recommended extensions; for VS Code and VSCodium |
| `.copier-answers.yml` | the answers, for `copier update` |

CMake Tools keeps its selected preset in the editor's workspace state, which no project file can set, so on the first open it asks once for a configure preset: pick `debug`. clangd and F5 work without it.

## Prerequisites
| For | Needs | Arch packages |
|---|---|---|
| Generating | Copier 9.6 or later (checked with 9.19), which needs Python 3 | `pipx install copier` (or `uv tool install copier`) |
| Building | `clang++` with C++26 modules and the `clang-scan-deps` of the same LLVM (checked with clang 23.1), CMake 3.30 or later (checked with 4.4), Ninja, `llvm-ar` and `llvm-ranlib` (for the `lto` preset) | `clang`, `llvm`, `cmake`, `ninja` |
| `./dev setup` | git, SSH access to the core2 repository (private), `python3`, Doxygen (core2's reference); for `imgui`, HTTPS access to GitHub; for every UI but `plain`, `pkg-config` | `git`, `python`, `doxygen`, `pkgconf` |
| The editor | VS Code or VSCodium (checked with VSCodium), with the clangd, CMake Tools and CodeLLDB extensions | `vscodium` (AUR: `vscodium-bin`) |

| UI | Also needs (found through `pkg-config`) | Arch packages |
|---|---|---|
| `plain` | | |
| `ncurses` | `ncursesw` | `ncurses` |
| `sdl3` | `sdl3` | `sdl3` |
| `imgui` + `sdl3_renderer`, `sdl3_gpu` | `sdl3` | `sdl3` |
| `imgui` + `glfw_opengl3` | `glfw3`, `gl` | `glfw`, `libglvnd` |
| `imgui` + `glfw_vulkan` | `glfw3`, `vulkan` | `glfw`, `vulkan-icd-loader`, `vulkan-headers` |

`./dev setup` names any missing tool before it changes anything.

## Releases
Each release is a tag `vX.Y.Z` on `main`; [CHANGELOG.md](CHANGELOG.md) lists what each changed. `copier copy gh:efvincent/core2-template` uses the latest tag, and `copier update` in a project moves it to the latest (or `--vcs-ref vX.Y.Z`). The version is counted from a generated project's side: a patch release fixes it, a minor release adds to it, and a major release means `copier update` leaves work to do by hand (the changelog says what). A merge that changes only this repo (docs, `check.sh`) is not released.

## Working on the template
- [docs/design.md](docs/design.md): how it works, and why.
- [docs/decisions.md](docs/decisions.md): the decisions, with dates.
- [docs/todo.md](docs/todo.md): status and open work.

`./check.sh plain` (or `./check.sh` for all seven variants, about five minutes) generates the variants into a temp dir with Copier, from the working tree, which runs `./dev setup`. Then for every preset it configures, builds (any compiler or CMake warning fails it) and runs the binaries that can run headless. It also checks the generated tree, `./dev` (executable, its help and errors away from a terminal, the setup check, `clean`), a second `./dev setup` (the pin stays), `build/cmake/debug/compile_commands.json` where clangd looks, the message when the submodule is missing, that the core2 submodule stays clean, core2's reference, and the project's `docs` target with its links into core2's. Environment:
- `CORE2_VERSION=vX.Y.Z`: check another core2 tag. When it passes, bump the default of `core2_version` in `copier.yml`, and the facts in `docs/` and this README.
- `CORE2_DIR=<local clone>`: take core2 from a local clone instead of the default SSH URL (offline, or a tag not pushed yet).
- `WINDOWED=1`: also run `sdl3_gpu`, `glfw_opengl3` and `glfw_vulkan` on the real display, about a second each once their window is up.
- `KEEP=1`: keep the temp dir after a pass (it is always kept after a failure).
