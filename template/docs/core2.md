# core2 in this project

core2 is a git submodule at `third-party/core2`, from `{{core2_url}}`, pinned to a release tag (`{{core2_version}}` when the project was generated). The pin is the commit the superproject records for `third-party/core2` (`git submodule status` shows it).

## What comes from the pin
- The library itself, built from source by `third-party/core2/core.mk` with this project's flags.
- The code style: `CLAUDE.md` imports `@third-party/core2/docs/style.md`.
- Claude Code's comment-only hook: `.claude/settings.json` runs `third-party/core2/.claude/hooks/comment_only_guard.py`.

Bumping the pin updates all three.

## Bumping the pin
```sh
git -C third-party/core2 fetch --tags
git -C third-party/core2 checkout vX.Y.Z
git add third-party/core2
make
```
core2's modules depend on their sources, so `make` rebuilds the ones that changed.
Then commit (`chore: bump core2 to vX.Y.Z`). core2's `CHANGELOG.md` lists what changed; a minor bump before 1.0 can break the API.

## Changing core2
Nothing under `third-party/` is edited here. A change this project needs is made in core2's own repo, through its gate (branch, tests, review, release, tag); then this project bumps its pin. Other projects stay on their own pins until they bump.

## A missing submodule
After `git clone` without `--recurse-submodules`, `third-party/core2` is empty: `make` stops with a message, and the comment-only hook blocks every Claude Code edit. Fix:
```sh
git submodule update --init
```
