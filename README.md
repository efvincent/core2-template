# core2-template

A [Copier](https://copier.readthedocs.io) template for C++26 projects built on [core2](https://github.com/efvincent/core2) (Orthodox / Handmade C++: modules, no C++ standard library, no exceptions or RTTI). One `copier copy` gives a finished project: core2 as a git submodule pinned to a release tag, a CMake build with presets that links core2's `core2::core2` target, already built in `debug`, core2's HTML reference built and linked, VS Code / VSCodium set up for clangd, CMake Tools and CodeLLDB, and a Claude Code setup that reuses core2's.

## Setting up a machine
Checked on Arch Linux; the package names below are Arch's. Another distribution needs the same tools at the same versions or later, clang 23 above all, which older distributions do not ship.

### Tools
| Tool | Why | Checked with | Arch |
|---|---|---|---|
| Python 3 | Copier runs on it; `./dev setup` and core2's Claude Code hook use `python3` | 3.14 | `python` |
| pipx | installs Copier in its own environment | 1.18 | `python-pipx` |
| Copier 9.6 or later | generates the project | 9.19 | `pipx install copier` |
| git, and SSH access to core2's repository | the submodules; core2 is private | 2.56 | `git` |
| clang 23 with `clang-scan-deps`, `clangd` and `clang-tidy` of the same LLVM | C++26 modules; clangd must match the compiler | 23.1 | `clang` |
| `llvm-ar`, `llvm-ranlib` | the `lto` preset | 23.1 | `llvm` |
| CMake 3.30 or later | the build | 4.4 | `cmake` |
| Ninja | the presets' generator | 1.13 | `ninja` |
| Doxygen | core2's reference and the project's | 1.18 | `doxygen` |
| `pkg-config` | finding the UI libraries (every UI but `plain`) | 3.0 | `pkgconf` |
| VSCodium (or VS Code) | the editor the project is set up for | 1.143 | AUR `vscodium-bin` |

```sh
sudo pacman -S --needed python python-pipx git clang llvm cmake ninja doxygen pkgconf
pipx ensurepath          # puts ~/.local/bin on PATH; open a new shell after it
pipx install copier
ssh -T git@github.com    # must greet you by your GitHub name: core2 is cloned over SSH
```

Without an SSH key on GitHub, make one (`ssh-keygen -t ed25519`) and add `~/.ssh/id_ed25519.pub` under GitHub's *Settings, SSH and GPG keys*. Your account needs read access to `efvincent/core2`. `uv tool install copier` works in place of pipx.

The UI libraries, only for the UI you pick:

| UI | Needs (through `pkg-config`) | Arch |
|---|---|---|
| `plain` | | |
| `ncurses` | `ncursesw` | `ncurses` |
| `sdl3` | `sdl3` | `sdl3` |
| `imgui` + `sdl3_renderer`, `sdl3_gpu` | `sdl3` | `sdl3` |
| `imgui` + `glfw_opengl3` | `glfw3`, `gl` | `glfw`, `libglvnd` |
| `imgui` + `glfw_vulkan` | `glfw3`, `vulkan` | `glfw`, `vulkan-icd-loader`, `vulkan-headers` |

`imgui` also clones Dear ImGui from GitHub over HTTPS. `./dev setup` names any missing tool before it changes anything.

### VSCodium
Install VSCodium (`yay -S vscodium-bin`, or any AUR helper), then the three extensions the project recommends, from Open VSX (VSCodium's marketplace):

```sh
codium --install-extension llvm-vs-code-extensions.vscode-clangd   # checked with 0.6.0
codium --install-extension ms-vscode.cmake-tools                   # 1.25.10
codium --install-extension vadimcn.vscode-lldb                     # CodeLLDB 1.12.3
```

Opening a generated project also offers them (`.vscode/extensions.json`). Nothing else is needed in your user settings: the project's `.vscode/settings.json` sets the rest. What to know:
- **clangd** runs the `clangd` on your `PATH` (`/usr/bin/clangd` from the `clang` package). If the extension offers to download its own clangd, decline: a clangd from another LLVM cannot read clang 23's module files. If `clangd.path` is set in your user settings, it must point to clang 23's. The project's `clangd.arguments` replace any in your user settings; they turn on clang-tidy, with the project's `.clang-tidy`.
- **Microsoft's C/C++ extension** (`ms-vscode.cpptools`) is not needed and the project marks it unwanted; its IntelliSense would fight clangd. If you have it for other work, the project turns its IntelliSense off (`C_Cpp.intelliSenseEngine`).
- **CodeLLDB** brings its own lldb; no system `lldb` is needed. F5 runs the project's `launch.json`, Shift+F5 is CMake Tools' *Debug*, set to CodeLLDB too.
- **CMake Tools** asks once, on the first open, for a configure preset: pick `debug`.

## Generating a project

```sh
copier copy --trust gh:efvincent/core2-template my-app
```

Copier asks the questions below, copies the files, then runs the new project's `./dev setup` in it. When it finishes, `my-app/` has its submodules added and staged, `build/cmake/debug/my-app` built, and core2's reference at `docs/core2/html/index.html`. `cd my-app && ./dev run` runs it, and `./dev` alone lists the commands. Open the folder in VSCodium (set up as in [VSCodium](#vscodium)). Nothing is committed: make the first commit yourself, `.copier-answers.yml` included.

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
| `CLAUDE.md`, `.claude/settings.json` | the working agreement (imports core2's style) and the sandbox, with core2's comment-only hook run from the submodule. Claude cannot change either, nor `src/`, the CMake build, `dev` or `.clang-tidy`: it offers patches in `proposals/` |
| `.clang-tidy`, `third-party/.clang-tidy` | clang-tidy in core2's style (naming, trailing return types, initialized locals, bug patterns), run by clangd; off in the submodules |
| `.vscode/` | clangd (reading `build/cmake/debug/compile_commands.json`), CMake Tools on the presets, CodeLLDB (F5), build tasks, recommended extensions; for VS Code and VSCodium |
| `.copier-answers.yml` | the answers, for `copier update` |

CMake Tools keeps its selected preset in the editor's workspace state, which no project file can set, so on the first open it asks once for a configure preset: pick `debug`. clangd and F5 work without it.

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
