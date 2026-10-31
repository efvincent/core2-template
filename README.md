# core2-template

A [cargo-generate](https://github.com/cargo-generate/cargo-generate) template for C++26 projects built on [core2](https://github.com/efvincent/core2) (Orthodox / Handmade C++: modules, no C++ standard library, no exceptions or RTTI). A generated project has core2 as a git submodule pinned to a release tag, a Makefile on core2's `core.mk`, VS Code and clangd settings, and a Claude Code setup that reuses core2's.

## Generating a project

```sh
cargo generate --git https://github.com/efvincent/core2-template.git template --name my-app
sh my-app/setup.sh
cd my-app && make && ./build/my-app_debug
```

From a local clone: `cargo generate --path ~/code/cpp/core2-template template --name my-app`.

cargo-generate asks for:
| Question | Choices | Default |
|---|---|---|
| core2 release tag to pin | `vX.Y.Z` (`v0.13.1` or later) | the latest tag this template was checked with |
| core2 repository URL | | `git@github.com:efvincent/core2.git` |
| UI | `plain`, `ncurses`, `sdl3`, `imgui` | `plain` |
| ImGui backend (only for `imgui`) | `sdl3_renderer`, `sdl3_gpu`, `glfw_opengl3`, `glfw_vulkan` | `sdl3_renderer` |

Answer on the command line instead with `-d ui=sdl3` and the like (`--silent` fails on anything not given).

`setup.sh` is a one-time step: it adds core2 (and, for `imgui`, Dear ImGui `v1.92.8`) as submodules at `third-party/`, checks out the pinned tags, stages them, and deletes itself. It is separate because cargo-generate cannot carry git data out of its hooks ([docs/design.md](docs/design.md), "Why setup.sh, not the hook"). Nothing is committed; make the first commit yourself.

## Prerequisites
All variants: `clang++` with C++26 modules (checked with clang 23), GNU make 4.2 or later, git, and SSH access to the core2 repository (private). `make bear` and the VS Code setup use clangd and CodeLLDB.

| UI | Also needs (found through `pkg-config`) | Arch packages |
|---|---|---|
| `plain` | | |
| `ncurses` | `ncursesw` | `ncurses` |
| `sdl3` | `sdl3` | `sdl3` |
| `imgui` + `sdl3_renderer`, `sdl3_gpu` | `sdl3` | `sdl3` |
| `imgui` + `glfw_opengl3` | `glfw3`, `gl` | `glfw`, `libglvnd` |
| `imgui` + `glfw_vulkan` | `glfw3`, `vulkan` | `glfw`, `vulkan-icd-loader`, `vulkan-headers` |

Dear ImGui itself comes in as a submodule; it needs network access to GitHub (HTTPS) during `setup.sh`.

Status: every variant generates, builds in every mode, and passes `./check.sh` ([docs/todo.md](docs/todo.md)).

## Working on the template
- [docs/design.md](docs/design.md): how it works, and why.
- [docs/decisions.md](docs/decisions.md): the decisions, with dates.
- `./check.sh` generates every variant into a temp dir, builds each in every mode, and runs the ones that can run headless (`WINDOWED=1` also runs the rest on the real display, about a second each once their window is up). It uses the core2 clone at `~/code/cpp/core2` (`CORE2_DIR` overrides it) and the default tag from `template/cargo-generate.toml` (`CORE2_VERSION` overrides it).
