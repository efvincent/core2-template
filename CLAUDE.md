# Working agreement (core2-template)

This repo is a cargo-generate template for projects built on core2 (`~/code/cpp/core2`, `git@github.com:efvincent/core2.git`). Unlike core2, **Claude writes everything here**: `template/` (the payload), `check.sh`, `README.md`, `docs/`. The author reviews, decides, pushes and merges.

## The payload is not this repo's rules
- `template/` is what a generated client gets. Its rules are for the client, not for work on this repo.
- The client's working agreement is stored as `template/CLAUDE.client.md` and renamed to `CLAUDE.md` by the post hook, so it is never loaded as a nested `CLAUDE.md` while working here. Keep that name.
- `template/.claude/settings.json` (the client's sandbox, `denyWrite` and hook) has no effect here.

## How the template works
Read [docs/design.md](docs/design.md) before changing the template: layout, placeholders, what the post hook and `setup.sh` do, the client's Makefile and settings, and why. The decisions and their dates are in [docs/decisions.md](docs/decisions.md); open work is in [docs/todo.md](docs/todo.md).

Short version:
- `cargo generate --git <repo> template` (or `--path . template` locally) renders `template/`. The post hook (`template/post.rhai`) only arranges files: it moves the chosen `ui/<variant>/` to `src/`, renames `CLAUDE.client.md`, and deletes itself.
- The generated project's `setup.sh`, run once, adds core2 (and ImGui) as git submodules pinned to tags, then deletes itself. The hook cannot do this: cargo-generate runs it in a temp dir and drops every `.git` directory when it copies the result (decision 8).
- One plain C++ file per UI variant, excluded from Liquid rendering (`{{` is valid C++).

## Checking
- `./check.sh` generates every variant into a temp dir, runs its `setup.sh` against the local core2 clone (offline for core2; ImGui comes from GitHub over HTTPS), builds every mode, and runs the binaries. Run it after any change to `template/` or `check.sh`, and report the output.
- `CORE2_VERSION=vX.Y.Z ./check.sh` checks a newer core2 tag. When it passes, bump the default of `core2_version` in `template/cargo-generate.toml` (and the facts in `docs/`).
- The sandbox blocks `github.com:22`, so Claude cannot generate from the SSH URL; `check.sh` passes `core2_url=$HOME/code/cpp/core2`.

## Changes to core2
Never edit `~/code/cpp/core2` from here. If the template needs a core2 change, say so and describe it; it goes through core2's own session and gate (branch, tests, review, release, tag).

## Style
- C++ in `template/ui/` follows core2's `docs/style.md` (`~/code/cpp/core2/docs/style.md`): Orthodox / Handmade C++26, trailing return types, 2-space indent, no STD, RAII, RTTI or exceptions; C headers for ncurses, SDL3, GLFW are fine.
- Markdown: short, factual, no filler. The client's docs speak to the client's author, not about this repo.

## Git
- Feature branches (`feature/<topic>`, `fix/<topic>`, `docs/<topic>`, `chore/<topic>`), Conventional Commits, local commits when the author says go. Stage named files, never `git add -A`.
- No `Co-Authored-By` line, as in core2.
- Claude does not push (the sandbox blocks `github.com:22`); the author pushes and merges.
