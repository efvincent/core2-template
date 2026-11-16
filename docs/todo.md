# To do

The order of work from the handoff brief, with status (2026-11-16).

1. [x] This repo's `CLAUDE.md`, `README.md` and `docs/`, carrying over the brief's decisions.
   - [ ] The author deletes the brief, `~/code/cpp/core2/proposals/core2-template-brief.md` (git-ignored in core2; everything in it is now here). Still there on 2026-10-31.
2. [x] `plain` end to end: `cargo-generate.toml`, `post.rhai`, `setup.sh`, the Makefile, `main.cpp`, the client's `CLAUDE.md`, `.claude/settings.json`, `.vscode/`, `.gitignore`, `README.md`, `docs/core2.md`, and `check.sh`. Generates, builds warning-free in all four modes, runs (core2 `v0.13.1`).
   - Generating from GitHub (`--git https://github.com/efvincent/core2-template.git`, public since 2026-10-31) works, then `setup.sh`, build and run.
   - [x] `setup.sh` with the default SSH `core2_url`: `check.sh` uses it by default since 2026-10-31 (SSH to GitHub turned out to work from Claude's sandbox).
3. [x] `ncurses`, `sdl3`, and `imgui` with the four backends, each in `check.sh`. All seven variants build warning-free in all four modes and link without the C++ runtime. `./check.sh` runs `plain`, `ncurses` (also on a pseudo-terminal), `sdl3` and `imgui` + `sdl3_renderer` headless; `WINDOWED=1` ran `sdl3_gpu`, `glfw_opengl3` and `glfw_vulkan` on the author's desktop. `glfw_vulkan` uses the system Vulkan headers (decision 10).
4. [ ] The work on branch `feature/ui-variants` (four commits on `main`: core2 protection, the variants, check.sh fixes): the author pushes and merges it.
5. [x] In core2, in core2's own session: the docs that described core2-template as planned, or a client on make. Done in core2 `#40` (`8be1b2f`, docs only, no tag): README "A new project: core2-template" and "Adding it" (pins `v2.0.0`), `docs/claude.md` "In client projects" (core2's protected paths by name, the CMake build), `docs/roadmap.md`.
   - [ ] Still open, for a core2 session (handoff in this repo's git-ignored `proposals/core2-handoff.md`): a note on clang 23's missing debug info for templates instantiated across modules (`<incomplete type>` for `Result`, `Array`, `string::Split`), from c2app1's `proposals/clang-modules-incomplete-debug-types.md`, which is meant for core2's `proposals/`; the client README's note could then link to it. The LLVM report itself is unfiled (that file's steps 1 to 3).
6. [x] Align with core2 `v0.14.0` and decision 11 (2026-11-01): default tag `v0.14.0`; the client's `CLAUDE.md` scopes its `third-party/` rules and points at core2's submodule workflow; `setup.sh` writes core2's `.claude/settings.local.json`; the client's `docs/core2.md` describes fixing core2 from the submodule; `check.sh` checks the settings file and that the submodule stays clean.
   - [ ] Generated clients made before this (c2app1) get the scope line by hand (done in c2app1) and the settings file from the command in `docs/core2.md`.

7. [ ] CMake only, on core2 `v2.0.0` (decision 12), branch `feature/cmake`: `CMakeLists.txt` and `CMakePresets.json` in place of the Makefile, the client's docs, settings and `.vscode/` in CMake terms, `check.sh` on presets. The author commits, pushes and merges.
   - [ ] c2app1 has these already (it was the model). Clients generated before this stay on make with a `1.x` pin, or move by hand as c2app1 did.
8. [ ] Backports from c2app1 (decision 13), on `feature/cmake`. c2app1 can take the template's Doxyfile, `docs` target and clangd flags back if wanted.

## Open questions
None.
