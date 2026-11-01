# To do

The order of work from the handoff brief, with status (2026-10-31).

1. [x] This repo's `CLAUDE.md`, `README.md` and `docs/`, carrying over the brief's decisions.
   - [ ] The author deletes the brief, `~/code/cpp/core2/proposals/core2-template-brief.md` (git-ignored in core2; everything in it is now here). Still there on 2026-10-31.
2. [x] `plain` end to end: `cargo-generate.toml`, `post.rhai`, `setup.sh`, the Makefile, `main.cpp`, the client's `CLAUDE.md`, `.claude/settings.json`, `.vscode/`, `.gitignore`, `README.md`, `docs/core2.md`, and `check.sh`. Generates, builds warning-free in all four modes, runs (core2 `v0.13.1`).
   - Generating from GitHub (`--git https://github.com/efvincent/core2-template.git`, public since 2026-10-31) works, then `setup.sh`, build and run.
   - [x] `setup.sh` with the default SSH `core2_url`: `check.sh` uses it by default since 2026-10-31 (SSH to GitHub turned out to work from Claude's sandbox).
3. [x] `ncurses`, `sdl3`, and `imgui` with the four backends, each in `check.sh`. All seven variants build warning-free in all four modes and link without the C++ runtime. `./check.sh` runs `plain`, `ncurses` (also on a pseudo-terminal), `sdl3` and `imgui` + `sdl3_renderer` headless; `WINDOWED=1` ran `sdl3_gpu`, `glfw_opengl3` and `glfw_vulkan` on the author's desktop. `glfw_vulkan` uses the system Vulkan headers (decision 10).
4. [ ] The work on branch `feature/ui-variants` (four commits on `main`: core2 protection, the variants, check.sh fixes): the author pushes and merges it.
5. [ ] In core2, in core2's own session and on a core2 branch (this repo never edits core2), the docs that still describe core2-template as planned or in progress:
   - `README.md`, "Using core2 in another project": lead with core2-template for new projects (`cargo generate --git https://github.com/efvincent/core2-template.git template`, then `sh setup.sh`), and keep the manual submodule and `core.mk` instructions for adding core2 to an existing project. Its "Adding it" example pins `v0.6.0`; clients need `v0.13.1` or later.
   - `README.md`, roadmap list: "In progress: `core2-template`" becomes done.
   - `docs/roadmap.md`: the same "In progress" line.
   - `docs/todo.md`: the "Resume here: `core2-template`" and "Then: `core2-template`" items, which point at the brief.
   - `docs/claude.md`, "In client projects": could add that the client's settings also block core2's own protected paths by name (decision 9).
   - Docs only, so no version bump or tag under core2's rules, unless the author wants one.

## Open questions
None.
