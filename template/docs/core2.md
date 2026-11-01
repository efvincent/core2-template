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
Then commit (`chore(deps): core2 vX.Y.Z`). core2's `CHANGELOG.md` lists what changed; a minor bump before 1.0 can break the API.

## Changing core2
Nothing under `third-party/` is edited from a session of this project. A change this project needs is made in core2, through core2's gate (branch, tests, review, release, tag); then this project bumps its pin. Other projects stay on their own pins until they bump.

The fix can be made right here, in the submodule, as core2 work. core2's `docs/workflow.md`, "Working from a client's submodule", has the full flow. In short:
1. Start Claude Code in `third-party/core2`, not at this project's root. That is a core2 session: core2's `CLAUDE.md`, settings and hook apply. This project's `CLAUDE.md` is still loaded (it is in a parent directory), and it says its `third-party/` rules do not apply there.
2. Branch from core2's `main`: `git fetch origin && git switch -c fix/<topic> origin/main` (in `third-party/core2`). Edits made at the tag come along.
3. Work as in core2. This project's `make` builds core2 from the submodule's working tree, so a fix can be tried here before it is committed.
4. Commit, review, push, PR, merge and tag in core2. Then, in `third-party/core2`, `git switch main && git pull && git checkout vX.Y.Z`, and bump the pin as above.

Until step 4, this project's `git status` shows `third-party/core2` as modified. Do not stage it before the tag.

### Letting Claude commit in a core2 session
git keeps the submodule's data in this project's `.git/modules/third-party/core2`, outside a core2 session's sandbox, so Claude can edit but not commit there unless core2's git-ignored `third-party/core2/.claude/settings.local.json` allows it. `setup.sh` wrote that file. After a fresh clone of this project, or after moving it, (re)create it from the project's root:
```sh
python3 -c '
import json, sys
g = sys.argv[1]
fs = { "allowWrite": [g], "denyWrite": [g + "/hooks", g + "/config"] }
with open(sys.argv[2], "w") as f:
  json.dump({ "sandbox": { "filesystem": fs } }, f, indent=2)
  f.write("\n")
' "$(git -C third-party/core2 rev-parse --absolute-git-dir)" third-party/core2/.claude/settings.local.json
```
The git directory's `hooks/` and `config` stay denied: git runs and reads them outside the sandbox. A running session picks the file up without a restart.

## A missing submodule
After `git clone` without `--recurse-submodules`, `third-party/core2` is empty: `make` stops with a message, and the comment-only hook blocks every Claude Code edit. Fix:
```sh
git submodule update --init
```
