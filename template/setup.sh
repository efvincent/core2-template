#!/bin/sh
# One-time setup for {{project-name}}, generated from core2-template: adds the
# pinned submodules under third-party/, stages them, then deletes this script.
# Safe to rerun if it stops halfway (a network error, say).
set -eu
cd "$(dirname "$0")"

[ -d .git ] || git init -q

# add_pinned <url> <path> <tag>: the submodule at <path>, checked out at <tag>.
# The staged gitlink is the pin.
add_pinned() {
  if [ -z "$(git ls-files --stage -- "$2")" ]; then
    git submodule add -q -- "$1" "$2"
  else
    git submodule update -q --init -- "$2"
  fi
  git -C "$2" -c advice.detachedHead=false checkout -q "$3"
  git add .gitmodules "$2"
  echo "$2 pinned at $3"
}

add_pinned '{{core2_url}}' third-party/core2 '{{core2_version}}'
{%- if ui == "imgui" %}
add_pinned https://github.com/ocornut/imgui.git third-party/imgui v1.92.8
{%- endif %}

# core2's local Claude Code settings (git-ignored by core2). A session started
# in third-party/core2 works on core2 itself (docs/core2.md), but git keeps the
# submodule's data in this repo's .git/modules, outside that session's sandbox.
# Allow writes there so Claude can commit core2 work; not to the hooks or the
# config, which git runs and reads outside the sandbox. An existing file is
# kept. The path is absolute: after moving the project, see docs/core2.md.
core2_local=third-party/core2/.claude/settings.local.json
if [ ! -e "$core2_local" ]; then
  python3 -c '
import json, sys
g = sys.argv[1]
fs = { "allowWrite": [g], "denyWrite": [g + "/hooks", g + "/config"] }
with open(sys.argv[2], "w") as f:
  json.dump({ "sandbox": { "filesystem": fs } }, f, indent=2)
  f.write("\n")
' "$(git -C third-party/core2 rev-parse --absolute-git-dir)" "$core2_local"
  echo "$core2_local written (Claude can commit in a core2 session)"
fi

rm -f setup.sh
echo "Done. Build with: make"
