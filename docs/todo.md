# To do

The order of work from the handoff brief, with status.

1. [x] This repo's `CLAUDE.md`, `README.md` and `docs/`, carrying over the brief's decisions (2026-10-31). The brief (`core2/proposals/core2-template-brief.md`) can then be deleted.
2. [x] `plain` end to end: `cargo-generate.toml`, `post.rhai`, `setup.sh`, the Makefile, `main.cpp`, the client's `CLAUDE.md`, `.claude/settings.json`, `.vscode/`, `.gitignore`, `README.md`, `docs/core2.md`, and `check.sh`. Generates, builds warning-free in all four modes, runs (2026-10-31, core2 `v0.13.1`).
   - Generating from GitHub (`--git https://github.com/efvincent/core2-template.git`, public since 2026-10-31) works, then `setup.sh`, build and run (core2 from the local clone: the sandbox blocks `github.com:22`). Not yet tried: `setup.sh` with the default SSH `core2_url`; the author's first real project will.
3. [x] `ncurses`, then `sdl3`, then `imgui` with the four backends, each added to `check.sh`.
   - [x] `ncurses`, `sdl3`, `imgui` + `sdl3_renderer`, `sdl3_gpu`, `glfw_opengl3`: generate, build warning-free in all four modes, link without the C++ runtime (2026-10-31). Run headless as far as possible ([design.md](design.md), "check.sh").
   - [x] `imgui` + `glfw_vulkan`: builds in all four modes against Vulkan-Headers 1.4.350 (through `CPATH`); uses the system headers (decision 10).
   - [x] `vulkan-headers` 1.4.363 installed; `./check.sh` passes all seven variants without `CPATH` (2026-10-31).
   - [x] `WINDOWED=1 ./check.sh` on the author's desktop: `sdl3_gpu`, `glfw_opengl3` and `glfw_vulkan` run in all four modes (2026-10-31).
4. [ ] In core2 (its own session and branch): point README's "Using core2 in another project" at core2-template, and update its roadmap line. The README's example pins `v0.6.0`; clients need `v0.13.1` or later.

## Open questions
- Decision 6: confirm the `<project>::<name>` namespace rule for client modules in core2's `docs/style.md`.
