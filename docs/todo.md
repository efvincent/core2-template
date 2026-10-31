# To do

The order of work from the handoff brief, with status.

1. [x] This repo's `CLAUDE.md`, `README.md` and `docs/`, carrying over the brief's decisions (2026-10-31). The brief (`core2/proposals/core2-template-brief.md`) can then be deleted.
2. [x] `plain` end to end: `cargo-generate.toml`, `post.rhai`, `setup.sh`, the Makefile, `main.cpp`, the client's `CLAUDE.md`, `.claude/settings.json`, `.vscode/`, `.gitignore`, `README.md`, `docs/core2.md`, and `check.sh`. Generates, builds warning-free in all four modes, runs (2026-10-31, core2 `v0.13.1`).
   - Not yet tried: generating from the GitHub URL (`--git`, SSH), which Claude's sandbox cannot reach; the author should run it once after the first push.
3. [ ] `ncurses`, then `sdl3`, then `imgui` with the four backends, each added to `check.sh`.
   - `glfw_vulkan`: do the system Vulkan headers (1.4.363 installed) suffice, or pin `Vulkan-Headers` as the old template did?
   - Each ImGui backend: what it pulls in at link time without the C++ runtime.
4. [ ] In core2 (its own session and branch): point README's "Using core2 in another project" at core2-template, and update its roadmap line. The README's example pins `v0.6.0`; clients need `v0.13.1` or later.

## Open questions
- Decision 6: confirm the `<project>::<name>` namespace rule for client modules in core2's `docs/style.md`.
