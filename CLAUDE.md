# Working agreement (core2-template)

This repo is a Copier template for projects built on core2 (`~/code/cpp/core2`, `git@github.com:efvincent/core2.git`). Unlike core2, **Claude writes everything here**: `template/` (the payload), `copier.yml`, `check.sh`, `README.md`, `docs/`. The author reviews, decides, pushes and merges.

Except the controls: this `CLAUDE.md` and `.claude/` (the sandbox and permissions) are the author's. Claude never edits them, as in core2: a change to them always comes as a patch file in `proposals/` (git-ignored; a unified diff, checked with `git apply --check`), with the command to apply it. `.claude/settings.json` enforces it (`deny` for the edit tools, `denyWrite` for Bash). The generated project protects its own `CLAUDE.md` and `.claude/` the same way. `template/CLAUDE.md.jinja` and `template/.claude/settings.json` are payload, not controls here: Claude writes them.

## The payload is not this repo's rules
- `template/` is what a generated client gets. Its rules are for the client, not for work on this repo.
- The client's working agreement is `template/CLAUDE.md.jinja` (Copier drops the suffix), so it is never loaded as a nested `CLAUDE.md` while working here. Keep the suffix.
- `template/.claude/settings.json` (the client's sandbox, `denyWrite` and hook) has no effect here.

## Sandbox
This repo's `.claude/settings.json` runs every Bash command in Claude Code's sandbox (`failIfUnavailable`, no unsandboxed commands): writes only inside this repo and `$TMPDIR`, never to `CLAUDE.md` or `.claude/`, network only to `github.com` (core2 over SSH, ImGui over HTTPS). It also denies the edit tools on `CLAUDE.md` and `.claude/`. `check.sh` works there: its temp dir is under `$TMPDIR`. The sandbox mounts `/dev/null` over some dotfiles in the repo (`.bashrc`, `.gitconfig`, `.mcp.json`, parts of `.claude/`, ...); `.gitignore` lists them so Copier's `git add -A` of the working tree skips them. Never edit `~/code/cpp/core2`: the sandbox would refuse anyway.

## How the template works
Read [docs/design.md](docs/design.md) before changing the template: layout, questions, what Copier and `./dev` do, the client's CMake build and settings, and why. The decisions and their dates are in [docs/decisions.md](docs/decisions.md); open work is in [docs/todo.md](docs/todo.md).

Short version:
- `copier copy --trust gh:efvincent/core2-template <dir>` (or `copier copy --trust . <dir>` locally, working tree included) asks the questions in `copier.yml` and renders `template/` (`_subdirectory`). Only `*.jinja` files are rendered; the rest, the C++ sources included, are copied as is (`{{` is valid C++).
- Each UI variant is one plain `main.cpp` in a directory named `{% if variant == '<variant>' %}src{% endif %}`: the chosen one renders to `src/`, the others to an empty name, which Copier skips.
- Copier's task then runs the new project's `./dev setup` in the project: submodules pinned to tags, core2's local Claude Code settings, the debug build (for clangd), core2's reference. `./dev` stays in the project as its one command (build, run, docs, clean, setup) and the setup is safe to rerun (decisions 14 and 15).

## Checking
- `./check.sh plain` generates the `plain` variant into a temp dir, runs its `./dev setup` (core2 from `core2_url`'s default, `git@github.com:efvincent/core2.git`), builds every mode, and runs the binary. Run it after any change to `template/` or `check.sh`, and report the output.
- `./check.sh` with no arguments checks all seven variants (ImGui from GitHub over HTTPS) and takes about five minutes. Run it only when the author asks.
- `CORE2_VERSION=vX.Y.Z ./check.sh plain` checks a newer core2 tag. When it passes, bump the default of `core2_version` in `copier.yml` (and the facts in `docs/` and `README.md`).
- `CORE2_DIR=~/code/cpp/core2 ./check.sh plain` uses the local core2 clone instead: offline for core2, and the way to check a tag that is not pushed yet.

## Changes to core2
Never edit `~/code/cpp/core2` from here. If the template needs a core2 change, say so and describe it; it goes through core2's own session and gate (branch, tests, review, release, tag).

## Proposals for other projects
A suggestion for another project (core2, a generated client, ...) goes in `proposals/` as `<project>-<proposal>.md`, for example `proposals/core2-clang-tidy.md`: what to change, why, and the evidence (file and line, commands to reproduce). The other project's own session acts on it.

## Style
- C++ in `template/*src*/` follows core2's `docs/style.md` (`~/code/cpp/core2/docs/style.md`): Orthodox / Handmade C++26, trailing return types, 2-space indent, no STD, RAII, RTTI or exceptions; C headers for ncurses, SDL3, GLFW are fine.
- Markdown: short, factual, no filler. The client's docs speak to the client's author, not about this repo.

## Git
- Feature branches (`feature/<topic>`, `fix/<topic>`, `docs/<topic>`, `chore/<topic>`), Conventional Commits, local commits when the author says go. Stage named files, never `git add -A`.
- No `Co-Authored-By` line, as in core2.
- Claude pushes, deletes remote branches or opens PRs only when the author asks; otherwise the author pushes and merges.
- Releases (decision 16): a branch that changes what a generated project gets (`template/`, `copier.yml`) adds its entry to `CHANGELOG.md` under `[Unreleased]`, and says whether it is a patch, minor or major release (counted from a generated project's side). Claude gives the tag and `gh release create` commands, with the notes in `proposals/`; the author tags. A branch that changes only this repo is not released.
