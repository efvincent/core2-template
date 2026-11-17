# To do

The order of work from the handoff brief, with status (2026-11-17, after the merge of `v1.0.0`).

1. [x] This repo's `CLAUDE.md`, `README.md` and `docs/`, carrying over the brief's decisions.
   - [ ] The author deletes the brief, `~/code/cpp/core2/proposals/core2-template-brief.md` (git-ignored in core2; everything in it is now here). Still there on 2026-10-31.
2. [x] `plain` end to end: `cargo-generate.toml`, `post.rhai`, `setup.sh`, the Makefile, `main.cpp`, the client's `CLAUDE.md`, `.claude/settings.json`, `.vscode/`, `.gitignore`, `README.md`, `docs/core2.md`, and `check.sh`. Generates, builds warning-free in all four modes, runs (core2 `v0.13.1`).
   - Generating from GitHub (`--git https://github.com/efvincent/core2-template.git`, public since 2026-10-31) works, then `setup.sh`, build and run.
   - [x] `setup.sh` with the default SSH `core2_url`: `check.sh` uses it by default since 2026-10-31 (SSH to GitHub turned out to work from Claude's sandbox).
3. [x] `ncurses`, `sdl3`, and `imgui` with the four backends, each in `check.sh`. All seven variants build warning-free in all four modes and link without the C++ runtime. `./check.sh` runs `plain`, `ncurses` (also on a pseudo-terminal), `sdl3` and `imgui` + `sdl3_renderer` headless; `WINDOWED=1` ran `sdl3_gpu`, `glfw_opengl3` and `glfw_vulkan` on the author's desktop. `glfw_vulkan` uses the system Vulkan headers (decision 10).
4. [x] The work on branch `feature/ui-variants` (core2 protection, the variants, check.sh fixes): on `main`.
5. [x] In core2, in core2's own session: the docs that described core2-template as planned, or a client on make. Done in core2 `#40` (`8be1b2f`, docs only, no tag): README "A new project: core2-template" and "Adding it" (pins `v2.0.0`), `docs/claude.md` "In client projects" (core2's protected paths by name, the CMake build), `docs/roadmap.md`.
   - [x] The note on clang 23's missing debug info for templates instantiated across modules: core2 `v2.0.1`'s `docs/guide/debugging.md`. The client README's note links to it (item 9).
6. [x] Align with core2 `v0.14.0` and decision 11 (2026-11-01): default tag `v0.14.0`; the client's `CLAUDE.md` scopes its `third-party/` rules and points at core2's submodule workflow; `setup.sh` writes core2's `.claude/settings.local.json`; the client's `docs/core2.md` describes fixing core2 from the submodule; `check.sh` checks the settings file and that the submodule stays clean.

7. [x] CMake only, on core2 `v2.0.0` (decision 12), on `main` since 2026-11-16: `CMakeLists.txt` and `CMakePresets.json` in place of the Makefile, the client's docs, settings and `.vscode/` in CMake terms, `check.sh` on presets. The remote branch `feature/cmake` is still there.
8. [x] Editor, docs and README additions (decision 13), on `main` with item 7.
9. [x] core2 `v2.1.8` (2.0.1 to 2.1.8 are docs, plus the additive `extend` and `splitInto` in 2.1.0), `./check.sh` passed for all seven variants with `CORE2_VERSION=v2.1.8` (2026-11-17), so it is the default `core2_version`. The client README points at core2's `docs/guide/debugging.md` and the user guide; the top README is brought up to date. On branch `feature/copier` with item 10.
10. [x] Copier (decision 14) and `./dev` (decision 15): `copier.yml`, the `*.jinja` files, the variant directories, `./dev` (with the rerunnable setup Copier's task runs), `core2_docs` and the README link, `.vscode/extensions.json` and CMake Tools, protected controls, `check.sh` on Copier. On `main` and tagged `v1.0.0` (2026-11-17).
    - `./check.sh` passed for all seven variants with `WINDOWED=1` before `./dev`; `./check.sh plain` passes with `./dev`, also inside this repo's sandbox.
    - clangd on a generated `plain`: 0 errors. The author opened a generated `plain` in VSCodium: extensions, the CMake Tools preset prompt, clangd on `import core;`, F5, core2's reference.
    - `copier copy --trust gh:efvincent/core2-template` generates from the `v1.0.0` tag (`_commit: v1.0.0` in `.copier-answers.yml`), runs `./dev setup`, and leaves `dev` executable (2026-11-17). Inside Claude's sandbox, Copier needs `XDG_CACHE_HOME="$TMPDIR/cache"`: it caches remote templates in `~/.cache/copier`, which the sandbox does not let it write. A local path (as `check.sh` uses) needs no cache.
    - The remote branches `feature/copier` and `feature/cmake` are deleted.
11. [ ] `./check.sh` for all seven variants (with `WINDOWED=1` on the author's desktop) on `v1.0.0`, since `./dev` and the protections came after the last full run. About five minutes; run when the author asks.
12. [x] Releases (decision 16): `CHANGELOG.md` with `v1.0.0`, the README's "Releases", the convention in decision 16 (2026-11-17).
    - [x] The author applies `proposals/claude-md-releases.patch` (the release rule in this repo's `CLAUDE.md`, "Git") and creates the GitHub release for `v1.0.0` from `proposals/release-v1.0.0.md`.

## Open questions
None.
