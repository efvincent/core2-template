# Decisions

Made by the author unless noted. Carried over from the handoff brief written at the end of the core2 session that planned this repo (`core2/proposals/core2-template-brief.md`).

1. **A separate repo**, not part of core2 (2026-10-30). Each generated client gets core2 as a **git submodule from GitHub, pinned to a release tag**. No local-override mechanism: a core2 change needed while working on a client is made in core2 through core2's own gate (branch, tests, review, release, tag), and then that client's pin is bumped. Other clients stay on their pins.
2. **Claude writes the template**, all of it: `cargo-generate.toml`, the hook, `setup.sh`, the Makefile, the `main.cpp` variants, `check.sh`, the READMEs and docs (2026-10-30). In core2, by contrast, the author types `src/`, `examples/` and the `Makefile`. In a generated client the author owns `src/` and the `Makefile` again.
3. **UI choice:** `plain`, `ncurses`, `sdl3`, `imgui`. For `imgui` a second question asks for the backend, offering all four: SDL3 + SDL_Renderer, SDL3 + SDL_GPU, GLFW + OpenGL3, GLFW + Vulkan (2026-10-30).
4. **No tests in generated clients for now.** A more robust test harness is planned in core2 later; nothing here designs for it yet (2026-10-30).
5. **The comment-only hook is referenced from the submodule**, not copied: `python3 "$CLAUDE_PROJECT_DIR"/third-party/core2/.claude/hooks/comment_only_guard.py`. If the submodule is not checked out, every `Edit`/`Write` is blocked (fail closed); the client's README and `CLAUDE.md` say to run `git submodule update --init` (2026-10-31).
6. **The style is imported from the submodule:** the client's `CLAUDE.md` has `@third-party/core2/docs/style.md` (2026-10-31). `style.md` says a client's modules use `<project>::<name>` namespaces with helpers in `detail`. That rule is Claude's extrapolation from core2's `base::<name>`; the author has not objected, but it is unconfirmed.
7. **Generator: cargo-generate** (0.23.13 installed; `copier` was considered and not chosen) (2026-10-31).
8. **Submodules come from a one-time `setup.sh`, not the post hook** (2026-10-31). The brief had the hook run `git submodule add`; cargo-generate runs hooks in a temp dir and drops every `.git` directory when it moves the result, so that cannot work (details in [design.md](design.md), "Why setup.sh, not the hook"). The hook renders `setup.sh` with the URL and tag; the author runs it once after generating, and it deletes itself. Chosen over a wrapper script and a Makefile bootstrap.

## Claude's choices (2026-10-31; the author may overrule)
- The client's `CLAUDE.md` is stored as `template/CLAUDE.client.md` so it is not loaded as a nested `CLAUDE.md` while working on this repo; the hook renames it.
- The hook moves the whole `ui/<variant>/` directory to `src/` (cargo-generate's `file::rename` needs the target's parent to exist, and the template has no `src/` of its own).
- The client's `.claude/settings.json` adds `deny: ["Edit(/third-party/**)"]` next to the sandbox's `denyWrite`: `denyWrite` covers Bash only, and the comment-only hook guards only `src/` and the `Makefile`.
- The Makefile compiles every `src/**/*.cpp`, not only `main.cpp`, and guards the `core_modules` call instead of shipping a starter module.
- `check.sh` checks the `core2_version` default from `cargo-generate.toml` (one source of truth); `CORE2_VERSION=...` overrides it to try a new tag.
- The ImGui backend defaults to `sdl3_renderer`.
